// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;



#include "sharedutils/magic_enum.hpp"
#include <cassert>

module pragma.materialsystem;

import :material;
import :material_manager;

#undef CreateFile

decltype(msys::Material::DIFFUSE_MAP_IDENTIFIER) msys::Material::DIFFUSE_MAP_IDENTIFIER = "diffuse_map";
decltype(msys::Material::ALBEDO_MAP_IDENTIFIER) msys::Material::ALBEDO_MAP_IDENTIFIER = "albedo_map";
decltype(msys::Material::ALBEDO_MAP2_IDENTIFIER) msys::Material::ALBEDO_MAP2_IDENTIFIER = "albedo_map2";
decltype(msys::Material::ALBEDO_MAP3_IDENTIFIER) msys::Material::ALBEDO_MAP3_IDENTIFIER = "albedo_map3";
decltype(msys::Material::NORMAL_MAP_IDENTIFIER) msys::Material::NORMAL_MAP_IDENTIFIER = "normal_map";
decltype(msys::Material::GLOW_MAP_IDENTIFIER) msys::Material::GLOW_MAP_IDENTIFIER = "emission_map";
decltype(msys::Material::EMISSION_MAP_IDENTIFIER) msys::Material::EMISSION_MAP_IDENTIFIER = GLOW_MAP_IDENTIFIER;
decltype(msys::Material::PARALLAX_MAP_IDENTIFIER) msys::Material::PARALLAX_MAP_IDENTIFIER = "parallax_map";
decltype(msys::Material::ALPHA_MAP_IDENTIFIER) msys::Material::ALPHA_MAP_IDENTIFIER = "alpha_map";
decltype(msys::Material::RMA_MAP_IDENTIFIER) msys::Material::RMA_MAP_IDENTIFIER = "rma_map";
decltype(msys::Material::DUDV_MAP_IDENTIFIER) msys::Material::DUDV_MAP_IDENTIFIER = "dudv_map";
decltype(msys::Material::WRINKLE_STRETCH_MAP_IDENTIFIER) msys::Material::WRINKLE_STRETCH_MAP_IDENTIFIER = "wrinkle_stretch_map";
decltype(msys::Material::WRINKLE_COMPRESS_MAP_IDENTIFIER) msys::Material::WRINKLE_COMPRESS_MAP_IDENTIFIER = "wrinkle_compress_map";
decltype(msys::Material::EXPONENT_MAP_IDENTIFIER) msys::Material::EXPONENT_MAP_IDENTIFIER = "exponent_map";

msys::Material::BaseMaterial::~BaseMaterial()
{
	if(onBaseTexturesUpdated.IsValid())
		onBaseTexturesUpdated.Remove();
}

std::shared_ptr<msys::Material> msys::Material::Create(msys::MaterialManager &manager)
{
	auto mat = std::shared_ptr<msys::Material> {new Material {manager}};
	mat->Reset();
	return mat;
}
std::shared_ptr<msys::Material> msys::Material::Create(msys::MaterialManager &manager, const util::WeakHandle<util::ShaderInfo> &shaderInfo, const std::shared_ptr<ds::Block> &data)
{
	auto mat = std::shared_ptr<msys::Material> {new Material {manager, shaderInfo, data}};
	mat->Initialize(shaderInfo, data);
	return mat;
}
std::shared_ptr<msys::Material> msys::Material::Create(msys::MaterialManager &manager, const std::string &shader, const std::shared_ptr<ds::Block> &data)
{
	auto mat = std::shared_ptr<msys::Material> {new Material {manager, shader, data}};
	mat->Initialize(shader, data);
	return mat;
}
msys::Material::Material(msys::MaterialManager &manager)
	: m_data(nullptr), m_shader(nullptr), m_manager(manager),
	m_handle {util::to_shared_handle(shared_from_this())}
{}

msys::Material::Material(msys::MaterialManager &manager, const util::WeakHandle<util::ShaderInfo> &shaderInfo, const std::shared_ptr<ds::Block> &data) : Material(manager) {}

msys::Material::Material(msys::MaterialManager &manager, const std::string &shader, const std::shared_ptr<ds::Block> &data) : Material(manager) {}

