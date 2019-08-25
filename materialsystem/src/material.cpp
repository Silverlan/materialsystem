/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "material.h"
#include <sharedutils/util_shaderinfo.hpp>
#include <sstream>
#include <fsys/filesystem.h>
#include <sharedutils/util_file.h>

DEFINE_BASE_HANDLE(DLLMATSYS,Material,Material);

Material::Material(MaterialManager *manager)
	: m_handle(new PtrMaterial(this)),m_data(nullptr),m_shader(nullptr),
	m_bLoaded(false),m_manager(manager)
{
	Reset();
}

Material::Material(MaterialManager *manager,const util::WeakHandle<util::ShaderInfo> &shaderInfo,const std::shared_ptr<ds::Block> &data)
	: Material(manager)
{
	Initialize(shaderInfo,data);
}

Material::Material(MaterialManager *manager,const std::string &shader,const std::shared_ptr<ds::Block> &data)
	: Material(manager)
{
	Initialize(shader,data);
}
MaterialHandle Material::GetHandle() {return m_handle;}

void Material::Remove() {delete this;}

void Material::Reset()
{
	m_bTranslucent = false;
	m_bLoaded = false;
	m_data = nullptr;
	m_shaderInfo.reset();
	m_shader = nullptr;
	m_texDiffuse = nullptr;
	m_texNormal = nullptr;
	m_userData = nullptr;
	m_texSpecular = nullptr;
	m_texGlow = nullptr;
	m_texParallax = nullptr;
	m_texAo = nullptr;
	m_texAlpha = nullptr;
}

void Material::Initialize(const util::WeakHandle<util::ShaderInfo> &shaderInfo,const std::shared_ptr<ds::Block> &data)
{
	Reset();
	SetShaderInfo(shaderInfo);
	m_data = data;
	UpdateTextures();
}

void Material::Initialize(const std::string &shader,const std::shared_ptr<ds::Block> &data)
{
	Reset();
	m_shader = std::make_unique<std::string>(shader);
	m_data = data;
	UpdateTextures();
}

void *Material::GetUserData() {return m_userData;}
void Material::SetUserData(void *data) {m_userData = data;}

bool Material::IsTranslucent() const {return m_bTranslucent;}

void Material::UpdateTextures()
{
	m_texDiffuse = GetTextureInfo("diffusemap");
	if(!m_texDiffuse)
		m_texDiffuse = GetTextureInfo("diffuse_map");
	if(!m_texDiffuse)
		m_texDiffuse = GetTextureInfo("albedomap");
	if(!m_texDiffuse)
		m_texDiffuse = GetTextureInfo("albedo_map");

	m_texNormal = GetTextureInfo("normalmap");
	if(!m_texNormal)
		m_texNormal = GetTextureInfo("normal_map");

	m_texSpecular = GetTextureInfo("specularmap");
	if(!m_texSpecular)
		m_texSpecular = GetTextureInfo("specular_map");

	m_texGlow = GetTextureInfo("glowmap");
	if(!m_texGlow)
		m_texGlow = GetTextureInfo("glow_map");
	if(!m_texGlow)
		m_texGlow = GetTextureInfo("emission_map");

	m_texParallax = GetTextureInfo("parallaxmap");
	if(!m_texParallax)
		m_texParallax = GetTextureInfo("parallax_map");

	m_texAo = GetTextureInfo("ambientocclusionmap");
	if(!m_texAo)
		m_texAo = GetTextureInfo("ambient_occlusion_map");
	if(!m_texAo)
		m_texAo = GetTextureInfo("ao_map");

	m_texAlpha = GetTextureInfo("alphamap");
	if(!m_texAlpha)
		m_texAlpha = GetTextureInfo("alpha_map");

	m_bTranslucent = m_data->GetBool("translucent");
}

void Material::SetShaderInfo(const util::WeakHandle<util::ShaderInfo> &shaderInfo)
{
	m_shaderInfo = shaderInfo;
	m_shader = nullptr;
}

Material::~Material()
{
	m_handle.Invalidate();
	for(auto &hCb : m_callOnLoaded)
	{
		if(hCb.IsValid() == true)
			hCb.Remove();
	}
}

Material *Material::Copy() const {return Copy<Material>();}

