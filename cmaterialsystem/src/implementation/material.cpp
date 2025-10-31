// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;


module pragma.cmaterialsystem;

import :material;
import :material_manager2;

msys::CMaterial::CallbackInfo::CallbackInfo(const std::function<void(std::shared_ptr<Texture>)> &_onload) : count(0)
{
	if(_onload) {
		onload = FunctionCallback<void, std::shared_ptr<Texture>>::Create([_onload](std::shared_ptr<Texture> tex) { _onload(tex); });
	}
}

////////////////////////////////

std::size_t msys::CMaterial::ShaderHash::operator()(const util::WeakHandle<prosper::Shader> &whShader) const
{
	if(whShader.expired())
		return std::hash<std::string>()("");
	return std::hash<std::string>()(whShader.get()->GetIdentifier());
}
bool msys::CMaterial::ShaderEqualFn::operator()(const util::WeakHandle<prosper::Shader> &whShader0, const util::WeakHandle<prosper::Shader> &whShader1) const { return whShader0.get() == whShader1.get(); }

////////////////////////////////

std::shared_ptr<msys::CMaterial> msys::CMaterial::Create(msys::MaterialManager &manager)
{
	auto mat = std::shared_ptr<CMaterial> {new CMaterial {manager}};
	mat->Reset();
	return mat;
}
std::shared_ptr<msys::CMaterial> msys::CMaterial::Create(msys::MaterialManager &manager, const util::WeakHandle<util::ShaderInfo> &shader, const std::shared_ptr<ds::Block> &data)
{
	auto mat = std::shared_ptr<CMaterial> {new CMaterial {manager, shader, data}};
	mat->Initialize(shader, data);
	return mat;
}
std::shared_ptr<msys::CMaterial> msys::CMaterial::Create(msys::MaterialManager &manager, const std::string &shader, const std::shared_ptr<ds::Block> &data)
{
	auto mat = std::shared_ptr<CMaterial> {new CMaterial {manager, shader, data}};
	mat->Initialize(shader, data);
	return mat;
}
msys::CMaterial::CMaterial(msys::MaterialManager &manager) : Material(manager) {}
msys::CMaterial::CMaterial(msys::MaterialManager &manager, const util::WeakHandle<util::ShaderInfo> &shader, const std::shared_ptr<ds::Block> &data) : Material(manager, shader, data)
{
	// UpdatePrimaryShader();
}
msys::CMaterial::CMaterial(msys::MaterialManager &manager, const std::string &shader, const std::shared_ptr<ds::Block> &data) : Material(manager, shader, data)
{
	// UpdatePrimaryShader();
}

msys::CMaterial::~CMaterial()
{
	if(m_callbackInfo && m_callbackInfo->onload.IsValid())
		m_callbackInfo->onload.Remove();
	for(auto &cb : m_onVkTexturesChanged) {
		if(cb.IsValid())
			cb.Remove();
	}
}

std::shared_ptr<msys::Material> msys::CMaterial::Copy(bool copyData) const
{
	auto cpy = Material::Copy(copyData);
	if(!cpy)
		return cpy;
	auto *ccpy = static_cast<CMaterial *>(cpy.get());
	ccpy->m_primaryShader = m_primaryShader;
	ccpy->m_sampler = m_sampler;
	if(copyData)
		ccpy->m_spriteSheetAnimation = m_spriteSheetAnimation;
	ccpy->m_stateFlags = m_stateFlags;
	if(!copyData)
		umath::set_flag(ccpy->m_stateFlags, StateFlags::TexturesLoaded, false);
	return cpy;
}