void msys::Material::Assign(const Material &other)
{
	Reset();

	m_stateFlags = StateFlags::Loaded;
	if(umath::is_flag_set(other.m_stateFlags, StateFlags::Error))
		m_stateFlags |= StateFlags::Error;
	m_data = other.m_data;
	m_shaderInfo = other.m_shaderInfo;
	m_shader = other.m_shader ? std::make_unique<std::string>(*other.m_shader) : nullptr;
	m_baseMaterial = m_baseMaterial ? std::make_unique<BaseMaterial>(*m_baseMaterial) : nullptr;
	// m_index = other.m_index;

	if(IsValid())
		UpdateTextures();
}

void msys::Material::Reset()
{
	umath::set_flag(m_stateFlags, StateFlags::Loaded, false);
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
	m_baseMaterial = nullptr;
}

void msys::Material::Initialize(const util::WeakHandle<util::ShaderInfo> &shaderInfo, const std::shared_ptr<ds::Block> &data)
{
	Reset();
	SetShaderInfo(shaderInfo);
	m_data = data;
	Initialize(m_data);
}

void msys::Material::Initialize(const std::string &shader, const std::shared_ptr<ds::Block> &data)
{
	Reset();
	m_shader = std::make_unique<std::string>(shader);
	m_data = data;
	Initialize(m_data);
}

void msys::Material::Initialize(const std::shared_ptr<ds::Block> &data)
{
	m_baseMaterial = {};
	std::string baseMaterial;
	if(data->GetString("base_material", &baseMaterial))
		SetBaseMaterial(baseMaterial);
}

void *msys::Material::GetUserData() { return m_userData; }
void msys::Material::SetUserData(void *data) { m_userData = data; }

bool msys::Material::IsTranslucent() const { return m_alphaMode == AlphaMode::Blend; }

void msys::Material::UpdateTextures(bool forceUpdate)
{
	if(forceUpdate)
		umath::set_flag(m_stateFlags, StateFlags::TexturesUpdated, false);
	if(umath::is_flag_set(m_stateFlags, StateFlags::TexturesUpdated))
		return;
	GetTextureInfo(DIFFUSE_MAP_IDENTIFIER);

	// The call above may have already invoked UpdateTextures(), so we
	// need to re-check
	if(umath::is_flag_set(m_stateFlags, StateFlags::TexturesUpdated))
		return;
	umath::set_flag(m_stateFlags, StateFlags::TexturesUpdated);

	m_texDiffuse = GetTextureInfo(DIFFUSE_MAP_IDENTIFIER);
	if(!m_texDiffuse)
		m_texDiffuse = GetTextureInfo(ALBEDO_MAP_IDENTIFIER);

	m_texNormal = GetTextureInfo(NORMAL_MAP_IDENTIFIER);
	m_texGlow = GetTextureInfo(EMISSION_MAP_IDENTIFIER);
	m_texParallax = GetTextureInfo(PARALLAX_MAP_IDENTIFIER);
	m_texAlpha = GetTextureInfo(ALPHA_MAP_IDENTIFIER);
	m_texRma = GetTextureInfo(RMA_MAP_IDENTIFIER);

	m_alphaMode = GetProperty<AlphaMode>("alpha_mode", AlphaMode::Opaque);

	++m_updateIndex;
	OnTexturesUpdated();
}

void msys::Material::CallEventListeners(Event event)
{
	auto itLs = m_eventListeners.find(event);
	if(itLs == m_eventListeners.end())
		return;
	auto &listeners = itLs->second;
	for(auto it = listeners.begin(); it != listeners.end();) {
		auto &listener = *it;
		if(listener.IsValid() == false) {
			it = listeners.erase(it);
			continue;
		}
		listener();
		++it;
	}
}

CallbackHandle msys::Material::AddEventListener(Event event, const std::function<void(void)> &f)
{
	auto it = m_eventListeners.find(event);
	if(it == m_eventListeners.end())
		it = m_eventListeners.emplace(event, std::vector<CallbackHandle> {}).first;
	auto &vec = it->second;
	vec.emplace_back(FunctionCallback<void>::Create(f));
	return vec.back();
}

void msys::Material::OnTexturesUpdated() { CallEventListeners(Event::OnTexturesUpdated); }