bool Material::IsValid() const {return (m_data != nullptr) ? true : false;}
MaterialManager *Material::GetManager() const {return m_manager;}
void Material::SetLoaded(bool b)
{
	m_bLoaded = b;
	if(b == true)
	{
		for(auto &f : m_callOnLoaded)
		{
			if(f.IsValid())
				f();
		}
		m_callOnLoaded.clear();
	}
}
bool Material::Save(const std::string &fileName,const std::string &inRootPath) const
{
	auto rootPath = inRootPath;
	if(rootPath.empty() == false && rootPath.back() != '/' && rootPath.back() != '\\')
		rootPath += '/';
	auto fullPath = rootPath +"materials/" +fileName;
	ufile::remove_extension_from_filename(fullPath);
	fullPath += ".wmi";

	auto pathWithoutFileName = ufile::get_path_from_filename(fullPath);
	FileManager::CreatePath(pathWithoutFileName.c_str());

	auto f = FileManager::OpenFile<VFilePtrReal>(fullPath.c_str(),"w");
	if(f == nullptr)
		return false;
	auto &rootData = GetDataBlock();
	std::stringstream ss;
	ss<<"\""<<GetShaderIdentifier()<<"\"\n{\n";
	std::function<void(const ds::Block&,const std::string&)> fIterateDataBlock = nullptr;
	fIterateDataBlock = [&ss,&fIterateDataBlock](const ds::Block &block,const std::string &t) {
		auto *data = block.GetData();
		if(data == nullptr)
			return;
		for(auto &pair : *data)
		{
			if(pair.second->IsBlock())
			{
				auto &block = static_cast<ds::Block&>(*pair.second);
				ss<<t<<"\""<<pair.first<<"\"\n"<<t<<"{\n";
				fIterateDataBlock(block,t +'\t');
				ss<<t<<"}\n";
				continue;
			}
			if(pair.second->IsContainer())
			{
				auto &container = static_cast<ds::Container&>(*pair.second);
				ss<<t<<"\""<<pair.first<<"\"\n"<<t<<"{\n";
				for(auto &block : container.GetBlocks())
				{
					if(block->IsContainer() || block->IsBlock())
						throw std::invalid_argument{"Data set block may only contain values!"};
					auto *dsValue = dynamic_cast<ds::Value*>(pair.second.get());
					if(dsValue == nullptr)
						throw std::invalid_argument{"Unexpected data set type!"};
					ss<<t<<"\t\""<<dsValue->GetString()<<"\"\n";
				}
				ss<<t<<"}\n";
				continue;
			}
			auto *dsValue = dynamic_cast<ds::Value*>(pair.second.get());
			if(dsValue == nullptr)
				throw std::invalid_argument{"Unexpected data set type!"};
			ss<<t<<"$"<<dsValue->GetTypeString()<<" "<<pair.first<<" \""<<dsValue->GetString()<<"\"\n";
		}
	};
	fIterateDataBlock(*rootData,"\t");
	ss<<"}\n";

	f->WriteString(ss.str());
	return true;
}
CallbackHandle Material::CallOnLoaded(const std::function<void(void)> &f) const
{
	if(IsLoaded())
	{
		f();
		return {};
	}
	m_callOnLoaded.push_back(FunctionCallback<>::Create(f));
	return m_callOnLoaded.back();
}
bool Material::IsLoaded() const {return m_bLoaded;}
const TextureInfo *Material::GetDiffuseMap() const {return const_cast<Material*>(this)->GetDiffuseMap();}
TextureInfo *Material::GetDiffuseMap() {return m_texDiffuse;}
const TextureInfo *Material::GetNormalMap() const {return const_cast<Material*>(this)->GetNormalMap();}
TextureInfo *Material::GetNormalMap() {return m_texNormal;}
const TextureInfo *Material::GetSpecularMap() const {return const_cast<Material*>(this)->GetSpecularMap();}
TextureInfo *Material::GetSpecularMap() {return m_texSpecular;}
const TextureInfo *Material::GetGlowMap() const {return const_cast<Material*>(this)->GetGlowMap();}
TextureInfo *Material::GetGlowMap() {return m_texGlow;}
const TextureInfo *Material::GetAlphaMap() const {return const_cast<Material*>(this)->GetAlphaMap();}
TextureInfo *Material::GetAlphaMap() {return m_texAlpha;}
const TextureInfo *Material::GetParallaxMap() const {return const_cast<Material*>(this)->GetParallaxMap();}
TextureInfo *Material::GetParallaxMap() {return m_texParallax;}
const TextureInfo *Material::GetAmbientOcclusionMap() const {return const_cast<Material*>(this)->GetAmbientOcclusionMap();}
TextureInfo *Material::GetAmbientOcclusionMap() {return m_texAo;}
void Material::SetName(const std::string &name) {m_name = name;}
const std::string &Material::GetName() {return m_name;}

const util::ShaderInfo *Material::GetShaderInfo() const {return m_shaderInfo.get();}
const std::string &Material::GetShaderIdentifier() const
{
	if(m_shaderInfo.expired() == false)
		return m_shaderInfo.get()->GetIdentifier();
	return *m_shader;
}

const TextureInfo *Material::GetTextureInfo(const std::string &key) const {return const_cast<Material*>(this)->GetTextureInfo(key);}

TextureInfo *Material::GetTextureInfo(const std::string &key)
{
	auto &base = m_data->GetValue(key);
	if(base == nullptr || base->IsBlock())
		return nullptr;
	auto &val = static_cast<ds::Value&>(*base);
	auto &type = typeid(val);
	if(type != typeid(ds::Texture))
		return nullptr;
	auto &datTex = static_cast<ds::Texture&>(val);
	return &const_cast<TextureInfo&>(datTex.GetValue());
}

const std::shared_ptr<ds::Block> &Material::GetDataBlock() const
{
	static std::shared_ptr<ds::Block> nptr = nullptr;
	return (m_data != nullptr) ? m_data : nptr;
}
