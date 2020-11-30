/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <prosper_context.hpp>
#include <sharedutils/util_shaderinfo.hpp>
#include "cmaterialmanager.h"
#include "cmaterial.h"
#include "texture_type.h"
#include "sprite_sheet_animation.hpp"
#include <prosper_util.hpp>
#include <buffers/prosper_buffer.hpp>
#include <image/prosper_sampler.hpp>
#include <sharedutils/util_string.h>

CMaterial::CallbackInfo::CallbackInfo(const std::function<void(std::shared_ptr<Texture>)> &_onload)
	: count(0),onload(_onload)
{}

////////////////////////////////

std::size_t CMaterial::ShaderHash::operator()(const util::WeakHandle<prosper::Shader> &whShader) const
{
	if(whShader.expired())
		return std::hash<std::string>()("");
	return std::hash<std::string>()(whShader.get()->GetIdentifier());
}
bool CMaterial::ShaderEqualFn::operator()(const util::WeakHandle<prosper::Shader> &whShader0,const util::WeakHandle<prosper::Shader> &whShader1) const
{
	return whShader0.get() == whShader1.get();
}

////////////////////////////////

CMaterial::CMaterial(MaterialManager &manager)
	: Material(manager)
{}
CMaterial::CMaterial(MaterialManager &manager,const util::WeakHandle<util::ShaderInfo> &shader,const std::shared_ptr<ds::Block> &data)
	: Material(manager,shader,data)
{
	UpdatePrimaryShader();
}
CMaterial::CMaterial(MaterialManager &manager,const std::string &shader,const std::shared_ptr<ds::Block> &data)
	: Material(manager,shader,data)
{
	UpdatePrimaryShader();
}

CMaterial::~CMaterial()
{}

Material *CMaterial::Copy() const {return Material::Copy<CMaterial>();}

void CMaterial::InitializeSampler()
{
	prosper::util::SamplerCreateInfo samplerInfo {};

	auto bUseCustomSampler = false;

	const std::unordered_map<std::string,int32_t> addressModes = {
		{"ADDRESS_MODE_REPEAT",umath::to_integral(prosper::SamplerAddressMode::Repeat)},
		{"ADDRESS_MODE_MIRRORED_REPEAT",umath::to_integral(prosper::SamplerAddressMode::MirroredRepeat)},
		{"ADDRESS_MODE_CLAMP_TO_EDGE",umath::to_integral(prosper::SamplerAddressMode::ClampToEdge)},
		{"ADDRESS_MODE_CLAMP_TO_BORDER",umath::to_integral(prosper::SamplerAddressMode::ClampToBorder)},
		{"ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE",umath::to_integral(prosper::SamplerAddressMode::MirrorClampToEdge)}
	};

	const std::unordered_map<std::string,int32_t> borderColors = {
		{"BORDER_COLOR_FLOAT_TRANSPARENT_BLACK",umath::to_integral(prosper::BorderColor::FloatTransparentBlack)},
		{"BORDER_COLOR_INT_TRANSPARENT_BLACK",umath::to_integral(prosper::BorderColor::IntTransparentBlack)},
		{"BORDER_COLOR_FLOAT_OPAQUE_BLACK",umath::to_integral(prosper::BorderColor::FloatOpaqueBlack)},
		{"BORDER_COLOR_INT_OPAQUE_BLACK",umath::to_integral(prosper::BorderColor::IntOpaqueBlack)},
		{"BORDER_COLOR_FLOAT_OPAQUE_WHITE",umath::to_integral(prosper::BorderColor::FloatOpaqueWhite)},
		{"BORDER_COLOR_INT_OPAQUE_WHITE",umath::to_integral(prosper::BorderColor::IntOpaqueWhite)}
	};

	const auto fGetValue = [this](const std::unordered_map<std::string,int32_t> &map,const std::string &identifier,int32_t &outVal) -> bool {
		outVal = -1;
		std::string strVal;
		if(m_data->GetString(identifier,&strVal) == true)
		{
			ustring::to_upper(strVal);
			auto it = map.find(strVal);
			if(it != map.end())
			{
				outVal = it->second;
				return true;
			}
		}
		return m_data->GetInt(identifier,&outVal);
	};
	const auto fGetAddressMode = [&fGetValue,&addressModes](const std::string &identifier,int32_t &outVal) -> bool {
		return fGetValue(addressModes,identifier,outVal);
	};
	const auto fGetBorderColor = [&fGetValue,&borderColors](const std::string &identifier,int32_t &outVal) -> bool {
		return fGetValue(borderColors,identifier,outVal);
	};

	int32_t intVal = -1;
	if(fGetAddressMode("address_mode_u",intVal) == true)
	{
		bUseCustomSampler = true;
		samplerInfo.addressModeU = static_cast<prosper::SamplerAddressMode>(intVal);
	}
	if(fGetAddressMode("address_mode_v",intVal) == true)
	{
		bUseCustomSampler = true;
		samplerInfo.addressModeV = static_cast<prosper::SamplerAddressMode>(intVal);
	}
	if(fGetAddressMode("address_mode_w",intVal) == true)
	{
		bUseCustomSampler = true;
		samplerInfo.addressModeW = static_cast<prosper::SamplerAddressMode>(intVal);
	}
	if(fGetBorderColor("border_color",intVal) == true)
	{
		bUseCustomSampler = true;
		samplerInfo.borderColor = static_cast<prosper::BorderColor>(intVal);
	}
	if(bUseCustomSampler == true)
	{
		auto mipmapMode = static_cast<TextureMipmapMode>(GetMipmapMode(m_data));
		TextureManager::SetupSamplerMipmapMode(samplerInfo,mipmapMode);
		m_sampler = GetContext().CreateSampler(samplerInfo);
	}
}

