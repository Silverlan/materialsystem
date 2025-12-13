// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <cassert>

module pragma.materialsystem;

import :material;
import :material_manager;

#undef CreateFile

decltype(pragma::material::ematerial::DIFFUSE_MAP_IDENTIFIER) pragma::material::ematerial::DIFFUSE_MAP_IDENTIFIER = "diffuse_map";
decltype(pragma::material::ematerial::ALBEDO_MAP_IDENTIFIER) pragma::material::ematerial::ALBEDO_MAP_IDENTIFIER = "albedo_map";
decltype(pragma::material::ematerial::ALBEDO_MAP2_IDENTIFIER) pragma::material::ematerial::ALBEDO_MAP2_IDENTIFIER = "albedo_map2";
decltype(pragma::material::ematerial::ALBEDO_MAP3_IDENTIFIER) pragma::material::ematerial::ALBEDO_MAP3_IDENTIFIER = "albedo_map3";
decltype(pragma::material::ematerial::NORMAL_MAP_IDENTIFIER) pragma::material::ematerial::NORMAL_MAP_IDENTIFIER = "normal_map";
decltype(pragma::material::ematerial::GLOW_MAP_IDENTIFIER) pragma::material::ematerial::GLOW_MAP_IDENTIFIER = "emission_map";
decltype(pragma::material::ematerial::EMISSION_MAP_IDENTIFIER) pragma::material::ematerial::EMISSION_MAP_IDENTIFIER = GLOW_MAP_IDENTIFIER;
decltype(pragma::material::ematerial::PARALLAX_MAP_IDENTIFIER) pragma::material::ematerial::PARALLAX_MAP_IDENTIFIER = "parallax_map";
decltype(pragma::material::ematerial::ALPHA_MAP_IDENTIFIER) pragma::material::ematerial::ALPHA_MAP_IDENTIFIER = "alpha_map";
decltype(pragma::material::ematerial::RMA_MAP_IDENTIFIER) pragma::material::ematerial::RMA_MAP_IDENTIFIER = "rma_map";
decltype(pragma::material::ematerial::DUDV_MAP_IDENTIFIER) pragma::material::ematerial::DUDV_MAP_IDENTIFIER = "dudv_map";
decltype(pragma::material::ematerial::WRINKLE_STRETCH_MAP_IDENTIFIER) pragma::material::ematerial::WRINKLE_STRETCH_MAP_IDENTIFIER = "wrinkle_stretch_map";
decltype(pragma::material::ematerial::WRINKLE_COMPRESS_MAP_IDENTIFIER) pragma::material::ematerial::WRINKLE_COMPRESS_MAP_IDENTIFIER = "wrinkle_compress_map";
decltype(pragma::material::ematerial::EXPONENT_MAP_IDENTIFIER) pragma::material::ematerial::EXPONENT_MAP_IDENTIFIER = "exponent_map";

pragma::material::Material::BaseMaterial::~BaseMaterial()
{
	if(onBaseTexturesUpdated.IsValid())
		onBaseTexturesUpdated.Remove();
}

std::shared_ptr<pragma::material::Material> pragma::material::Material::Create(MaterialManager &manager)
{
	auto mat = std::shared_ptr<Material> {new Material {manager}};
	mat->Reset();
	return mat;
}
std::shared_ptr<pragma::material::Material> pragma::material::Material::Create(MaterialManager &manager, const pragma::util::WeakHandle<pragma::util::ShaderInfo> &shaderInfo, const std::shared_ptr<datasystem::Block> &data)
{
	auto mat = std::shared_ptr<Material> {new Material {manager, shaderInfo, data}};
	mat->Initialize(shaderInfo, data);
	return mat;
}
std::shared_ptr<pragma::material::Material> pragma::material::Material::Create(MaterialManager &manager, const std::string &shader, const std::shared_ptr<datasystem::Block> &data)
{
	auto mat = std::shared_ptr<Material> {new Material {manager, shader, data}};
	mat->Initialize(shader, data);
	return mat;
}
pragma::material::Material::Material(MaterialManager &manager) : m_data(nullptr), m_shader(nullptr), m_manager(manager) {}

