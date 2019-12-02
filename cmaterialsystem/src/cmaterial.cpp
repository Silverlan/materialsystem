/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <sharedutils/util_shaderinfo.hpp>
#include "cmaterialmanager.h"
#include "cmaterial.h"
#include "texture_type.h"
#include <prosper_util.hpp>
#include <prosper_context.hpp>
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
{}
CMaterial::CMaterial(MaterialManager &manager,const std::string &shader,const std::shared_ptr<ds::Block> &data)
	: Material(manager,shader,data)
{}

CMaterial::~CMaterial()
{}

Material *CMaterial::Copy() const {return Material::Copy<CMaterial>();}

void CMaterial::InitializeSampler()
{
	prosper::util::SamplerCreateInfo samplerInfo {};

	auto bUseCustomSampler = false;

	const std::unordered_map<std::string,int32_t> addressModes = {
		{"ADDRESS_MODE_REPEAT",umath::to_integral(vk::SamplerAddressMode::eRepeat)},
		{"ADDRESS_MODE_MIRRORED_REPEAT",umath::to_integral(vk::SamplerAddressMode::eMirroredRepeat)},
		{"ADDRESS_MODE_CLAMP_TO_EDGE",umath::to_integral(vk::SamplerAddressMode::eClampToEdge)},
		{"ADDRESS_MODE_CLAMP_TO_BORDER",umath::to_integral(vk::SamplerAddressMode::eClampToBorder)},
		{"ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE",umath::to_integral(vk::SamplerAddressMode::eMirrorClampToEdge)}
	};

	const std::unordered_map<std::string,int32_t> borderColors = {
		{"BORDER_COLOR_FLOAT_TRANSPARENT_BLACK",umath::to_integral(vk::BorderColor::eFloatTransparentBlack)},
		{"BORDER_COLOR_INT_TRANSPARENT_BLACK",umath::to_integral(vk::BorderColor::eIntTransparentBlack)},
		{"BORDER_COLOR_FLOAT_OPAQUE_BLACK",umath::to_integral(vk::BorderColor::eFloatOpaqueBlack)},
		{"BORDER_COLOR_INT_OPAQUE_BLACK",umath::to_integral(vk::BorderColor::eIntOpaqueBlack)},
		{"BORDER_COLOR_FLOAT_OPAQUE_WHITE",umath::to_integral(vk::BorderColor::eFloatOpaqueWhite)},
		{"BORDER_COLOR_INT_OPAQUE_WHITE",umath::to_integral(vk::BorderColor::eIntOpaqueWhite)}
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
		samplerInfo.addressModeU = static_cast<Anvil::SamplerAddressMode>(intVal);
	}
	if(fGetAddressMode("address_mode_v",intVal) == true)
	{
		bUseCustomSampler = true;
		samplerInfo.addressModeV = static_cast<Anvil::SamplerAddressMode>(intVal);
	}
	if(fGetAddressMode("address_mode_w",intVal) == true)
	{
		bUseCustomSampler = true;
		samplerInfo.addressModeW = static_cast<Anvil::SamplerAddressMode>(intVal);
	}
	if(fGetBorderColor("border_color",intVal) == true)
	{
		bUseCustomSampler = true;
		samplerInfo.borderColor = static_cast<Anvil::BorderColor>(intVal);
	}
	if(bUseCustomSampler == true)
	{
		auto mipmapMode = static_cast<TextureMipmapMode>(GetMipmapMode(m_data));
		TextureManager::SetupSamplerMipmapMode(samplerInfo,mipmapMode);
		m_sampler = prosper::util::create_sampler(GetContext().GetDevice(),samplerInfo);
	}
}

std::shared_ptr<prosper::Sampler> CMaterial::GetSampler() {return m_sampler;}

prosper::Buffer *CMaterial::GetSettingsBuffer() {return m_settingsBuffer.get();}
void CMaterial::SetSettingsBuffer(prosper::Buffer &buffer) {m_settingsBuffer = buffer.shared_from_this();}

prosper::Context &CMaterial::GetContext() {return static_cast<CMaterialManager&>(m_manager).GetContext();}
TextureManager &CMaterial::GetTextureManager() {return static_cast<CMaterialManager&>(m_manager).GetTextureManager();}
std::unordered_map<util::WeakHandle<prosper::Shader>,std::shared_ptr<prosper::DescriptorSetGroup>,CMaterial::ShaderHash,CMaterial::ShaderEqualFn>::iterator CMaterial::FindShaderDescriptorSetGroup(prosper::Shader &shader)
{
	return std::find_if(m_descriptorSetGroups.begin(),m_descriptorSetGroups.end(),[&shader](const std::pair<util::WeakHandle<prosper::Shader>,std::shared_ptr<prosper::DescriptorSetGroup>> &pair) {
		return pair.first.get() == &shader;
	});
}
std::unordered_map<util::WeakHandle<prosper::Shader>,std::shared_ptr<prosper::DescriptorSetGroup>,CMaterial::ShaderHash,CMaterial::ShaderEqualFn>::const_iterator CMaterial::FindShaderDescriptorSetGroup(prosper::Shader &shader) const {return const_cast<CMaterial*>(this)->FindShaderDescriptorSetGroup(shader);}
const std::shared_ptr<prosper::DescriptorSetGroup> &CMaterial::GetDescriptorSetGroup(prosper::Shader &shader) const
{
	static std::shared_ptr<prosper::DescriptorSetGroup> nptr = nullptr;
	auto it = FindShaderDescriptorSetGroup(shader);
	return (it != m_descriptorSetGroups.end()) ? it->second : nptr;
}
bool CMaterial::IsInitialized() const {return (IsLoaded() && m_descriptorSetGroups.empty() == false) ? true : false;}
util::WeakHandle<prosper::Shader> CMaterial::GetPrimaryShader()
{
	auto &context = GetContext();
	auto &shaderManager = context.GetShaderManager();
	return shaderManager.GetShader(GetShaderIdentifier());
}
void CMaterial::SetDescriptorSetGroup(prosper::Shader &shader,const std::shared_ptr<prosper::DescriptorSetGroup> &descSetGroup)
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

	SetLoaded(false);
	auto callbackInfo = InitializeCallbackInfo([this]() {
		SetLoaded(true);
		if(m_fOnLoaded != nullptr)
			m_fOnLoaded();
		UpdateTextures();
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