std::shared_ptr<prosper::ISampler> CMaterial::GetSampler() {return m_sampler;}

prosper::IBuffer *CMaterial::GetSettingsBuffer() {return m_settingsBuffer.get();}
void CMaterial::SetSettingsBuffer(prosper::IBuffer &buffer) {m_settingsBuffer = buffer.shared_from_this();}

prosper::IPrContext &CMaterial::GetContext() {return static_cast<CMaterialManager&>(m_manager).GetContext();}
TextureManager &CMaterial::GetTextureManager() {return static_cast<CMaterialManager&>(m_manager).GetTextureManager();}
std::unordered_map<util::WeakHandle<prosper::Shader>,std::shared_ptr<prosper::IDescriptorSetGroup>,CMaterial::ShaderHash,CMaterial::ShaderEqualFn>::iterator CMaterial::FindShaderDescriptorSetGroup(prosper::Shader &shader)
{
	return std::find_if(m_descriptorSetGroups.begin(),m_descriptorSetGroups.end(),[&shader](const std::pair<util::WeakHandle<prosper::Shader>,std::shared_ptr<prosper::IDescriptorSetGroup>> &pair) {
		return pair.first.get() == &shader;
	});
}
std::unordered_map<util::WeakHandle<prosper::Shader>,std::shared_ptr<prosper::IDescriptorSetGroup>,CMaterial::ShaderHash,CMaterial::ShaderEqualFn>::const_iterator CMaterial::FindShaderDescriptorSetGroup(prosper::Shader &shader) const {return const_cast<CMaterial*>(this)->FindShaderDescriptorSetGroup(shader);}
const std::shared_ptr<prosper::IDescriptorSetGroup> &CMaterial::GetDescriptorSetGroup(prosper::Shader &shader) const
{
	static std::shared_ptr<prosper::IDescriptorSetGroup> nptr = nullptr;
	auto it = FindShaderDescriptorSetGroup(shader);
	return (it != m_descriptorSetGroups.end()) ? it->second : nptr;
}
bool CMaterial::IsInitialized() const {return (IsLoaded() && m_descriptorSetGroups.empty() == false) ? true : false;}
void CMaterial::SetShaderInfo(const util::WeakHandle<util::ShaderInfo> &shaderInfo)
{
	Material::SetShaderInfo(shaderInfo);
	UpdatePrimaryShader();
}
void CMaterial::Reset()
{
	Material::Reset();
	UpdatePrimaryShader();
}
void CMaterial::UpdatePrimaryShader()
{
	if(m_shader == nullptr && m_shaderInfo.expired())
	{
		m_primaryShader = nullptr;
		return;
	}
	auto &context = GetContext();
	auto &shaderManager = context.GetShaderManager();
	m_primaryShader = shaderManager.GetShader(GetShaderIdentifier()).get();
}
prosper::Shader *CMaterial::GetPrimaryShader() {return m_primaryShader;}
void CMaterial::SetDescriptorSetGroup(prosper::Shader &shader,const std::shared_ptr<prosper::IDescriptorSetGroup> &descSetGroup)
{
	auto it = FindShaderDescriptorSetGroup(shader);
	if(it != m_descriptorSetGroups.end())
	{
		it->second = descSetGroup;
		return;
	}
	m_descriptorSetGroups.insert(std::make_pair(shader.GetHandle(),descSetGroup));
}
uint32_t CMaterial::GetMipmapMode(const std::shared_ptr<ds::Block> &data) const
{
	auto mipmapMode = TextureMipmapMode::Load;
	data->GetInt("mipmap_load_mode",reinterpret_cast<std::underlying_type<decltype(mipmapMode)>::type*>(&mipmapMode));
	return umath::to_integral(mipmapMode);
}
void CMaterial::SetLoaded(bool b)
{
	auto *texNormalMap = GetNormalMap();
	if(texNormalMap && texNormalMap->texture)
		std::static_pointer_cast<Texture>(texNormalMap->texture)->AddFlags(Texture::Flags::NormalMap);
	Material::SetLoaded(b);
}
void CMaterial::SetSpriteSheetAnimation(const SpriteSheetAnimation &animInfo) {m_spriteSheetAnimation = animInfo;}
void CMaterial::ClearSpriteSheetAnimation() {m_spriteSheetAnimation = {};}
const SpriteSheetAnimation *CMaterial::GetSpriteSheetAnimation() const {return const_cast<CMaterial*>(this)->GetSpriteSheetAnimation();}
SpriteSheetAnimation *CMaterial::GetSpriteSheetAnimation()
{
	if(m_spriteSheetAnimation.has_value() == false)
	{
		// Lazy initialization
		auto &data = GetDataBlock();
		if(data)
		{
			auto &anim = data->GetValue("animation");
			if(anim && typeid(*anim) == typeid(ds::String))
			{
				auto animFilePath = static_cast<ds::String&>(*anim).GetString();
				auto f = FileManager::OpenFile(("materials/" +animFilePath +".psd").c_str(),"rb");
				m_spriteSheetAnimation = SpriteSheetAnimation{};
				if(f == nullptr || m_spriteSheetAnimation->Load(f) == false)
					m_spriteSheetAnimation = {};
			}
		}
	}
	return m_spriteSheetAnimation.has_value() ? &*m_spriteSheetAnimation : nullptr;
}
void CMaterial::LoadTexture(const std::shared_ptr<ds::Block> &data,TextureInfo &texInfo,TextureLoadFlags loadFlags,const std::shared_ptr<CallbackInfo> &callbackInfo)
{
	if(texInfo.texture == nullptr) // Texture hasn't been initialized yet
	{
		auto mipmapMode = static_cast<TextureMipmapMode>(GetMipmapMode(data));
		auto &textureManager = GetTextureManager();
		auto &context = GetContext();
		TextureManager::LoadInfo loadInfo {};
		loadInfo.flags |= loadFlags;
		loadInfo.mipmapLoadMode = mipmapMode;
		loadInfo.sampler = m_sampler;
		if(callbackInfo != nullptr)
			loadInfo.onLoadCallback = callbackInfo->onload;
		textureManager.Load(context,texInfo.name,loadInfo,&texInfo.texture);

		auto info = callbackInfo;
		std::static_pointer_cast<Texture>(texInfo.texture)->CallOnLoaded([info](std::shared_ptr<Texture>) {info.use_count();}); // Keep it alive until the texture is loaded ('use_count' to make sure the compiler doesn't get the idea of optimzing anything here.).
	}
	else if(callbackInfo != nullptr)
		callbackInfo->onload(nullptr);
}
void CMaterial::ClearDescriptorSets()
{
	for(auto &pair : m_descriptorSetGroups)
		GetContext().KeepResourceAliveUntilPresentationComplete(pair.second);
	m_descriptorSetGroups.clear();
}
void CMaterial::LoadTexture(const std::shared_ptr<ds::Block> &data,const std::shared_ptr<ds::Texture> &dataTexture,TextureLoadFlags loadFlags,const std::shared_ptr<CallbackInfo> &callbackInfo)
{
	LoadTexture(data,dataTexture->GetValue(),loadFlags,callbackInfo);
}