void msys::CMaterial::InitializeSampler()
{
	prosper::util::SamplerCreateInfo samplerInfo {};

	auto bUseCustomSampler = false;

	const std::unordered_map<std::string, int32_t> addressModes
	  = {{"ADDRESS_MODE_REPEAT", umath::to_integral(prosper::SamplerAddressMode::Repeat)}, {"ADDRESS_MODE_MIRRORED_REPEAT", umath::to_integral(prosper::SamplerAddressMode::MirroredRepeat)}, {"ADDRESS_MODE_CLAMP_TO_EDGE", umath::to_integral(prosper::SamplerAddressMode::ClampToEdge)},
	    {"ADDRESS_MODE_CLAMP_TO_BORDER", umath::to_integral(prosper::SamplerAddressMode::ClampToBorder)}, {"ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE", umath::to_integral(prosper::SamplerAddressMode::MirrorClampToEdge)}};

	const std::unordered_map<std::string, int32_t> borderColors = {{"BORDER_COLOR_FLOAT_TRANSPARENT_BLACK", umath::to_integral(prosper::BorderColor::FloatTransparentBlack)}, {"BORDER_COLOR_INT_TRANSPARENT_BLACK", umath::to_integral(prosper::BorderColor::IntTransparentBlack)},
	  {"BORDER_COLOR_FLOAT_OPAQUE_BLACK", umath::to_integral(prosper::BorderColor::FloatOpaqueBlack)}, {"BORDER_COLOR_INT_OPAQUE_BLACK", umath::to_integral(prosper::BorderColor::IntOpaqueBlack)},
	  {"BORDER_COLOR_FLOAT_OPAQUE_WHITE", umath::to_integral(prosper::BorderColor::FloatOpaqueWhite)}, {"BORDER_COLOR_INT_OPAQUE_WHITE", umath::to_integral(prosper::BorderColor::IntOpaqueWhite)}};

	const auto fGetValue = [this](const std::unordered_map<std::string, int32_t> &map, const std::string &identifier, int32_t &outVal) -> bool {
		outVal = -1;
		std::string strVal;
		if(m_data->GetString(identifier, &strVal) == true) {
			ustring::to_upper(strVal);
			auto it = map.find(strVal);
			if(it != map.end()) {
				outVal = it->second;
				return true;
			}
		}
		return m_data->GetInt(identifier, &outVal);
	};
	const auto fGetAddressMode = [&fGetValue, &addressModes](const std::string &identifier, int32_t &outVal) -> bool { return fGetValue(addressModes, identifier, outVal); };
	const auto fGetBorderColor = [&fGetValue, &borderColors](const std::string &identifier, int32_t &outVal) -> bool { return fGetValue(borderColors, identifier, outVal); };

	int32_t intVal = -1;
	if(fGetAddressMode("address_mode_u", intVal) == true) {
		bUseCustomSampler = true;
		samplerInfo.addressModeU = static_cast<prosper::SamplerAddressMode>(intVal);
	}
	if(fGetAddressMode("address_mode_v", intVal) == true) {
		bUseCustomSampler = true;
		samplerInfo.addressModeV = static_cast<prosper::SamplerAddressMode>(intVal);
	}
	if(fGetAddressMode("address_mode_w", intVal) == true) {
		bUseCustomSampler = true;
		samplerInfo.addressModeW = static_cast<prosper::SamplerAddressMode>(intVal);
	}
	if(fGetBorderColor("border_color", intVal) == true) {
		bUseCustomSampler = true;
		samplerInfo.borderColor = static_cast<prosper::BorderColor>(intVal);
	}
	if(bUseCustomSampler == true) {
		auto mipmapMode = static_cast<TextureMipmapMode>(GetMipmapMode(*m_data));
		msys::setup_sampler_mipmap_mode(samplerInfo, mipmapMode);
		m_sampler = GetContext().CreateSampler(samplerInfo);
	}
}

std::shared_ptr<prosper::ISampler> msys::CMaterial::GetSampler() { return m_sampler; }

prosper::IBuffer *msys::CMaterial::GetSettingsBuffer() { return m_settingsBuffer.get(); }
void msys::CMaterial::SetSettingsBuffer(prosper::IBuffer &buffer) { m_settingsBuffer = buffer.shared_from_this(); }

