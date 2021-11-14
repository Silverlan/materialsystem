/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "material.h"
#include "material_copy.hpp"
#include "materialmanager.h"
#include <sharedutils/alpha_mode.hpp>
#include <sharedutils/util_shaderinfo.hpp>
#include <sstream>
#include <fsys/filesystem.h>
#include <datasystem_vector.h>
#include <sharedutils/util_string.h>
#include <sharedutils/util_file.h>
#include <sharedutils/util_path.hpp>
#include <datasystem_color.h>
#include <udm.hpp>

DEFINE_BASE_HANDLE(DLLMATSYS,Material,Material);

#undef CreateFile

decltype(Material::DIFFUSE_MAP_IDENTIFIER) Material::DIFFUSE_MAP_IDENTIFIER = "diffuse_map";
decltype(Material::ALBEDO_MAP_IDENTIFIER) Material::ALBEDO_MAP_IDENTIFIER = "albedo_map";
decltype(Material::ALBEDO_MAP2_IDENTIFIER) Material::ALBEDO_MAP2_IDENTIFIER = "albedo_map2";
decltype(Material::ALBEDO_MAP3_IDENTIFIER) Material::ALBEDO_MAP3_IDENTIFIER = "albedo_map3";
decltype(Material::NORMAL_MAP_IDENTIFIER) Material::NORMAL_MAP_IDENTIFIER = "normal_map";
decltype(Material::GLOW_MAP_IDENTIFIER) Material::GLOW_MAP_IDENTIFIER = "emission_map";
decltype(Material::EMISSION_MAP_IDENTIFIER) Material::EMISSION_MAP_IDENTIFIER = GLOW_MAP_IDENTIFIER;
decltype(Material::PARALLAX_MAP_IDENTIFIER) Material::PARALLAX_MAP_IDENTIFIER = "parallax_map";
decltype(Material::ALPHA_MAP_IDENTIFIER) Material::ALPHA_MAP_IDENTIFIER = "alpha_map";
decltype(Material::RMA_MAP_IDENTIFIER) Material::RMA_MAP_IDENTIFIER = "rma_map";
decltype(Material::DUDV_MAP_IDENTIFIER) Material::DUDV_MAP_IDENTIFIER = "dudv_map";
decltype(Material::WRINKLE_STRETCH_MAP_IDENTIFIER) Material::WRINKLE_STRETCH_MAP_IDENTIFIER = "wrinkle_stretch_map";
decltype(Material::WRINKLE_COMPRESS_MAP_IDENTIFIER) Material::WRINKLE_COMPRESS_MAP_IDENTIFIER = "wrinkle_compress_map";
decltype(Material::EXPONENT_MAP_IDENTIFIER) Material::EXPONENT_MAP_IDENTIFIER = "exponent_map";

Material *Material::Create(MaterialManager &manager)
{
	auto *mat = new Material{manager};
	mat->Reset();
	return mat;
}
Material *Material::Create(MaterialManager &manager,const util::WeakHandle<util::ShaderInfo> &shaderInfo,const std::shared_ptr<ds::Block> &data)
{
	auto *mat = new Material{manager,shaderInfo,data};
	mat->Initialize(shaderInfo,data);
	return mat;
}
Material *Material::Create(MaterialManager &manager,const std::string &shader,const std::shared_ptr<ds::Block> &data)
{
	auto *mat = new Material{manager,shader,data};
	mat->Initialize(shader,data);
	return mat;
}
Material::Material(MaterialManager &manager)
	: m_handle(new PtrMaterial(this)),m_data(nullptr),m_shader(nullptr),m_manager(manager)
{}

Material::Material(MaterialManager &manager,const util::WeakHandle<util::ShaderInfo> &shaderInfo,const std::shared_ptr<ds::Block> &data)
	: Material(manager)
{}

Material::Material(MaterialManager &manager,const std::string &shader,const std::shared_ptr<ds::Block> &data)
	: Material(manager)
{}
MaterialHandle Material::GetHandle() {return m_handle;}

void Material::Remove() {delete this;}