void CMaterial::SetTexture(const std::string &identifier,Texture *texture)
{
	auto dsSettingsTmp = ds::create_data_settings({});
	auto dataTex = std::make_shared<ds::Texture>(*dsSettingsTmp,""); // Data settings will be overwrriten by AddData-call below
	auto &v = dataTex->GetValue();
	v.texture = texture->shared_from_this();
	v.width = texture->GetWidth();
	v.height = texture->GetHeight();
	v.name = texture->GetName();
	m_data->AddData(identifier,dataTex);

	UpdateTextures();
	auto shaderHandler = static_cast<CMaterialManager&>(GetManager()).GetShaderHandler();
	if(shaderHandler != nullptr)
		shaderHandler(this);
}

void CMaterial::SetOnLoadedCallback(const std::function<void(void)> &f) {m_fOnLoaded = f;}

void CMaterial::SetTexture(const std::string &identifier,const std::string &texture)
{
	auto dsSettingsTmp = ds::create_data_settings({});
	auto dataTex = std::make_shared<ds::Texture>(*dsSettingsTmp,texture); // Data settings will be overwrriten by AddData-call below
	m_data->AddData(identifier,dataTex);
	UpdateTextures();

	SetLoaded(false);
	auto callbackInfo = InitializeCallbackInfo([this]() {
		SetLoaded(true);
		if(m_fOnLoaded != nullptr)
			m_fOnLoaded();
		auto shaderHandler = static_cast<CMaterialManager&>(GetManager()).GetShaderHandler();
		if(shaderHandler != nullptr)
			shaderHandler(this);
	},nullptr);
	auto &texInfo = dataTex->GetValue();
	auto tex = std::static_pointer_cast<Texture>(texInfo.texture);
	++callbackInfo->count;

	auto bLoadInstantly = false; // Never load instantly?
	LoadTexture(m_data,dataTex,bLoadInstantly ? TextureLoadFlags::LoadInstantly : TextureLoadFlags::None,callbackInfo);
}