prosper::IPrContext &msys::CMaterial::GetContext() { return static_cast<msys::CMaterialManager &>(m_manager).GetContext(); }
msys::TextureManager &msys::CMaterial::GetTextureManager() { return static_cast<msys::CMaterialManager &>(m_manager).GetTextureManager(); }
std::unordered_map<util::WeakHandle<prosper::Shader>, std::shared_ptr<prosper::IDescriptorSetGroup>, msys::CMaterial::ShaderHash, msys::CMaterial::ShaderEqualFn>::iterator msys::CMaterial::FindShaderDescriptorSetGroup(prosper::Shader &shader)
{
	return std::find_if(m_descriptorSetGroups.begin(), m_descriptorSetGroups.end(), [&shader](const std::pair<util::WeakHandle<prosper::Shader>, std::shared_ptr<prosper::IDescriptorSetGroup>> &pair) { return pair.first.get() == &shader; });
}
std::unordered_map<util::WeakHandle<prosper::Shader>, std::shared_ptr<prosper::IDescriptorSetGroup>, msys::CMaterial::ShaderHash, msys::CMaterial::ShaderEqualFn>::const_iterator msys::CMaterial::FindShaderDescriptorSetGroup(prosper::Shader &shader) const
{
	return const_cast<msys::CMaterial *>(this)->FindShaderDescriptorSetGroup(shader);
}
const std::shared_ptr<prosper::IDescriptorSetGroup> &msys::CMaterial::GetDescriptorSetGroup(prosper::Shader &shader) const
{
	static std::shared_ptr<prosper::IDescriptorSetGroup> nptr = nullptr;
	auto it = FindShaderDescriptorSetGroup(shader);
	return (it != m_descriptorSetGroups.end()) ? it->second : nptr;
}
bool msys::CMaterial::IsInitialized() const { return (IsLoaded() && m_descriptorSetGroups.empty() == false) ? true : false; }
void msys::CMaterial::SetShaderInfo(const util::WeakHandle<util::ShaderInfo> &shaderInfo) { msys::Material::SetShaderInfo(shaderInfo); }
void msys::CMaterial::ResetRenderResources() { m_descriptorSetGroups.clear(); }
void msys::CMaterial::Reset()
{
	msys::Material::Reset();
	m_primaryShader = nullptr;
	ResetRenderResources();
	m_stateFlags = StateFlags::None;
	// UpdatePrimaryShader();
}
void msys::CMaterial::Assign(const msys::Material &other)
{
	msys::Material::Assign(other);
	UpdatePrimaryShader();
}
void msys::CMaterial::SetPrimaryShader(prosper::Shader &shader)
{
	auto &context = GetContext();
	auto &shaderManager = context.GetShaderManager();
	m_shader = std::make_unique<std::string>(shader.GetIdentifier());
	m_shaderInfo = shaderManager.PreRegisterShader(shader.GetIdentifier());
	m_primaryShader = &shader;
	context.KeepResourceAliveUntilPresentationComplete(m_settingsBuffer);
	m_settingsBuffer = nullptr;
}
void msys::CMaterial::UpdatePrimaryShader()
{
	if(m_shader == nullptr && m_shaderInfo.expired()) {
		m_primaryShader = nullptr;
		return;
	}
	auto &context = GetContext();
	auto &shaderManager = context.GetShaderManager();
	m_primaryShader = shaderManager.GetShader(GetShaderIdentifier()).get();
}
prosper::Shader *msys::CMaterial::GetPrimaryShader()
{
	if(m_primaryShader == nullptr)
		UpdatePrimaryShader();
	return m_primaryShader;
}
void msys::CMaterial::SetDescriptorSetGroup(prosper::Shader &shader, const std::shared_ptr<prosper::IDescriptorSetGroup> &descSetGroup)
{
	auto it = FindShaderDescriptorSetGroup(shader);
	if(it != m_descriptorSetGroups.end()) {
		GetContext().KeepResourceAliveUntilPresentationComplete(it->second);
		it->second = descSetGroup;
		return;
	}
	m_descriptorSetGroups.insert(std::make_pair(shader.GetHandle(), descSetGroup));
}
uint32_t msys::CMaterial::GetMipmapMode(const ds::Block &block) const { return umath::to_integral(GetProperty<TextureMipmapMode>(block, "mipmap_load_mode", TextureMipmapMode::Load)); }
void msys::CMaterial::SetLoaded(bool b) { msys::Material::SetLoaded(b); }
void msys::CMaterial::SetSpriteSheetAnimation(const SpriteSheetAnimation &animInfo) { m_spriteSheetAnimation = animInfo; }
void msys::CMaterial::ClearSpriteSheetAnimation() { m_spriteSheetAnimation = {}; }
const SpriteSheetAnimation *msys::CMaterial::GetSpriteSheetAnimation() const { return const_cast<msys::CMaterial *>(this)->GetSpriteSheetAnimation(); }
SpriteSheetAnimation *msys::CMaterial::GetSpriteSheetAnimation()
{
	if(m_spriteSheetAnimation.has_value() == false) {
		// Lazy initialization
		std::string animFilePath;
		if(GetProperty<std::string>("animation", &animFilePath)) {
			auto f = filemanager::open_file("materials/" + animFilePath + ".psd", filemanager::FileMode::Read | filemanager::FileMode::Binary);
			m_spriteSheetAnimation = SpriteSheetAnimation {};
			if(f == nullptr || m_spriteSheetAnimation->Load(f) == false)
				m_spriteSheetAnimation = {};
		}
	}
	return m_spriteSheetAnimation.has_value() ? &*m_spriteSheetAnimation : nullptr;
}
void msys::CMaterial::LoadTexture(TextureMipmapMode mipmapMode, TextureInfo &texInfo, TextureLoadFlags loadFlags, const std::shared_ptr<CallbackInfo> &callbackInfo)
{
	if(texInfo.texture == nullptr) // Texture hasn't been initialized yet
	{
		auto &textureManager = GetTextureManager();
		auto &context = GetContext();

		texInfo.texture = nullptr;
		auto loadInfo = std::make_unique<msys::TextureLoadInfo>();
		loadInfo->mipmapMode = mipmapMode;
		if(!umath::is_flag_set(loadFlags, TextureLoadFlags::LoadInstantly)) {
			textureManager.PreloadAsset(texInfo.name, std::move(loadInfo));
			return;
		}
		if(callbackInfo && callbackInfo->onload.IsValid()) {
			loadInfo->onLoaded = [callbackInfo](util::Asset &asset) {
				if(callbackInfo->onload.IsValid())
					msys::TextureManager::GetAssetObject(asset)->CallOnLoaded(callbackInfo->onload);
			};
		}
		auto tex = textureManager.LoadAsset(texInfo.name, std::move(loadInfo));
		if(tex) {
			auto &vkTex = tex->GetVkTexture();
			if(vkTex && m_sampler)
				vkTex->SetSampler(*m_sampler);
			texInfo.texture = tex;
		}
		else if(callbackInfo && callbackInfo->onload.IsValid())
			callbackInfo->onload.Call<void, std::shared_ptr<Texture>>(nullptr);

		if(texInfo.texture) {
			auto cb = static_cast<Texture *>(texInfo.texture.get())->CallOnVkTextureChanged([this]() {
				ClearDescriptorSets();
				static_cast<msys::CMaterialManager &>(m_manager).MarkForReload(*this);
			});
			m_onVkTexturesChanged.push_back(cb);

			auto info = callbackInfo;
			std::static_pointer_cast<Texture>(texInfo.texture)->CallOnLoaded([info](std::shared_ptr<Texture>) { info.use_count(); }); // Keep it alive until the texture is loaded ('use_count' to make sure the compiler doesn't get the idea of optimzing anything here.).
		}
	}
	else if(callbackInfo != nullptr && umath::is_flag_set(loadFlags, TextureLoadFlags::LoadInstantly))
		callbackInfo->onload(nullptr);
}
void msys::CMaterial::LoadTexture(const std::shared_ptr<ds::Block> &data, TextureInfo &texInfo, TextureLoadFlags loadFlags, const std::shared_ptr<CallbackInfo> &callbackInfo)
{
	auto mipmapMode = static_cast<TextureMipmapMode>(GetMipmapMode(*data));
	LoadTexture(mipmapMode, texInfo, loadFlags, callbackInfo);
}
void msys::CMaterial::ClearDescriptorSets()
{
	for(auto &pair : m_descriptorSetGroups)
		GetContext().KeepResourceAliveUntilPresentationComplete(pair.second);
	m_descriptorSetGroups.clear();
}
void msys::CMaterial::LoadTexture(const std::shared_ptr<ds::Block> &data, const std::shared_ptr<ds::Texture> &dataTexture, TextureLoadFlags loadFlags, const std::shared_ptr<CallbackInfo> &callbackInfo) { LoadTexture(data, dataTexture->GetValue(), loadFlags, callbackInfo); }