void msys::Material::SetShaderInfo(const util::WeakHandle<util::ShaderInfo> &shaderInfo)
{
	m_shaderInfo = shaderInfo;
	m_shader = nullptr;
}

msys::Material::~Material()
{
	for(auto &hCb : m_callOnLoaded) {
		if(hCb.IsValid() == true)
			hCb.Remove();
	}
}

bool msys::Material::IsValid() const { return (m_data != nullptr) ? true : false; }
msys::MaterialManager &msys::Material::GetManager() const { return m_manager; }
void msys::Material::SetLoaded(bool b)
{
	if(umath::is_flag_set(m_stateFlags, StateFlags::ExecutingOnLoadCallbacks))
		return; // Prevent possible recursion while on-load callbacks are being executed
	umath::set_flag(m_stateFlags, StateFlags::Loaded, b);
	if(b == true) {
		umath::set_flag(m_stateFlags, StateFlags::ExecutingOnLoadCallbacks, true);
		for(auto &f : m_callOnLoaded) {
			if(f.IsValid())
				f();
		}
		m_callOnLoaded.clear();
		umath::set_flag(m_stateFlags, StateFlags::ExecutingOnLoadCallbacks, false);
	}
}
bool msys::Material::Save(udm::AssetData outData, std::string &outErr)
{
	outData.GetData().Add(GetShaderIdentifier());
	auto udm = (*outData)[GetShaderIdentifier()];
	std::function<void(udm::LinkedPropertyWrapper, ds::Block &)> dataBlockToUdm = nullptr;
	dataBlockToUdm = [&dataBlockToUdm, &udm](udm::LinkedPropertyWrapper prop, ds::Block &block) {
		for(auto &pair : *block.GetData()) {
			auto &key = pair.first;
			auto &val = pair.second;
			if(val->IsBlock()) {
				auto &block = static_cast<ds::Block &>(*pair.second);
				dataBlockToUdm(prop[key], block);
				continue;
			}
			if(val->IsContainer()) {
				auto &container = static_cast<ds::Container &>(*pair.second);
				auto &children = container.GetBlocks();
				auto udmChildren = prop.AddArray(key, children.size());
				uint32_t idx = 0;
				for(auto &child : children) {
					if(child->IsContainer() || child->IsBlock())
						continue;
					auto *dsValue = dynamic_cast<ds::Value *>(pair.second.get());
					if(dsValue == nullptr)
						continue;
					udmChildren[idx++] = dsValue->GetString();
				}
				udmChildren.Resize(idx);
				continue;
			}
			auto *dsValue = dynamic_cast<ds::Value *>(val.get());
			assert(dsValue);
			if(dsValue) {
				auto *dsStr = dynamic_cast<ds::String *>(dsValue);
				if(dsStr)
					prop[key] = dsStr->GetString();
				auto *dsInt = dynamic_cast<ds::Int *>(dsValue);
				if(dsInt)
					prop[key] = dsInt->GetInt();
				auto *dsFloat = dynamic_cast<ds::Float *>(dsValue);
				if(dsFloat)
					prop[key] = dsFloat->GetFloat();
				auto *dsBool = dynamic_cast<ds::Bool *>(dsValue);
				if(dsBool)
					prop[key] = dsBool->GetBool();
				auto *dsVec = dynamic_cast<ds::Vector *>(dsValue);
				if(dsVec)
					prop[key] = dsVec->GetVector();
				auto *dsVec4 = dynamic_cast<ds::Vector4 *>(dsValue);
				if(dsVec4)
					prop[key] = dsVec4->GetVector4();
				auto *dsVec2 = dynamic_cast<ds::Vector2 *>(dsValue);
				if(dsVec2)
					prop[key] = dsVec2->GetVector2();
				auto *dsTex = dynamic_cast<ds::Texture *>(dsValue);
				if(dsTex)
					udm["textures"][key] = dsTex->GetString();
				auto *dsCol = dynamic_cast<ds::Color *>(dsValue);
				if(dsCol)
					prop[key] = dsCol->GetColor().ToVector4();
			}
		}
	};

	outData.SetAssetType(PMAT_IDENTIFIER);
	outData.SetAssetVersion(PMAT_VERSION);
	dataBlockToUdm(udm["properties"], *m_data);
	if(m_baseMaterial)
		udm["base_material"] = m_baseMaterial->name;
	return true;
}
extern const std::array<std::string, 5> g_knownMaterialFormats;
bool msys::Material::Save(const std::string &relFileName, std::string &outErr, bool absolutePath)
{
	auto udmData = udm::Data::Create();
	std::string err;
	auto result = Save(udmData->GetAssetData(), err);
	if(result == false)
		return false;
	auto fileName = relFileName;
	if(absolutePath == false) {
		auto assetFilePath = GetManager().FindAssetFilePath(fileName);
		if(assetFilePath.has_value())
			fileName = std::move(*assetFilePath);
		fileName = "materials/" + fileName;
	}

	std::string existingFilePath;
	// If material already exists (e.g. inside an addon), make sure we have the correct path to overwrite it
	if(FileManager::FindLocalPath(fileName, existingFilePath))
		fileName = std::move(existingFilePath);

	std::string ext;
	auto binary = false;
	if(ufile::get_extension(fileName, &ext) && ustring::compare(ext.c_str(), FORMAT_MATERIAL_BINARY, false))
		binary = true;

	ufile::remove_extension_from_filename(fileName, g_knownMaterialFormats);
	if(binary)
		fileName += '.' + std::string {FORMAT_MATERIAL_BINARY};
	else
		fileName += '.' + std::string {FORMAT_MATERIAL_ASCII};

	FileManager::CreatePath(ufile::get_path_from_filename(fileName).c_str());
	auto f = FileManager::OpenFile<VFilePtrReal>(fileName.c_str(), binary ? "wb" : "w");
	if(f == nullptr) {
		outErr = "Unable to open file '" + fileName + "'!";
		return false;
	}
	if(binary)
		result = udmData->Save(f);
	else
		result = udmData->SaveAscii(f, udm::AsciiSaveFlags::None);
	if(result == false) {
		outErr = "Unable to save UDM data!";
		return false;
	}
	return true;
}
bool msys::Material::Save(std::string &outErr)
{
	auto mdlName = GetName();
	std::string absFileName;
	auto result = FileManager::FindAbsolutePath("materials/" + mdlName, absFileName);
	auto absolutePath = false;
	if(result == false)
		absFileName = mdlName;
	else {
		if(!filemanager::find_relative_path(absFileName, absFileName)) {
			outErr = "Unable to determine relative path!";
			return false;
		}
		absolutePath = true;
	}
	return Save(absFileName, outErr, absolutePath);
}
bool msys::Material::SaveLegacy(std::shared_ptr<VFilePtrInternalReal> f) const
{
	std::stringstream ss;
	ss << m_data->ToString(GetShaderIdentifier());

	f->WriteString(ss.str());
	return true;
}
std::optional<std::string> msys::Material::GetAbsolutePath() const
{
	auto name = const_cast<Material *>(this)->GetName();
	if(name.empty())
		return {};
	std::string absPath = GetManager().GetRootDirectory().GetString() + "\\";
	absPath += name;
	ufile::remove_extension_from_filename(absPath, g_knownMaterialFormats);
	absPath += ".wmi";
	if(FileManager::FindLocalPath(absPath, absPath) == false)
		return {};
	return absPath;
}
bool msys::Material::SaveLegacy() const
{
	auto name = const_cast<Material *>(this)->GetName();
	if(name.empty())
		return false;
	std::string absPath = GetManager().GetRootDirectory().GetString() + "\\";
	absPath += name;
	ufile::remove_extension_from_filename(absPath, g_knownMaterialFormats);
	absPath += ".wmi";
	if(FileManager::FindLocalPath(absPath, absPath) == false)
		absPath = "addons/converted/" + absPath;

	auto f = FileManager::OpenFile<VFilePtrReal>(absPath.c_str(), "w");
	if(f == nullptr)
		return false;
	return SaveLegacy(f);
}
bool msys::Material::SaveLegacy(const std::string &fileName, const std::string &inRootPath) const
{
	auto rootPath = inRootPath;
	if(rootPath.empty() == false && rootPath.back() != '/' && rootPath.back() != '\\')
		rootPath += '/';
	auto fullPath = rootPath + ::MaterialManager::GetRootMaterialLocation() + "/" + fileName;
	ufile::remove_extension_from_filename(fullPath, g_knownMaterialFormats);
	fullPath += ".wmi";

	auto pathWithoutFileName = ufile::get_path_from_filename(fullPath);
	FileManager::CreatePath(pathWithoutFileName.c_str());

	auto f = FileManager::OpenFile<VFilePtrReal>(fullPath.c_str(), "w");
	if(f == nullptr)
		return false;
	return SaveLegacy(f);
}
CallbackHandle msys::Material::CallOnLoaded(const std::function<void(void)> &f) const
{
	if(IsLoaded()) {
		f();
		return {};
	}
	m_callOnLoaded.push_back(FunctionCallback<>::Create(f));
	return m_callOnLoaded.back();
}
bool msys::Material::IsLoaded() const { return umath::is_flag_set(m_stateFlags, StateFlags::Loaded); }