void CMaterial::SetTexture(const std::string &identifier,prosper::Texture &texture)
{
	auto ptrProsperTex = texture.shared_from_this();
	auto tex = std::make_shared<Texture>(GetContext(),ptrProsperTex);
	SetTexture(identifier,tex.get());
}

void CMaterial::InitializeTextures(const std::shared_ptr<ds::Block> &data,const std::shared_ptr<CallbackInfo> &info,TextureLoadFlags loadFlags)
{
	InitializeSampler();
	ClearDescriptorSets();
	const auto &typeTexture = typeid(ds::Texture);
	auto *values = data->GetData();
	for(auto &it : *values)
	{
		auto &value = it.second;
		if(!value->IsBlock())
		{
			auto &type = typeid(*value);
			if(type == typeTexture)
			{
				++info->count;
				LoadTexture(data,std::static_pointer_cast<ds::Texture>(value),loadFlags,info);
			}
			else
				continue;
		}
		else
		{
			auto dataBlock = std::static_pointer_cast<ds::Block>(value);
			InitializeTextures(dataBlock,info,loadFlags);
		}
	}
}

std::shared_ptr<CMaterial::CallbackInfo> CMaterial::InitializeCallbackInfo(const std::function<void(void)> &onAllTexturesLoaded,const std::function<void(std::shared_ptr<Texture>)> &onTextureLoaded)
{
	if(m_callbackInfo == nullptr)
	{
		m_callbackInfo = std::make_shared<CallbackInfo>();
		auto *info = m_callbackInfo.get();
		m_callbackInfo->onload = [info,onTextureLoaded,onAllTexturesLoaded](std::shared_ptr<Texture> texture) {
			--info->count;
			if(texture != nullptr && onTextureLoaded != nullptr)
				onTextureLoaded(texture);
			if(info->count == 0)
				onAllTexturesLoaded();
		};
	}
	return m_callbackInfo;
}

void CMaterial::InitializeTextures(const std::shared_ptr<ds::Block> &data,const std::function<void(void)> &onAllTexturesLoaded,const std::function<void(std::shared_ptr<Texture>)> &onTextureLoaded,TextureLoadFlags loadFlags)
{
	auto callbackInfo = InitializeCallbackInfo(onAllTexturesLoaded,onTextureLoaded);
	++callbackInfo->count; // Dummy, in case all textures are loaded immediately (In which case the final callback would be triggered multiple times.).
	InitializeTextures(data,callbackInfo,loadFlags);
	callbackInfo->onload(nullptr); // Clear dummy
}