void msys::CMaterial::SetTexture(const std::string &identifier, Texture *texture)
{
	auto dsSettingsTmp = ds::create_data_settings({});
	auto dataTex = std::make_shared<ds::Texture>(*dsSettingsTmp, ""); // Data settings will be overwrriten by AddData-call below
	auto &v = dataTex->GetValue();
	v.texture = texture->shared_from_this();
	v.width = texture->GetWidth();
	v.height = texture->GetHeight();
	v.name = texture->GetName();
	m_data->AddData(identifier, dataTex);

	umath::set_flag(msys::Material::m_stateFlags, msys::Material::StateFlags::TexturesUpdated, false);
	UpdateTextures();
	auto &shaderHandler = static_cast<msys::CMaterialManager &>(GetManager()).GetShaderHandler();
	if(shaderHandler != nullptr)
		shaderHandler(this);
}

void msys::CMaterial::SetOnLoadedCallback(const std::function<void(void)> &f) { m_fOnLoaded = f; }

void msys::CMaterial::OnTexturesUpdated()
{
	msys::Material::OnTexturesUpdated();
	for(auto &cb : m_onVkTexturesChanged) {
		if(cb.IsValid())
			cb.Remove();
	}
	m_onVkTexturesChanged.clear();
	for(auto &pair : *m_data->GetData()) {
		auto &val = static_cast<ds::Value &>(*pair.second);
		auto &type = typeid(val);
		if(type != typeid(ds::Texture))
			continue;
		auto &dsTex = static_cast<ds::Texture &>(val);
		auto &texInfo = dsTex.GetValue();
		if(!texInfo.texture)
			continue;
		auto &tex = *static_cast<msys::Texture *>(texInfo.texture.get());
		// If one of our textures has changed, we have to clear our descriptor sets because
		// they may no longer be valid.
		auto cb = tex.CallOnVkTextureChanged([this]() { ClearDescriptorSets(); });
		m_onVkTexturesChanged.push_back(cb);
	}
}