const TextureInfo *msys::Material::GetDiffuseMap() const { return const_cast<Material *>(this)->GetDiffuseMap(); }
TextureInfo *msys::Material::GetDiffuseMap()
{
	UpdateTextures();
	return m_texDiffuse;
}

const TextureInfo *msys::Material::GetAlbedoMap() const { return GetDiffuseMap(); }
TextureInfo *msys::Material::GetAlbedoMap() { return GetDiffuseMap(); }

const TextureInfo *msys::Material::GetNormalMap() const { return const_cast<Material *>(this)->GetNormalMap(); }
TextureInfo *msys::Material::GetNormalMap()
{
	UpdateTextures();
	return m_texNormal;
}

const TextureInfo *msys::Material::GetGlowMap() const { return const_cast<Material *>(this)->GetGlowMap(); }
TextureInfo *msys::Material::GetGlowMap()
{
	UpdateTextures();
	return m_texGlow;
}

const TextureInfo *msys::Material::GetAlphaMap() const { return const_cast<Material *>(this)->GetAlphaMap(); }
TextureInfo *msys::Material::GetAlphaMap()
{
	UpdateTextures();
	return m_texAlpha;
}

const TextureInfo *msys::Material::GetParallaxMap() const { return const_cast<Material *>(this)->GetParallaxMap(); }
TextureInfo *msys::Material::GetParallaxMap()
{
	UpdateTextures();
	return m_texParallax;
}

