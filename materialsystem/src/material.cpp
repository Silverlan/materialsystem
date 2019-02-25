/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "material.h"
#include <sharedutils/util_shaderinfo.hpp>

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
	m_texNormal = GetTextureInfo("normalmap");
	m_texSpecular = GetTextureInfo("specularmap");
	m_texGlow = GetTextureInfo("glowmap");
	m_texParallax = GetTextureInfo("parallaxmap");
	m_texAlpha = GetTextureInfo("alphamap");
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
CallbackHandle Material::CallOnLoaded(const std::function<void(void)> &f)
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
TextureInfo *Material::GetDiffuseMap() {return m_texDiffuse;}
TextureInfo *Material::GetNormalMap() {return m_texNormal;}
TextureInfo *Material::GetSpecularMap() {return m_texSpecular;}
TextureInfo *Material::GetGlowMap() {return m_texGlow;}
TextureInfo *Material::GetAlphaMap() {return m_texAlpha;}
TextureInfo *Material::GetParallaxMap() {return m_texParallax;}
void Material::SetName(const std::string &name) {m_name = name;}
const std::string &Material::GetName() {return m_name;}

const util::ShaderInfo *Material::GetShaderInfo() const {return m_shaderInfo.get();}
const std::string &Material::GetShaderIdentifier() const
{
	if(m_shaderInfo.expired() == false)
		return m_shaderInfo.get()->GetIdentifier();
	return *m_shader;
}

TextureInfo *Material::GetTextureInfo(const std::string &key)
{
	auto &base = m_data->GetValue(key);
	if(base == nullptr || base->IsBlock())
		return nullptr;
	auto &val = static_cast<ds::Value&>(*base);
	auto &type = typeid(val);
	if(type != typeid(ds::Texture))
	{
		if(type == typeid(ds::Cubemap))
		{
			auto &datTex = static_cast<ds::Cubemap&>(val);
			return &const_cast<TextureInfo&>(datTex.GetValue());
		}
		return nullptr;
	}
	auto &datTex = static_cast<ds::Texture&>(val);
	return &const_cast<TextureInfo&>(datTex.GetValue());
}

const std::shared_ptr<ds::Block> &Material::GetDataBlock()
{
	static std::shared_ptr<ds::Block> nptr = nullptr;
	return (m_data != nullptr) ? m_data : nptr;
}