void msys::CMaterial::SetTexture(const std::string &identifier, const std::string &texture)
{
	auto dsSettingsTmp = ds::create_data_settings({});
	auto dataTex = std::make_shared<ds::Texture>(*dsSettingsTmp, texture); // Data settings will be overwrriten by AddData-call below
	m_data->AddData(identifier, dataTex);
	umath::set_flag(msys::Material::m_stateFlags, msys::Material::StateFlags::TexturesUpdated, false);
	UpdateTextures();

	SetLoaded(false);
	auto callbackInfo = InitializeCallbackInfo(
	  [this]() {
		  SetLoaded(true);
		  if(m_fOnLoaded != nullptr)
			  m_fOnLoaded();
		  auto shaderHandler = static_cast<msys::CMaterialManager &>(GetManager()).GetShaderHandler();
		  if(shaderHandler != nullptr)
			  shaderHandler(this);
	  },
	  nullptr);
	auto &texInfo = dataTex->GetValue();
	auto tex = std::static_pointer_cast<msys::Texture>(texInfo.texture);
	++callbackInfo->count;

	auto bLoadInstantly = true; // TODO: Load in background
	LoadTexture(m_data, dataTex, bLoadInstantly ? TextureLoadFlags::LoadInstantly : TextureLoadFlags::None, callbackInfo);
}