const TextureInfo *msys::Material::GetRMAMap() const { return const_cast<Material *>(this)->GetRMAMap(); }
TextureInfo *msys::Material::GetRMAMap()
{
	UpdateTextures();
	return m_texRma;
}

AlphaMode msys::Material::GetAlphaMode() const { return m_alphaMode; }
float msys::Material::GetAlphaCutoff() const { return GetProperty<float>("alpha_cutoff", 0.5f); }

bool msys::Material::HasPropertyBlock(const std::string_view &name) const { return GetPropertyBlock(name) != nullptr; }

std::shared_ptr<ds::Block> msys::Material::GetPropertyBlock(const std::string_view &name) const
{
	if(name.empty())
		return m_data;
	auto path = util::FilePath(name);
	auto data = m_data;
	for(auto &segment : path) {
		data = data->GetBlock(std::string {segment});
		if(data == nullptr)
			return nullptr;
	}
	return data;
}

msys::PropertyType msys::Material::GetPropertyType(const std::string_view &key) const
{
	auto dsVal = m_data->GetDataValue(std::string {key});
	if(!dsVal) {
		auto *baseMaterial = GetBaseMaterial();
		if(baseMaterial)
			return baseMaterial->GetPropertyType(key);
		return msys::PropertyType::None;
	}
	if(dsVal->IsBlock())
		return msys::PropertyType::Block;
	if(typeid(*dsVal) == typeid(ds::Texture))
		return msys::PropertyType::Texture;
	return msys::PropertyType::Value;
}