void Material::Reset()
{
	umath::set_flag(m_stateFlags,StateFlags::Loaded,false);
	m_data = nullptr;
	m_shaderInfo.reset();
	m_shader = nullptr;
	m_texDiffuse = nullptr;
	m_texNormal = nullptr;
	m_userData = nullptr;
	m_texGlow = nullptr;
	m_texParallax = nullptr;
	m_texRma = nullptr;
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

bool Material::IsTranslucent() const {return m_alphaMode == AlphaMode::Blend;}

void Material::UpdateTextures()
{
	m_texDiffuse = GetTextureInfo(DIFFUSE_MAP_IDENTIFIER);
	if(!m_texDiffuse)
		m_texDiffuse = GetTextureInfo(ALBEDO_MAP_IDENTIFIER);

	m_texNormal = GetTextureInfo(NORMAL_MAP_IDENTIFIER);
	m_texGlow = GetTextureInfo(EMISSION_MAP_IDENTIFIER);
	m_texParallax = GetTextureInfo(PARALLAX_MAP_IDENTIFIER);
	m_texAlpha = GetTextureInfo(ALPHA_MAP_IDENTIFIER);
	m_texRma = GetTextureInfo(RMA_MAP_IDENTIFIER);

	auto &data = GetDataBlock();
	if(data->IsString("alpha_mode"))
	{
		auto e = magic_enum::enum_cast<AlphaMode>(data->GetString("alpha_mode"));
		m_alphaMode = e.has_value() ? *e : AlphaMode::Opaque;
	}
	else
		m_alphaMode = static_cast<AlphaMode>(data->GetInt("alpha_mode",umath::to_integral(AlphaMode::Opaque)));

	OnTexturesUpdated();
}

void Material::OnTexturesUpdated() {}

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
MaterialManager &Material::GetManager() const {return m_manager;}
void Material::SetLoaded(bool b)
{
	if(umath::is_flag_set(m_stateFlags,StateFlags::ExecutingOnLoadCallbacks))
		return; // Prevent possible recursion while on-load callbacks are being executed
	umath::set_flag(m_stateFlags,StateFlags::Loaded,b);
	if(b == true)
	{
		umath::set_flag(m_stateFlags,StateFlags::ExecutingOnLoadCallbacks,true);
		for(auto &f : m_callOnLoaded)
		{
			if(f.IsValid())
				f();
		}
		m_callOnLoaded.clear();
		umath::set_flag(m_stateFlags,StateFlags::ExecutingOnLoadCallbacks,false);
	}
}
bool Material::Save(udm::AssetData outData,std::string &outErr)
{
	auto udm = (*outData)[GetShaderIdentifier()];
	std::function<void(udm::LinkedPropertyWrapper,ds::Block&)> dataBlockToUdm = nullptr;
	dataBlockToUdm = [&dataBlockToUdm,&udm](udm::LinkedPropertyWrapper prop,ds::Block &block) {
		for(auto &pair : *block.GetData())
		{
			auto &key = pair.first;
			auto &val = pair.second;
			if(val->IsBlock())
			{
				auto &block = static_cast<ds::Block&>(*pair.second);
				dataBlockToUdm(prop[key],block);
				continue;
			}
			if(val->IsContainer())
			{
				auto &container = static_cast<ds::Container&>(*pair.second);
				auto &children = container.GetBlocks();
				auto udmChildren = prop.AddArray(key,children.size());
				uint32_t idx = 0;
				for(auto &child : children)
				{
					if(child->IsContainer() || child->IsBlock())
						continue;
					auto *dsValue = dynamic_cast<ds::Value*>(pair.second.get());
					if(dsValue == nullptr)
						continue;
					udmChildren[idx++] = dsValue->GetString();
				}
				udmChildren.Resize(idx);
				continue;
			}
			auto *dsValue = dynamic_cast<ds::Value*>(val.get());
			assert(dsValue);
			if(dsValue)
			{
				auto *dsStr = dynamic_cast<ds::String*>(dsValue);
				if(dsStr)
					prop[key] = dsStr->GetString();
				auto *dsInt = dynamic_cast<ds::Int*>(dsValue);
				if(dsInt)
					prop[key] = dsInt->GetInt();
				auto *dsFloat = dynamic_cast<ds::Float*>(dsValue);
				if(dsFloat)
					prop[key] = dsFloat->GetFloat();
				auto *dsBool = dynamic_cast<ds::Bool*>(dsValue);
				if(dsBool)
					prop[key] = dsBool->GetBool();
				auto *dsVec = dynamic_cast<ds::Vector*>(dsValue);
				if(dsVec)
					prop[key] = dsVec->GetVector();
				auto *dsVec4 = dynamic_cast<ds::Vector4*>(dsValue);
				if(dsVec4)
					prop[key] = dsVec4->GetVector4();
				auto *dsVec2 = dynamic_cast<ds::Vector2*>(dsValue);
				if(dsVec2)
					prop[key] = dsVec2->GetVector2();
				auto *dsTex = dynamic_cast<ds::Texture*>(dsValue);
				if(dsTex)
					udm["textures"][key] = dsTex->GetString();
				auto *dsCol = dynamic_cast<ds::Color*>(dsValue);
				if(dsCol)
					prop[key] = dsCol->GetColor().ToVector4();
			}
		}
	};

	outData.SetAssetType(PMAT_IDENTIFIER);
	outData.SetAssetVersion(PMAT_VERSION);
	dataBlockToUdm(udm["properties"],*m_data);
	return true;
}
extern const std::array<std::string,5> g_knownMaterialFormats;
bool Material::Save(const std::string &relFileName,std::string &outErr,bool absolutePath)
{
	auto udmData = udm::Data::Create();
	std::string err;
	auto result = Save(udmData->GetAssetData(),err);
	if(result == false)
		return false;
	auto fileName = relFileName;
	if(absolutePath == false)
		fileName = "materials/" +fileName;
	FileManager::CreatePath(ufile::get_path_from_filename(fileName).c_str());
	auto writeFileName = fileName;
	ufile::remove_extension_from_filename(writeFileName,g_knownMaterialFormats);
	writeFileName += '.' +std::string{FORMAT_MATERIAL_ASCII};
	auto f = FileManager::OpenFile<VFilePtrReal>(writeFileName.c_str(),"w");
	if(f == nullptr)
	{
		outErr = "Unable to open file '" +writeFileName +"'!";
		return false;
	}
	result = udmData->SaveAscii(f,udm::AsciiSaveFlags::None);
	if(result == false)
	{
		outErr = "Unable to save UDM data!";
		return false;
	}
	return true;
}
bool Material::Save(std::string &outErr)
{
	auto mdlName = GetName();
	std::string absFileName;
	auto result = FileManager::FindAbsolutePath("materials/" +mdlName,absFileName);
	auto absolutePath = false;
	if(result == false)
		absFileName = mdlName;
	else
	{
		auto path = util::Path::CreateFile(absFileName);
		path.MakeRelative(util::get_program_path());
		absFileName = path.GetString();
		absolutePath = true;
	}
	return Save(absFileName,outErr,absolutePath);
}
bool Material::SaveLegacy(std::shared_ptr<VFilePtrInternalReal> f) const
{
	auto &rootData = GetDataBlock();
	std::stringstream ss;
	ss<<rootData->ToString(GetShaderIdentifier());

	f->WriteString(ss.str());
	return true;
}
std::optional<std::string> Material::GetAbsolutePath() const
{
	auto name = const_cast<Material*>(this)->GetName();
	if(name.empty())
		return {};
	std::string absPath = GetManager().GetRootMaterialLocation() +"\\";
	absPath += name;
	ufile::remove_extension_from_filename(absPath,g_knownMaterialFormats);
	absPath += ".wmi";
	if(FileManager::FindLocalPath(absPath,absPath) == false)
		return {};
	return absPath;
}
bool Material::SaveLegacy() const
{
	auto name = const_cast<Material*>(this)->GetName();
	if(name.empty())
		return false;
	std::string absPath = GetManager().GetRootMaterialLocation() +"\\";
	absPath += name;
	ufile::remove_extension_from_filename(absPath,g_knownMaterialFormats);
	absPath += ".wmi";
	if(FileManager::FindLocalPath(absPath,absPath) == false)
		absPath = "addons/converted/" +absPath;

	auto f = FileManager::OpenFile<VFilePtrReal>(absPath.c_str(),"w");
	if(f == nullptr)
		return false;
	return SaveLegacy(f);
}
bool Material::SaveLegacy(const std::string &fileName,const std::string &inRootPath) const
{
	auto rootPath = inRootPath;
	if(rootPath.empty() == false && rootPath.back() != '/' && rootPath.back() != '\\')
		rootPath += '/';
	auto fullPath = rootPath +MaterialManager::GetRootMaterialLocation() +"/" +fileName;
	ufile::remove_extension_from_filename(fullPath,g_knownMaterialFormats);
	fullPath += ".wmi";

	auto pathWithoutFileName = ufile::get_path_from_filename(fullPath);
	FileManager::CreatePath(pathWithoutFileName.c_str());

	auto f = FileManager::OpenFile<VFilePtrReal>(fullPath.c_str(),"w");
	if(f == nullptr)
		return false;
	return SaveLegacy(f);
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
bool Material::IsLoaded() const {return umath::is_flag_set(m_stateFlags,StateFlags::Loaded);}

const TextureInfo *Material::GetDiffuseMap() const {return const_cast<Material*>(this)->GetDiffuseMap();}
TextureInfo *Material::GetDiffuseMap() {return m_texDiffuse;}

const TextureInfo *Material::GetAlbedoMap() const {return GetDiffuseMap();}
TextureInfo *Material::GetAlbedoMap() {return GetDiffuseMap();}

const TextureInfo *Material::GetNormalMap() const {return const_cast<Material*>(this)->GetNormalMap();}
TextureInfo *Material::GetNormalMap() {return m_texNormal;}

const TextureInfo *Material::GetGlowMap() const {return const_cast<Material*>(this)->GetGlowMap();}
TextureInfo *Material::GetGlowMap() {return m_texGlow;}

const TextureInfo *Material::GetAlphaMap() const {return const_cast<Material*>(this)->GetAlphaMap();}
TextureInfo *Material::GetAlphaMap() {return m_texAlpha;}

const TextureInfo *Material::GetParallaxMap() const {return const_cast<Material*>(this)->GetParallaxMap();}
TextureInfo *Material::GetParallaxMap() {return m_texParallax;}

const TextureInfo *Material::GetRMAMap() const {return const_cast<Material*>(this)->GetRMAMap();}
TextureInfo *Material::GetRMAMap() {return m_texRma;}

AlphaMode Material::GetAlphaMode() const {return m_alphaMode;}
float Material::GetAlphaCutoff() const
{
	auto &data = GetDataBlock();
	return data->GetFloat("alpha_cutoff",0.5f);
}

void Material::SetColorFactor(const Vector4 &colorFactor)
{
	auto &data = GetDataBlock();
	data->AddValue("vector4","color_factor",std::to_string(colorFactor.r) +' ' +std::to_string(colorFactor.g) +' ' +std::to_string(colorFactor.b) +' ' +std::to_string(colorFactor.a));
}
Vector4 Material::GetColorFactor() const
{
	auto &data = GetDataBlock();
	auto colFactor = data->GetValue("color_factor");
	if(colFactor == nullptr || typeid(*colFactor) != typeid(ds::Vector4))
		return {1.f,1.f,1.f,1.f};
	return static_cast<ds::Vector4&>(*colFactor).GetValue();
}
void Material::SetBloomColorFactor(const Vector4 &bloomColorFactor)
{
	auto &data = GetDataBlock();
	data->AddValue("vector4","bloom_color_factor",std::to_string(bloomColorFactor.r) +' ' +std::to_string(bloomColorFactor.g) +' ' +std::to_string(bloomColorFactor.b) +' ' +std::to_string(bloomColorFactor.a));
}
std::optional<Vector4> Material::GetBloomColorFactor() const
{
	auto &data = GetDataBlock();
	auto colFactor = data->GetValue("bloom_color_factor");
	if(colFactor == nullptr || typeid(*colFactor) != typeid(ds::Vector4))
		return {};
	return static_cast<ds::Vector4&>(*colFactor).GetValue();
}

void Material::SetName(const std::string &name) {m_name = name;}
const std::string &Material::GetName() {return m_name;}

bool Material::IsError() const {return umath::is_flag_set(m_stateFlags,StateFlags::Error);}
void Material::SetErrorFlag(bool set) {umath::set_flag(m_stateFlags,StateFlags::Error,set);}

const util::ShaderInfo *Material::GetShaderInfo() const {return m_shaderInfo.get();}
const std::string &Material::GetShaderIdentifier() const
{
	if(m_shaderInfo.expired() == false)
		return m_shaderInfo.get()->GetIdentifier();
	static std::string empty;
	return m_shader ? *m_shader : empty;
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

std::ostream &operator<<(std::ostream &out,const Material &o)
{
	out<<"Material";
	out<<"[Index:"<<o.GetIndex()<<"]";
	out<<"[Name:"<<const_cast<Material&>(o).GetName()<<"]";
	out<<"[Shader:"<<o.GetShaderIdentifier()<<"]";
	out<<"[AlphaMode:"<<magic_enum::enum_name(o.GetAlphaMode())<<"]";
	out<<"[AlphaCutoff:"<<o.GetAlphaCutoff()<<"]";
	out<<"[ColorFactor:"<<o.GetColorFactor()<<"]";
	out<<"[Error:"<<o.IsError()<<"]";
	out<<"[Loaded:"<<o.IsLoaded()<<"]";
	return out;
}