void msys::CMaterial::SetTexture(const std::string &identifier, prosper::Texture &texture)
{
	auto ptrProsperTex = texture.shared_from_this();
	auto tex = std::make_shared<msys::Texture>(GetContext(), ptrProsperTex);
	SetTexture(identifier, tex.get());
}

bool msys::CMaterial::HaveTexturesBeenInitialized() const { return umath::is_flag_set(m_stateFlags, StateFlags::TexturesInitialized); }

void msys::CMaterial::LoadTexture(TextureInfo &texInfo, bool precache)
{
	if(texInfo.name.empty() || texInfo.texture != nullptr)
		return;
	auto mipmapMode = static_cast<TextureMipmapMode>(GetMipmapMode(*m_data));
	auto &textureManager = GetTextureManager();
	auto loadInfo = std::make_unique<msys::TextureLoadInfo>();
	loadInfo->textureData = std::static_pointer_cast<udm::Property>(texInfo.userData);
	loadInfo->mipmapMode = mipmapMode;
	if(loadInfo->textureData) {
		auto cache = true;
		if((*loadInfo->textureData)["cache"](cache) && !cache)
			loadInfo->flags = util::AssetLoadFlags::IgnoreCache | util::AssetLoadFlags ::DontCache;
	}
	if(!precache)
		texInfo.texture = textureManager.LoadAsset(texInfo.name, std::move(loadInfo));
	else
		textureManager.PreloadAsset(texInfo.name, std::move(loadInfo));
}

TextureInfo *msys::CMaterial::GetTextureInfo(const std::string_view &key)
{
	LoadTextures(false);
	return msys::Material::GetTextureInfo(key);
}

void msys::CMaterial::OnBaseMaterialChanged()
{
	Material::OnBaseMaterialChanged();
	ResetRenderResources();
	m_stateFlags = StateFlags::None;
	LoadTextures(false, true);
	UpdateTextures(true);
}

void msys::CMaterial::LoadTextures(bool precache, bool force)
{
	if(precache) {
		if(!force && (umath::is_flag_set(m_stateFlags, StateFlags::TexturesLoaded) || umath::is_flag_set(m_stateFlags, StateFlags::TexturesPrecached)))
			return;
		LoadTextures(*m_data, precache, force);
		return;
	}
	if(!force && umath::is_flag_set(m_stateFlags, StateFlags::TexturesLoaded))
		return;
	LoadTextures(*m_data, precache, force);
	UpdateTextures();
	auto &shaderHandler = static_cast<msys::CMaterialManager &>(m_manager).GetShaderHandler();
	if(shaderHandler)
		shaderHandler(this);
}
void msys::CMaterial::LoadTextures(ds::Block &data, bool precache, bool force)
{
	if(precache) {
		if(!force && (umath::is_flag_set(m_stateFlags, StateFlags::TexturesLoaded) || umath::is_flag_set(m_stateFlags, StateFlags::TexturesPrecached)))
			return;
		umath::set_flag(m_stateFlags, StateFlags::TexturesPrecached);
	}
	else {
		if(!force && umath::is_flag_set(m_stateFlags, StateFlags::TexturesLoaded))
			return;
		umath::set_flag(m_stateFlags, StateFlags::TexturesLoaded);
	}
	auto *values = data.GetData();
	const auto &typeTexture = typeid(ds::Texture);
	for(auto &it : *values) {
		auto &value = it.second;
		if(!value->IsBlock()) {
			auto &type = typeid(*value);
			if(type == typeTexture)
				LoadTexture(std::static_pointer_cast<ds::Texture>(value)->GetValue(), precache);
			else
				continue;
		}
		else {
			auto dataBlock = std::static_pointer_cast<ds::Block>(value);
			LoadTextures(*dataBlock, precache, false);
		}
	}

	if(precache)
		return;
	auto *texNormalMap = GetNormalMap();
	if(texNormalMap && texNormalMap->texture)
		std::static_pointer_cast<msys::Texture>(texNormalMap->texture)->AddFlags(msys::Texture::Flags::NormalMap);
}