void msys::Material::ClearProperty(ds::Block &block, const std::string_view &key, bool clearBlocksIfEmpty)
{
	if(key.find_first_of('/') != std::string::npos) {
		auto path = util::FilePath(key);
		auto data = std::static_pointer_cast<ds::Block>(block.shared_from_this());
		auto parentBlock = data;
		std::shared_ptr<ds::Block> secondToLastBlock;
		std::string_view lastSegment;
		for(auto &segment : path) {
			if(data == nullptr)
				return;
			secondToLastBlock = data;
			parentBlock = data;
			data = data->GetBlock(std::string {segment});
			lastSegment = segment;
		}
		if(secondToLastBlock) {
			ClearProperty(*secondToLastBlock, std::string {lastSegment}, clearBlocksIfEmpty);
			if(clearBlocksIfEmpty && secondToLastBlock->IsEmpty())
				parentBlock->RemoveValue(std::string {lastSegment});
		}
		return;
	}
	block.RemoveValue(std::string {key});
}
void msys::Material::ClearProperty(const std::string_view &key, bool clearBlocksIfEmpty) { ClearProperty(*m_data, key, clearBlocksIfEmpty); }
ds::ValueType msys::Material::GetPropertyValueType(const std::string_view &strPath) const
{
	auto [block, key] = ResolvePropertyPath(strPath);
	if(block == nullptr)
		return ds::ValueType::Invalid; // No path segments
	auto &dsVal = block->GetValue(key);
	if(dsVal && dsVal->IsValue())
		return static_cast<ds::Value &>(*dsVal).GetType();
	auto *baseMaterial = GetBaseMaterial();
	if(baseMaterial)
		return baseMaterial->GetPropertyValueType(strPath);
	return ds::ValueType::Invalid;
}
void msys::Material::SetTextureProperty(const std::string_view &strPath, const std::string_view &tex)
{
	auto [block, key] = ResolvePropertyPath(strPath);
	if(block == nullptr)
		return;
	block->AddValue("texture", std::string {key}, std::string {tex});
}

std::pair<std::shared_ptr<ds::Block>, std::string> msys::Material::ResolvePropertyPath(const std::string_view &strPath) const
{
	if(strPath.find('/') == std::string::npos)
		return {m_data, std::string {strPath}}; // Fast exit if no path segments
	auto path = util::FilePath(strPath);
	auto data = m_data;
	std::shared_ptr<ds::Block> secondToLastBlock;
	std::string_view lastSegment;
	for(auto &segment : path) {
		if(data == nullptr)
			return {};
		secondToLastBlock = data;
		data = data->GetBlock(std::string {segment});
		lastSegment = segment;
	}
	if(!secondToLastBlock)
		return {m_data, std::string {strPath}}; // No path segments
	return {secondToLastBlock, std::string {lastSegment}};
}

void msys::Material::SetColorFactor(const Vector4 &colorFactor) { SetProperty("color_factor", colorFactor); }
Vector4 msys::Material::GetColorFactor() const { return GetProperty<Vector4>("color_factor", {1.f, 1.f, 1.f, 1.f}); }
void msys::Material::SetBloomColorFactor(const Vector4 &bloomColorFactor) { SetProperty("bloom_color_factor", bloomColorFactor); }
std::optional<Vector4> msys::Material::GetBloomColorFactor() const
{
	Vector4 bloomColor;
	if(!GetProperty<Vector4>("bloom_color_factor", &bloomColor))
		return {};
	return bloomColor;
}

void msys::Material::SetName(const std::string &name) { m_name = name; }
const std::string &msys::Material::GetName() { return m_name; }

bool msys::Material::IsError() const { return umath::is_flag_set(m_stateFlags, StateFlags::Error); }
void msys::Material::SetErrorFlag(bool set) { umath::set_flag(m_stateFlags, StateFlags::Error, set); }

const util::ShaderInfo *msys::Material::GetShaderInfo() const { return m_shaderInfo.get(); }
const std::string &msys::Material::GetShaderIdentifier() const
{
	if(m_shaderInfo.expired() == false)
		return m_shaderInfo.get()->GetIdentifier();
	static std::string empty;
	return m_shader ? *m_shader : empty;
}