pragma::material::Material::Material(MaterialManager &manager, const pragma::util::WeakHandle<pragma::util::ShaderInfo> &shaderInfo, const std::shared_ptr<datasystem::Block> &data) : Material(manager) {}

pragma::material::Material::Material(MaterialManager &manager, const std::string &shader, const std::shared_ptr<datasystem::Block> &data) : Material(manager) {}

void pragma::material::Material::Assign(const Material &other)
{
	Reset();

	m_stateFlags = StateFlags::Loaded;
	if(pragma::math::is_flag_set(other.m_stateFlags, StateFlags::Error))
		m_stateFlags |= StateFlags::Error;
	m_data = other.m_data;
	m_shaderInfo = other.m_shaderInfo;
	m_shader = other.m_shader ? std::make_unique<std::string>(*other.m_shader) : nullptr;
	m_baseMaterial = m_baseMaterial ? std::make_unique<BaseMaterial>(*m_baseMaterial) : nullptr;
	// m_index = other.m_index;

	if(IsValid())
		UpdateTextures();
}

void pragma::material::Material::Reset()
{
	pragma::math::set_flag(m_stateFlags, StateFlags::Loaded, false);
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

void pragma::material::Material::Initialize(const pragma::util::WeakHandle<pragma::util::ShaderInfo> &shaderInfo, const std::shared_ptr<datasystem::Block> &data)
{
	Reset();
	SetShaderInfo(shaderInfo);
	m_data = data;
	Initialize(m_data);
}

void pragma::material::Material::Initialize(const std::string &shader, const std::shared_ptr<datasystem::Block> &data)
{
	Reset();
	m_shader = std::make_unique<std::string>(shader);
	m_data = data;
	Initialize(m_data);
}

void pragma::material::Material::Initialize(const std::shared_ptr<datasystem::Block> &data)
{
	m_baseMaterial = {};
	std::string baseMaterial;
	if(data->GetString("base_material", &baseMaterial))
		SetBaseMaterial(baseMaterial);
}

void *pragma::material::Material::GetUserData() { return m_userData; }
void pragma::material::Material::SetUserData(void *data) { m_userData = data; }

bool pragma::material::Material::IsTranslucent() const { return m_alphaMode == AlphaMode::Blend; }

void pragma::material::Material::UpdateTextures(bool forceUpdate)
{
	if(forceUpdate)
		pragma::math::set_flag(m_stateFlags, StateFlags::TexturesUpdated, false);
	if(pragma::math::is_flag_set(m_stateFlags, StateFlags::TexturesUpdated))
		return;
	GetTextureInfo(ematerial::DIFFUSE_MAP_IDENTIFIER);

	// The call above may have already invoked UpdateTextures(), so we
	// need to re-check
	if(pragma::math::is_flag_set(m_stateFlags, StateFlags::TexturesUpdated))
		return;
	pragma::math::set_flag(m_stateFlags, StateFlags::TexturesUpdated);

	m_texDiffuse = GetTextureInfo(ematerial::DIFFUSE_MAP_IDENTIFIER);
	if(!m_texDiffuse)
		m_texDiffuse = GetTextureInfo(ematerial::ALBEDO_MAP_IDENTIFIER);

	m_texNormal = GetTextureInfo(ematerial::NORMAL_MAP_IDENTIFIER);
	m_texGlow = GetTextureInfo(ematerial::EMISSION_MAP_IDENTIFIER);
	m_texParallax = GetTextureInfo(ematerial::PARALLAX_MAP_IDENTIFIER);
	m_texAlpha = GetTextureInfo(ematerial::ALPHA_MAP_IDENTIFIER);
	m_texRma = GetTextureInfo(ematerial::RMA_MAP_IDENTIFIER);

	m_alphaMode = GetProperty<AlphaMode>("alpha_mode", AlphaMode::Opaque);

	++m_updateIndex;
	OnTexturesUpdated();
}

void pragma::material::Material::CallEventListeners(Event event)
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

CallbackHandle pragma::material::Material::AddEventListener(Event event, const std::function<void(void)> &f)
{
	auto it = m_eventListeners.find(event);
	if(it == m_eventListeners.end())
		it = m_eventListeners.emplace(event, std::vector<CallbackHandle> {}).first;
	auto &vec = it->second;
	vec.emplace_back(FunctionCallback<void>::Create(f));
	return vec.back();
}

void pragma::material::Material::OnTexturesUpdated() { CallEventListeners(Event::OnTexturesUpdated); }

void pragma::material::Material::SetShaderInfo(const pragma::util::WeakHandle<pragma::util::ShaderInfo> &shaderInfo)
{
	m_shaderInfo = shaderInfo;
	m_shader = nullptr;
}

pragma::material::Material::~Material()
{
	for(auto &hCb : m_callOnLoaded) {
		if(hCb.IsValid() == true)
			hCb.Remove();
	}
}

bool pragma::material::Material::IsValid() const { return (m_data != nullptr) ? true : false; }
pragma::material::MaterialManager &pragma::material::Material::GetManager() const { return m_manager; }
void pragma::material::Material::SetLoaded(bool b)
{
	if(pragma::math::is_flag_set(m_stateFlags, StateFlags::ExecutingOnLoadCallbacks))
		return; // Prevent possible recursion while on-load callbacks are being executed
	pragma::math::set_flag(m_stateFlags, StateFlags::Loaded, b);
	if(b == true) {
		pragma::math::set_flag(m_stateFlags, StateFlags::ExecutingOnLoadCallbacks, true);
		for(auto &f : m_callOnLoaded) {
			if(f.IsValid())
				f();
		}
		m_callOnLoaded.clear();
		pragma::math::set_flag(m_stateFlags, StateFlags::ExecutingOnLoadCallbacks, false);
	}
}
bool pragma::material::Material::Save(udm::AssetData outData, std::string &outErr)
{
	outData.GetData().Add(GetShaderIdentifier());
	auto udm = (*outData)[GetShaderIdentifier()];
	std::function<void(udm::LinkedPropertyWrapper, datasystem::Block &)> dataBlockToUdm = nullptr;
	dataBlockToUdm = [&dataBlockToUdm, &udm](udm::LinkedPropertyWrapper prop, datasystem::Block &block) {
		for(auto &pair : *block.GetData()) {
			auto &key = pair.first;
			auto &val = pair.second;
			if(val->IsBlock()) {
				auto &block = static_cast<datasystem::Block &>(*pair.second);
				dataBlockToUdm(prop[key], block);
				continue;
			}
			if(val->IsContainer()) {
				auto &container = static_cast<datasystem::Container &>(*pair.second);
				auto &children = container.GetBlocks();
				auto udmChildren = prop.AddArray(key, children.size());
				uint32_t idx = 0;
				for(auto &child : children) {
					if(child->IsContainer() || child->IsBlock())
						continue;
					auto *dsValue = dynamic_cast<datasystem::Value *>(pair.second.get());
					if(dsValue == nullptr)
						continue;
					udmChildren[idx++] = dsValue->GetString();
				}
				udmChildren.Resize(idx);
				continue;
			}
			auto *dsValue = dynamic_cast<datasystem::Value *>(val.get());
			assert(dsValue);
			if(dsValue) {
				auto *dsStr = dynamic_cast<datasystem::String *>(dsValue);
				if(dsStr)
					prop[key] = dsStr->GetString();
				auto *dsInt = dynamic_cast<datasystem::Int *>(dsValue);
				if(dsInt)
					prop[key] = dsInt->GetInt();
				auto *dsFloat = dynamic_cast<datasystem::Float *>(dsValue);
				if(dsFloat)
					prop[key] = dsFloat->GetFloat();
				auto *dsBool = dynamic_cast<datasystem::Bool *>(dsValue);
				if(dsBool)
					prop[key] = dsBool->GetBool();
				auto *dsVec = dynamic_cast<datasystem::Vector *>(dsValue);
				if(dsVec)
					prop[key] = dsVec->GetVector();
				auto *dsVec4 = dynamic_cast<datasystem::Vector4 *>(dsValue);
				if(dsVec4)
					prop[key] = dsVec4->GetVector4();
				auto *dsVec2 = dynamic_cast<datasystem::Vector2 *>(dsValue);
				if(dsVec2)
					prop[key] = dsVec2->GetVector2();
				auto *dsTex = dynamic_cast<datasystem::Texture *>(dsValue);
				if(dsTex)
					udm["textures"][key] = dsTex->GetString();
				auto *dsCol = dynamic_cast<datasystem::Color *>(dsValue);
				if(dsCol)
					prop[key] = dsCol->GetColor().ToVector4();
			}
		}
	};

	outData.SetAssetType(ematerial::PMAT_IDENTIFIER);
	outData.SetAssetVersion(ematerial::PMAT_VERSION);
	dataBlockToUdm(udm["properties"], *m_data);
	if(m_baseMaterial)
		udm["base_material"] = m_baseMaterial->name;
	return true;
}
extern const std::array<std::string, 5> g_knownMaterialFormats;
bool pragma::material::Material::Save(const std::string &relFileName, std::string &outErr, bool absolutePath)
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
	if(fs::find_local_path(fileName, existingFilePath))
		fileName = std::move(existingFilePath);

	std::string ext;
	auto binary = false;
	if(ufile::get_extension(fileName, &ext) && pragma::string::compare(ext.c_str(), ematerial::FORMAT_MATERIAL_BINARY, false))
		binary = true;

	ufile::remove_extension_from_filename(fileName, g_knownMaterialFormats);
	if(binary)
		fileName += '.' + std::string {ematerial::FORMAT_MATERIAL_BINARY};
	else
		fileName += '.' + std::string {ematerial::FORMAT_MATERIAL_ASCII};

	fs::create_path(ufile::get_path_from_filename(fileName));
	auto f = fs::open_file<fs::VFilePtrReal>(fileName, binary ? (fs::FileMode::Write | fs::FileMode::Binary) : fs::FileMode::Write);
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
bool pragma::material::Material::Save(std::string &outErr)
{
	auto mdlName = GetName();
	std::string absFileName;
	auto result = fs::find_absolute_path("materials/" + mdlName, absFileName);
	auto absolutePath = false;
	if(result == false)
		absFileName = mdlName;
	else {
		if(!fs::find_relative_path(absFileName, absFileName)) {
			outErr = "Unable to determine relative path!";
			return false;
		}
		absolutePath = true;
	}
	return Save(absFileName, outErr, absolutePath);
}
bool pragma::material::Material::SaveLegacy(std::shared_ptr<fs::VFilePtrInternalReal> f) const
{
	std::stringstream ss;
	ss << m_data->ToString(GetShaderIdentifier());

	f->WriteString(ss.str());
	return true;
}
std::optional<std::string> pragma::material::Material::GetAbsolutePath() const
{
	auto name = const_cast<Material *>(this)->GetName();
	if(name.empty())
		return {};
	std::string absPath = GetManager().GetRootDirectory().GetString() + "\\";
	absPath += name;
	ufile::remove_extension_from_filename(absPath, g_knownMaterialFormats);
	absPath += ".wmi";
	if(fs::find_local_path(absPath, absPath) == false)
		return {};
	return absPath;
}
bool pragma::material::Material::SaveLegacy() const
{
	auto name = const_cast<Material *>(this)->GetName();
	if(name.empty())
		return false;
	std::string absPath = GetManager().GetRootDirectory().GetString() + "\\";
	absPath += name;
	ufile::remove_extension_from_filename(absPath, g_knownMaterialFormats);
	absPath += ".wmi";
	if(fs::find_local_path(absPath, absPath) == false)
		absPath = "addons/converted/" + absPath;

	auto f = fs::open_file<fs::VFilePtrReal>(absPath, fs::FileMode::Write);
	if(f == nullptr)
		return false;
	return SaveLegacy(f);
}
bool pragma::material::Material::SaveLegacy(const std::string &fileName, const std::string &inRootPath) const
{
	auto rootPath = inRootPath;
	if(rootPath.empty() == false && rootPath.back() != '/' && rootPath.back() != '\\')
		rootPath += '/';
	auto fullPath = rootPath + ::MaterialManager::GetRootMaterialLocation() + "/" + fileName;
	ufile::remove_extension_from_filename(fullPath, g_knownMaterialFormats);
	fullPath += ".wmi";

	auto pathWithoutFileName = ufile::get_path_from_filename(fullPath);
	fs::create_path(pathWithoutFileName);

	auto f = fs::open_file<fs::VFilePtrReal>(fullPath, fs::FileMode::Write);
	if(f == nullptr)
		return false;
	return SaveLegacy(f);
}
CallbackHandle pragma::material::Material::CallOnLoaded(const std::function<void(void)> &f) const
{
	if(IsLoaded()) {
		f();
		return {};
	}
	m_callOnLoaded.push_back(FunctionCallback<>::Create(f));
	return m_callOnLoaded.back();
}
bool pragma::material::Material::IsLoaded() const { return pragma::math::is_flag_set(m_stateFlags, StateFlags::Loaded); }

const TextureInfo *pragma::material::Material::GetDiffuseMap() const { return const_cast<Material *>(this)->GetDiffuseMap(); }
TextureInfo *pragma::material::Material::GetDiffuseMap()
{
	UpdateTextures();
	return m_texDiffuse;
}

const TextureInfo *pragma::material::Material::GetAlbedoMap() const { return GetDiffuseMap(); }
TextureInfo *pragma::material::Material::GetAlbedoMap() { return GetDiffuseMap(); }

const TextureInfo *pragma::material::Material::GetNormalMap() const { return const_cast<Material *>(this)->GetNormalMap(); }
TextureInfo *pragma::material::Material::GetNormalMap()
{
	UpdateTextures();
	return m_texNormal;
}

const TextureInfo *pragma::material::Material::GetGlowMap() const { return const_cast<Material *>(this)->GetGlowMap(); }
TextureInfo *pragma::material::Material::GetGlowMap()
{
	UpdateTextures();
	return m_texGlow;
}

const TextureInfo *pragma::material::Material::GetAlphaMap() const { return const_cast<Material *>(this)->GetAlphaMap(); }
TextureInfo *pragma::material::Material::GetAlphaMap()
{
	UpdateTextures();
	return m_texAlpha;
}

const TextureInfo *pragma::material::Material::GetParallaxMap() const { return const_cast<Material *>(this)->GetParallaxMap(); }
TextureInfo *pragma::material::Material::GetParallaxMap()
{
	UpdateTextures();
	return m_texParallax;
}

const TextureInfo *pragma::material::Material::GetRMAMap() const { return const_cast<Material *>(this)->GetRMAMap(); }
TextureInfo *pragma::material::Material::GetRMAMap()
{
	UpdateTextures();
	return m_texRma;
}

AlphaMode pragma::material::Material::GetAlphaMode() const { return m_alphaMode; }
float pragma::material::Material::GetAlphaCutoff() const { return GetProperty<float>("alpha_cutoff", 0.5f); }

bool pragma::material::Material::HasPropertyBlock(const std::string_view &name) const { return GetPropertyBlock(name) != nullptr; }

std::shared_ptr<pragma::datasystem::Block> pragma::material::Material::GetPropertyBlock(const std::string_view &name) const
{
	if(name.empty())
		return m_data;
	auto path = pragma::util::FilePath(name);
	auto data = m_data;
	for(auto &segment : path) {
		data = data->GetBlock(std::string {segment});
		if(data == nullptr)
			return nullptr;
	}
	return data;
}

pragma::material::PropertyType pragma::material::Material::GetPropertyType(const std::string_view &key) const
{
	auto dsVal = m_data->GetDataValue(std::string {key});
	if(!dsVal) {
		auto *baseMaterial = GetBaseMaterial();
		if(baseMaterial)
			return baseMaterial->GetPropertyType(key);
		return PropertyType::None;
	}
	if(dsVal->IsBlock())
		return PropertyType::Block;
	if(typeid(*dsVal) == typeid(datasystem::Texture))
		return PropertyType::Texture;
	return PropertyType::Value;
}

void pragma::material::Material::ClearProperty(datasystem::Block &block, const std::string_view &key, bool clearBlocksIfEmpty)
{
	if(key.find_first_of('/') != std::string::npos) {
		auto path = pragma::util::FilePath(key);
		auto data = std::static_pointer_cast<datasystem::Block>(block.shared_from_this());
		auto parentBlock = data;
		std::shared_ptr<datasystem::Block> secondToLastBlock;
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
void pragma::material::Material::ClearProperty(const std::string_view &key, bool clearBlocksIfEmpty) { ClearProperty(*m_data, key, clearBlocksIfEmpty); }
pragma::datasystem::ValueType pragma::material::Material::GetPropertyValueType(const std::string_view &strPath) const
{
	auto [block, key] = ResolvePropertyPath(strPath);
	if(block == nullptr)
		return datasystem::ValueType::Invalid; // No path segments
	auto &dsVal = block->GetValue(key);
	if(dsVal && dsVal->IsValue())
		return static_cast<datasystem::Value &>(*dsVal).GetType();
	auto *baseMaterial = GetBaseMaterial();
	if(baseMaterial)
		return baseMaterial->GetPropertyValueType(strPath);
	return datasystem::ValueType::Invalid;
}
void pragma::material::Material::SetTextureProperty(const std::string_view &strPath, const std::string_view &tex)
{
	auto [block, key] = ResolvePropertyPath(strPath);
	if(block == nullptr)
		return;
	block->AddValue("texture", std::string {key}, std::string {tex});
}

std::pair<std::shared_ptr<pragma::datasystem::Block>, std::string> pragma::material::Material::ResolvePropertyPath(const std::string_view &strPath) const
{
	if(strPath.find('/') == std::string::npos)
		return {m_data, std::string {strPath}}; // Fast exit if no path segments
	auto path = pragma::util::FilePath(strPath);
	auto data = m_data;
	std::shared_ptr<datasystem::Block> secondToLastBlock;
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

void pragma::material::Material::SetColorFactor(const Vector4 &colorFactor) { SetProperty("color_factor", colorFactor); }
Vector4 pragma::material::Material::GetColorFactor() const { return GetProperty<Vector4>("color_factor", {1.f, 1.f, 1.f, 1.f}); }
void pragma::material::Material::SetBloomColorFactor(const Vector4 &bloomColorFactor) { SetProperty("bloom_color_factor", bloomColorFactor); }
std::optional<Vector4> pragma::material::Material::GetBloomColorFactor() const
{
	Vector4 bloomColor;
	if(!GetProperty<Vector4>("bloom_color_factor", &bloomColor))
		return {};
	return bloomColor;
}

void pragma::material::Material::SetName(const std::string &name) { m_name = name; }
const std::string &pragma::material::Material::GetName() const { return m_name; }

bool pragma::material::Material::IsError() const { return pragma::math::is_flag_set(m_stateFlags, StateFlags::Error); }
void pragma::material::Material::SetErrorFlag(bool set) { pragma::math::set_flag(m_stateFlags, StateFlags::Error, set); }

const pragma::util::ShaderInfo *pragma::material::Material::GetShaderInfo() const { return m_shaderInfo.get(); }
const std::string &pragma::material::Material::GetShaderIdentifier() const
{
	if(m_shaderInfo.expired() == false)
		return m_shaderInfo.get()->GetIdentifier();
	static std::string empty;
	return m_shader ? *m_shader : empty;
}

const pragma::material::Material *pragma::material::Material::GetBaseMaterial() const { return const_cast<Material *>(this)->GetBaseMaterial(); }
void pragma::material::Material::UpdateBaseMaterial()
{
	if(m_baseMaterial->material)
		return;
	auto baseMat = m_manager.LoadAsset(m_baseMaterial->name);
	if(baseMat) {
		m_baseMaterial->material = baseMat->shared_from_this();
		m_baseMaterial->onBaseTexturesUpdated = baseMat->AddEventListener(Event::OnTexturesUpdated, [this]() { OnBaseMaterialChanged(); });
	}
}
pragma::material::Material *pragma::material::Material::GetBaseMaterial()
{
	if(!m_baseMaterial)
		return nullptr;
	UpdateBaseMaterial();
	return m_baseMaterial ? m_baseMaterial->material.get() : nullptr;
}
void pragma::material::Material::SetBaseMaterial(const std::string &baseMaterial)
{
	m_baseMaterial = std::make_unique<BaseMaterial>();
	m_baseMaterial->name = baseMaterial;
	m_manager.PreloadAsset(m_baseMaterial->name);
}
void pragma::material::Material::SetBaseMaterial(Material *baseMaterial)
{
	m_baseMaterial = nullptr;
	if(!baseMaterial)
		return;
	m_baseMaterial = std::make_unique<BaseMaterial>();
	m_baseMaterial->material = baseMaterial->shared_from_this();
	m_baseMaterial->onBaseTexturesUpdated = baseMaterial->AddEventListener(Event::OnTexturesUpdated, [this]() { OnBaseMaterialChanged(); });
	m_baseMaterial->name = baseMaterial->GetName();
}

void pragma::material::Material::OnBaseMaterialChanged() {}

const TextureInfo *pragma::material::Material::GetTextureInfo(const std::string_view &key) const { return const_cast<Material *>(this)->GetTextureInfo(key); }

TextureInfo *pragma::material::Material::GetTextureInfo(const std::string_view &key)
{
	TextureInfo *texInfo = nullptr;
	if(m_data) {
		auto &base = m_data->GetValue(key);
		if(base != nullptr && !base->IsBlock()) {
			auto &val = static_cast<datasystem::Value &>(*base);
			auto &type = typeid(val);
			if(type == typeid(datasystem::Texture)) {
				auto &datTex = static_cast<datasystem::Texture &>(val);
				return &const_cast<TextureInfo &>(datTex.GetValue());
			}
		}
	}
	if(m_baseMaterial && m_baseMaterial->material)
		return m_baseMaterial->material->GetTextureInfo(key);
	return nullptr;
}

pragma::material::MaterialHandle pragma::material::Material::GetHandle() { return shared_from_this(); }

std::shared_ptr<pragma::material::Material> pragma::material::Material::Copy(bool copyData) const
{
	std::shared_ptr<Material> r = nullptr;
	if(!IsValid())
		r = GetManager().CreateMaterial("pbr", nullptr);
	else if(m_shaderInfo.expired() == false) {
		auto data = copyData ? std::shared_ptr<datasystem::Block>(m_data->Copy()) : std::make_shared<datasystem::Block>(m_data->GetDataSettings());
		r = GetManager().CreateMaterial(m_shaderInfo->GetIdentifier(), data);
	}
	else {
		auto data = copyData ? std::shared_ptr<datasystem::Block>(m_data->Copy()) : std::make_shared<datasystem::Block>(m_data->GetDataSettings());
		r = GetManager().CreateMaterial(*m_shader, data);
	}
	if(IsLoaded())
		r->SetLoaded(true);
	r->m_stateFlags = m_stateFlags;
	pragma::math::set_flag(r->m_stateFlags, StateFlags::TexturesUpdated, false);
	return r;
}

std::ostream &operator<<(std::ostream &out, const pragma::material::Material &o)
{
	out << "Material";
	out << "[Index:" << o.GetIndex() << "]";
	out << "[Name:" << const_cast<pragma::material::Material &>(o).GetName() << "]";
	out << "[Shader:" << o.GetShaderIdentifier() << "]";
	out << "[AlphaMode:" << magic_enum::enum_name(o.GetAlphaMode()) << "]";
	out << "[AlphaCutoff:" << o.GetAlphaCutoff() << "]";
	out << "[ColorFactor:" << o.GetColorFactor() << "]";
	out << "[Error:" << o.IsError() << "]";
	out << "[Loaded:" << o.IsLoaded() << "]";
	return out;
}