void msys::CMaterial::Initialize(const std::shared_ptr<ds::Block> &data)
{
	Material::Initialize(data);
	LoadTextures(true);
}

void msys::CMaterial::InitializeTextures(const std::shared_ptr<ds::Block> &data, const std::shared_ptr<CallbackInfo> &info, TextureLoadFlags loadFlags)
{
	auto precache = !umath::is_flag_set(loadFlags, TextureLoadFlags::LoadInstantly);
	if(precache) {
		InitializeSampler();
		ClearDescriptorSets();
	}

	std::function<void(CMaterial &, const util::Path &)> loadTextures = nullptr;
	loadTextures = [this, &loadTextures, &info, &loadFlags](CMaterial &mat, const util::Path &path) {
		auto mipmapMode = GetProperty<TextureMipmapMode>(util::FilePath(path, "mipmap_load_mode").GetString(), TextureMipmapMode::Load);
		for(auto &name : msys::MaterialPropertyBlockView {mat, path}) {
			auto propType = mat.GetPropertyType(name);
			switch(propType) {
			case msys::PropertyType::Block:
				loadTextures(mat, util::FilePath(path, name));
				break;
			case msys::PropertyType::Texture:
				{
					std::string texName;
					if(!mat.GetProperty(util::FilePath(path, name).GetString(), &texName))
						continue;
					auto *texInfo = mat.GetTextureInfo(name);
					if(!texInfo)
						continue;
					++info->count;

					LoadTexture(mipmapMode, *texInfo, loadFlags, info);
					break;
				}
			}
		}
	};
	loadTextures(*this, {});
}

std::shared_ptr<msys::CMaterial::CallbackInfo> msys::CMaterial::InitializeCallbackInfo(const std::function<void(void)> &onAllTexturesLoaded, const std::function<void(std::shared_ptr<Texture>)> &onTextureLoaded)
{
	if(m_callbackInfo == nullptr) {
		m_callbackInfo = std::make_shared<CallbackInfo>();
		auto *info = m_callbackInfo.get();
		m_callbackInfo->onload = FunctionCallback<void, std::shared_ptr<msys::Texture>>::Create([info, onTextureLoaded, onAllTexturesLoaded](std::shared_ptr<msys::Texture> texture) {
			--info->count;
			if(texture != nullptr && onTextureLoaded != nullptr)
				onTextureLoaded(texture);
			if(info->count == 0)
				onAllTexturesLoaded();
		});
	}
	return m_callbackInfo;
}

void msys::CMaterial::InitializeTextures(const std::shared_ptr<ds::Block> &data, const std::function<void(void)> &onAllTexturesLoaded, const std::function<void(std::shared_ptr<msys::Texture>)> &onTextureLoaded, TextureLoadFlags loadFlags)
{
	/*if(!umath::is_flag_set(loadFlags,TextureLoadFlags::LoadInstantly))
	{
		// Precache only
		auto tmp = std::make_shared<CallbackInfo>();
		InitializeTextures(data,tmp,loadFlags);
		return;
	}
	umath::set_flag(m_stateFlags,StateFlags::TexturesInitialized);
	auto callbackInfo = InitializeCallbackInfo(onAllTexturesLoaded,onTextureLoaded);
	++callbackInfo->count; // Dummy, in case all textures are loaded immediately (In which case the final callback would be triggered multiple times.).
	InitializeTextures(data,callbackInfo,loadFlags);
	callbackInfo->onload(std::shared_ptr<Texture>{nullptr}); // Clear dummy
	*/
}