const msys::Material *msys::Material::GetBaseMaterial() const { return const_cast<Material *>(this)->GetBaseMaterial(); }
void msys::Material::UpdateBaseMaterial()
{
	if(m_baseMaterial->material)
		return;
	auto baseMat = m_manager.LoadAsset(m_baseMaterial->name);
	if(baseMat) {
		m_baseMaterial->material = baseMat->shared_from_this();
		m_baseMaterial->onBaseTexturesUpdated = baseMat->AddEventListener(Event::OnTexturesUpdated, [this]() { OnBaseMaterialChanged(); });
	}
}
msys::Material *msys::Material::GetBaseMaterial()
{
	if(!m_baseMaterial)
		return nullptr;
	UpdateBaseMaterial();
	return m_baseMaterial ? m_baseMaterial->material.get() : nullptr;
}
void msys::Material::SetBaseMaterial(const std::string &baseMaterial)
{
	m_baseMaterial = std::make_unique<BaseMaterial>();
	m_baseMaterial->name = baseMaterial;
	m_manager.PreloadAsset(m_baseMaterial->name);
}
void msys::Material::SetBaseMaterial(Material *baseMaterial)
{
	m_baseMaterial = nullptr;
	if(!baseMaterial)
		return;
	m_baseMaterial = std::make_unique<BaseMaterial>();
	m_baseMaterial->material = baseMaterial->shared_from_this();
	m_baseMaterial->onBaseTexturesUpdated = baseMaterial->AddEventListener(Event::OnTexturesUpdated, [this]() { OnBaseMaterialChanged(); });
	m_baseMaterial->name = baseMaterial->GetName();
}

void msys::Material::OnBaseMaterialChanged() {}

const TextureInfo *msys::Material::GetTextureInfo(const std::string_view &key) const { return const_cast<Material *>(this)->GetTextureInfo(key); }

TextureInfo *msys::Material::GetTextureInfo(const std::string_view &key)
{
	TextureInfo *texInfo = nullptr;
	if(m_data) {
		auto &base = m_data->GetValue(key);
		if(base != nullptr && !base->IsBlock()) {
			auto &val = static_cast<ds::Value &>(*base);
			auto &type = typeid(val);
			if(type == typeid(ds::Texture)) {
				auto &datTex = static_cast<ds::Texture &>(val);
				return &const_cast<TextureInfo &>(datTex.GetValue());
			}
		}
	}
	if(m_baseMaterial && m_baseMaterial->material)
		return m_baseMaterial->material->GetTextureInfo(key);
	return nullptr;
}

msys::MaterialHandle msys::Material::GetHandle() { return m_handle; }

std::shared_ptr<msys::Material> msys::Material::Copy(bool copyData) const
{
	std::shared_ptr<msys::Material> r = nullptr;
	if(!IsValid())
		r = GetManager().CreateMaterial("pbr", nullptr);
	else if(m_shaderInfo.expired() == false) {
		auto data = copyData ? std::shared_ptr<ds::Block>(m_data->Copy()) : std::make_shared<ds::Block>(m_data->GetDataSettings());
		r = GetManager().CreateMaterial(m_shaderInfo->GetIdentifier(), data);
	}
	else {
		auto data = copyData ? std::shared_ptr<ds::Block>(m_data->Copy()) : std::make_shared<ds::Block>(m_data->GetDataSettings());
		r = GetManager().CreateMaterial(*m_shader, data);
	}
	if(IsLoaded())
		r->SetLoaded(true);
	r->m_stateFlags = m_stateFlags;
	umath::set_flag(r->m_stateFlags, StateFlags::TexturesUpdated, false);
	return r;
}

std::ostream &operator<<(std::ostream &out, const msys::Material &o)
{
	out << "Material";
	out << "[Index:" << o.GetIndex() << "]";
	out << "[Name:" << const_cast<msys::Material &>(o).GetName() << "]";
	out << "[Shader:" << o.GetShaderIdentifier() << "]";
	out << "[AlphaMode:" << magic_enum::enum_name(o.GetAlphaMode()) << "]";
	out << "[AlphaCutoff:" << o.GetAlphaCutoff() << "]";
	out << "[ColorFactor:" << o.GetColorFactor() << "]";
	out << "[Error:" << o.IsError() << "]";
	out << "[Loaded:" << o.IsLoaded() << "]";
	return out;
}
