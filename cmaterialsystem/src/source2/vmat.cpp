/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DISABLE_VMAT_SUPPORT
#include "cmaterialmanager.h"
#include "cmaterial.h"
#include "shaders/source2/c_shader_generate_tangent_space_normal_map.hpp"
#include "shaders/source2/c_shader_decompose_metalness_reflectance.hpp"
#include "shaders/source2/c_shader_decompose_pbr.hpp"
#include "shaders/c_shader_ssbumpmap_to_normalmap.hpp"
#include "shaders/c_shader_extract_image_channel.hpp"
#include <prosper_context.hpp>
#include <prosper_util.hpp>
#include <image/prosper_render_target.hpp>
#include <image/prosper_image.hpp>
#include <prosper_command_buffer.hpp>
#include <prosper_descriptor_set_group.hpp>
#include <util_source2.hpp>
#include <source2/resource.hpp>
#include <source2/resource_data.hpp>
#include <sharedutils/util_file.h>
#include <util_texture_info.hpp>

#pragma optimize("",off)
#include "impl_texture_formats.h"

static std::shared_ptr<prosper::Texture> load_texture(CMaterialManager &matManager,const std::string &texPath)
{
	TextureManager::LoadInfo loadInfo {};
	loadInfo.flags = TextureLoadFlags::LoadInstantly;

	std::shared_ptr<void> map = nullptr;
	auto &textureManager = matManager.GetTextureManager();
	auto &context = matManager.GetContext();
	if(textureManager.Load(context,texPath,loadInfo,&map) == false)
		return nullptr;
	auto *pMap = static_cast<Texture*>(map.get());
	if(pMap == nullptr || pMap->HasValidVkTexture() == false)
		return nullptr;
	return pMap->GetVkTexture();
}

static std::shared_ptr<prosper::Texture> get_texture(CMaterialManager &matManager,ds::Block &rootData,const std::string &identifier,std::string *optOutName=nullptr)
{
	auto dsMap = std::dynamic_pointer_cast<ds::Texture>(rootData.GetValue(identifier));
	if(dsMap == nullptr)
		return nullptr;
	if(optOutName)
		*optOutName = dsMap->GetString();
	return load_texture(matManager,dsMap->GetString());
}

bool CMaterialManager::InitializeVMatData(source2::resource::Resource &resource,source2::resource::Material &vmat,LoadInfo &info,ds::Block &rootData,ds::Settings &settings,const std::string &shader)
{
	//TODO: These do not work if the textures haven't been imported yet!!
	if(MaterialManager::InitializeVMatData(resource,vmat,info,rootData,settings,shader) == false)
		return false;
	// TODO: Steam VR assets seem to behave differently than Source 2 assets.
	// How do we determine what type it is?
	constexpr auto isSteamVrModel = false;

	auto &context = GetContext();

	auto *shaderExtractImageChannel = static_cast<msys::ShaderExtractImageChannel*>(context.GetShader("extract_image_channel").get());
	if constexpr(isSteamVrModel)
	{
		auto dsMetalnessMap = std::dynamic_pointer_cast<ds::Texture>(rootData.GetValue("metalness_map"));
		if(dsMetalnessMap)
		{
			auto &context = GetContext();
			auto *shaderDecomposeMetalnessReflectance = static_cast<msys::source2::ShaderDecomposeMetalnessReflectance*>(context.GetShader("source2_decompose_metalness_reflectance").get());
			if(shaderDecomposeMetalnessReflectance)
			{
				auto &textureManager = GetTextureManager();
				TextureManager::LoadInfo loadInfo {};
				loadInfo.flags = TextureLoadFlags::LoadInstantly;

				std::shared_ptr<void> normalMap = nullptr;
				auto metalnessReflectancePath = dsMetalnessMap->GetString();
				if(textureManager.Load(context,metalnessReflectancePath,loadInfo,&normalMap))
				{
					auto *pMetalnessReflectanceMap = static_cast<Texture*>(normalMap.get());
					if(pMetalnessReflectanceMap && pMetalnessReflectanceMap->HasValidVkTexture())
					{
						prosper::util::ImageCreateInfo imgCreateInfo {};
						//imgCreateInfo.flags |= prosper::util::ImageCreateInfo::Flags::FullMipmapChain;
						imgCreateInfo.format = Anvil::Format::R16G16B16A16_SFLOAT;
						imgCreateInfo.memoryFeatures = prosper::util::MemoryFeatureFlags::GPUBulk;
						imgCreateInfo.postCreateLayout = Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
						imgCreateInfo.tiling = Anvil::ImageTiling::OPTIMAL;
						imgCreateInfo.usage = Anvil::ImageUsageFlagBits::COLOR_ATTACHMENT_BIT | Anvil::ImageUsageFlagBits::TRANSFER_SRC_BIT;

						imgCreateInfo.width = pMetalnessReflectanceMap->GetWidth();
						imgCreateInfo.height = pMetalnessReflectanceMap->GetHeight();
						auto &dev = context.GetDevice();
						auto imgMetalness = prosper::util::create_image(context.GetDevice(),imgCreateInfo);
						auto imgReflectance = prosper::util::create_image(context.GetDevice(),imgCreateInfo);

						prosper::util::ImageViewCreateInfo imgViewCreateInfo {};
						auto texMetalness = prosper::util::create_texture(dev,{},imgMetalness,&imgViewCreateInfo);
						auto texReflectance = prosper::util::create_texture(dev,{},imgReflectance,&imgViewCreateInfo);
						auto rt = prosper::util::create_render_target(dev,{texMetalness,texReflectance},shaderDecomposeMetalnessReflectance->GetRenderPass());

						auto dsg = shaderDecomposeMetalnessReflectance->CreateDescriptorSetGroup(msys::source2::ShaderGenerateTangentSpaceNormalMap::DESCRIPTOR_SET_TEXTURE.setIndex);
						auto &ds = *dsg->GetDescriptorSet();
						auto &vkMetalnessReflectanceTex = pMetalnessReflectanceMap->GetVkTexture();
						prosper::util::set_descriptor_set_binding_texture(ds,*vkMetalnessReflectanceTex,umath::to_integral(msys::source2::ShaderGenerateTangentSpaceNormalMap::TextureBinding::NormalMap));
						auto &setupCmd = context.GetSetupCommandBuffer();
						if(prosper::util::record_begin_render_pass(**setupCmd,*rt))
						{
							if(shaderDecomposeMetalnessReflectance->BeginDraw(setupCmd))
							{
								shaderDecomposeMetalnessReflectance->Draw(*ds);
								shaderDecomposeMetalnessReflectance->EndDraw();
							}
							prosper::util::record_end_render_pass(**setupCmd);
						}
						context.FlushSetupCommandBuffer();

						auto pathNoExt = metalnessReflectancePath;
						ufile::remove_extension_from_filename(pathNoExt);

						auto errHandler = [](const std::string &err) {
							std::cout<<"WARNING: Unable to save map image as DDS: "<<err<<std::endl;
						};

						// TODO: Change width/height
						auto rootPath = "addons/converted/" +MaterialManager::GetRootMaterialLocation();
						uimg::TextureInfo texInfo {};
						texInfo.containerFormat = uimg::TextureInfo::ContainerFormat::DDS;
						texInfo.alphaMode = uimg::TextureInfo::AlphaMode::Auto;
						texInfo.inputFormat = uimg::TextureInfo::InputFormat::R32G32B32A32_Float;
						texInfo.outputFormat = uimg::TextureInfo::OutputFormat::GradientMap;
						auto metalnessPath = pathNoExt +"_metalness";
						auto glossinessPath = pathNoExt +"_glossiness";
						prosper::util::save_texture(rootPath +'/' +metalnessPath,*texMetalness->GetImage(),texInfo,errHandler);
						prosper::util::save_texture(rootPath +'/' +glossinessPath,*texReflectance->GetImage(),texInfo,errHandler);

						// TODO: These should be Material::NORMAL_MAP_IDENTIFIER, but
						// for some reason the linker complains about unresolved symbols?
						rootData.AddData("metalness_map",std::make_shared<ds::Texture>(settings,metalnessPath));
						rootData.AddData("specular_map",std::make_shared<ds::Texture>(settings,glossinessPath));
					}
				}
			}
		}
	}
	else
	{
		auto *shaderDecomposePbr = static_cast<msys::source2::ShaderDecomposePBR*>(context.GetShader("source2_decompose_pbr").get());
		std::string texPath;
		auto albedoTex = get_texture(*this,rootData,"albedo_map",&texPath);
		auto normalTex = get_texture(*this,rootData,"normal_map");
		if(normalTex == nullptr)
			normalTex = load_texture(*this,"white");
		if(albedoTex && normalTex)
		{
			auto &textureManager = GetTextureManager();
			TextureManager::LoadInfo loadInfo {};
			loadInfo.flags = TextureLoadFlags::LoadInstantly;

			auto pathNoExt = texPath;
			ufile::remove_extension_from_filename(pathNoExt);

			// Decompose Source 2 textures into albedo and metalness-roughness
			auto pbrSet = shaderDecomposePbr->DecomposePBR(context,*albedoTex,*normalTex);

			auto albedoPath = pathNoExt +"_albedo";
			auto rootPath = "addons/converted/" +MaterialManager::GetRootMaterialLocation();
			uimg::TextureInfo texInfo {};
			texInfo.containerFormat = uimg::TextureInfo::ContainerFormat::DDS;
			texInfo.alphaMode = uimg::TextureInfo::AlphaMode::None;
			texInfo.flags = uimg::TextureInfo::Flags::GenerateMipmaps;
			texInfo.inputFormat = uimg::TextureInfo::InputFormat::R8G8B8A8_UInt;
			texInfo.outputFormat = uimg::TextureInfo::OutputFormat::ColorMap;
			prosper::util::save_texture(rootPath +'/' +albedoPath,*pbrSet.albedoMap,texInfo,[](const std::string &err) {
				std::cout<<"WARNING: Unable to save albedo image as DDS: "<<err<<std::endl;
			});

			auto metalnessRoughnessPath = pathNoExt +"_metallic_roughness";
			texInfo.outputFormat = uimg::TextureInfo::OutputFormat::GradientMap;
			prosper::util::save_texture(rootPath +'/' +metalnessRoughnessPath,*pbrSet.metallicRoughnessMap,texInfo,[](const std::string &err) {
				std::cout<<"WARNING: Unable to save metallic-roughness image as DDS: "<<err<<std::endl;
			});

			rootData.AddData("albedo_map",std::make_shared<ds::Texture>(settings,albedoPath));
			rootData.AddData("metalness_roughness_map",std::make_shared<ds::Texture>(settings,metalnessRoughnessPath));
			rootData.AddValue("float","metalness_factor","1.0");
			rootData.AddValue("float","roughness_factor","1.0");
		}
	}

	auto dsNormalMap = std::dynamic_pointer_cast<ds::Texture>(rootData.GetValue("normal_map"));
	if(dsNormalMap)
	{
		auto normalMapPath = dsNormalMap->GetString();
		TextureType nmType;
		auto found = false;
		auto path = translate_image_path(normalMapPath,nmType,MaterialManager::GetRootMaterialLocation() +'/',nullptr,&found);
		if(found && nmType == TextureType::VTex)
		{
			// TODO
			if(true)// constexpr(isSteamVrModel)
			{
				auto *shaderGenerateTangentSpaceNormalMap = static_cast<msys::source2::ShaderGenerateTangentSpaceNormalMap*>(context.GetShader("source2_generate_tangent_space_normal_map").get());
				if(shaderGenerateTangentSpaceNormalMap)
				{
					auto &textureManager = GetTextureManager();
					TextureManager::LoadInfo loadInfo {};
					loadInfo.flags = TextureLoadFlags::LoadInstantly;

					std::shared_ptr<void> normalMap = nullptr;
					if(textureManager.Load(context,normalMapPath,loadInfo,&normalMap))
					{
						auto *pNormalMap = static_cast<Texture*>(normalMap.get());
						if(pNormalMap && pNormalMap->HasValidVkTexture())
						{
							prosper::util::ImageCreateInfo imgCreateInfo {};
							//imgCreateInfo.flags |= prosper::util::ImageCreateInfo::Flags::FullMipmapChain;
							imgCreateInfo.format = Anvil::Format::R16G16B16A16_SFLOAT;
							imgCreateInfo.memoryFeatures = prosper::util::MemoryFeatureFlags::GPUBulk;
							imgCreateInfo.postCreateLayout = Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
							imgCreateInfo.tiling = Anvil::ImageTiling::OPTIMAL;
							imgCreateInfo.usage = Anvil::ImageUsageFlagBits::COLOR_ATTACHMENT_BIT | Anvil::ImageUsageFlagBits::TRANSFER_SRC_BIT;

							imgCreateInfo.width = pNormalMap->GetWidth();
							imgCreateInfo.height = pNormalMap->GetHeight();
							auto &dev = context.GetDevice();
							auto imgNormal = prosper::util::create_image(context.GetDevice(),imgCreateInfo);

							prosper::util::ImageViewCreateInfo imgViewCreateInfo {};
							auto texNormal = prosper::util::create_texture(dev,{},imgNormal,&imgViewCreateInfo);
							auto rt = prosper::util::create_render_target(dev,{texNormal},shaderGenerateTangentSpaceNormalMap->GetRenderPass());

							auto dsg = shaderGenerateTangentSpaceNormalMap->CreateDescriptorSetGroup(msys::source2::ShaderGenerateTangentSpaceNormalMap::DESCRIPTOR_SET_TEXTURE.setIndex);
							auto &ds = *dsg->GetDescriptorSet();
							auto &vkNormalTex = pNormalMap->GetVkTexture();
							prosper::util::set_descriptor_set_binding_texture(ds,*vkNormalTex,umath::to_integral(msys::source2::ShaderGenerateTangentSpaceNormalMap::TextureBinding::NormalMap));
							auto &setupCmd = context.GetSetupCommandBuffer();
							if(prosper::util::record_begin_render_pass(**setupCmd,*rt))
							{
								if(shaderGenerateTangentSpaceNormalMap->BeginDraw(setupCmd))
								{
									shaderGenerateTangentSpaceNormalMap->Draw(*ds);
									shaderGenerateTangentSpaceNormalMap->EndDraw();
								}
								prosper::util::record_end_render_pass(**setupCmd);
							}
							context.FlushSetupCommandBuffer();

							auto normalMapPathNoExt = normalMapPath;
							ufile::remove_extension_from_filename(normalMapPathNoExt);

							auto errHandler = [](const std::string &err) {
								std::cout<<"WARNING: Unable to save normal map image as DDS: "<<err<<std::endl;
							};

							// TODO: Change width/height
							auto rootPath = "addons/converted/" +MaterialManager::GetRootMaterialLocation();
							uimg::TextureInfo texInfo {};
							texInfo.containerFormat = uimg::TextureInfo::ContainerFormat::DDS;
							texInfo.alphaMode = uimg::TextureInfo::AlphaMode::Auto;
							texInfo.flags = uimg::TextureInfo::Flags::NormalMap;
							texInfo.inputFormat = uimg::TextureInfo::InputFormat::R32G32B32A32_Float;
							texInfo.outputFormat = uimg::TextureInfo::OutputFormat::NormalMap;
							prosper::util::save_texture(rootPath +'/' +normalMapPathNoExt,*texNormal->GetImage(),texInfo,errHandler);
						}
					}
				}
			}
			else
			{
				// Note: Source 2 normal maps always seem to be self-shadowed bumpmaps?
				auto *shaderSSBumpMapToNormalMap = static_cast<msys::ShaderSSBumpMapToNormalMap*>(context.GetShader("ssbumpmap_to_normalmap").get());
				if(shaderSSBumpMapToNormalMap)
				{
					auto &textureManager = GetTextureManager();
					TextureManager::LoadInfo loadInfo {};
					loadInfo.flags = TextureLoadFlags::LoadInstantly;

					std::shared_ptr<void> bumpMap = nullptr;
					textureManager.Load(context,normalMapPath,loadInfo,&bumpMap);

					auto *pBumpMap = static_cast<Texture*>(bumpMap.get());
					if(pBumpMap && pBumpMap->HasValidVkTexture())
					{
						// Prepare output texture (normal map)
						prosper::util::ImageCreateInfo imgCreateInfo {};
						//imgCreateInfo.flags |= prosper::util::ImageCreateInfo::Flags::FullMipmapChain;
						imgCreateInfo.format = Anvil::Format::R16G16B16A16_SFLOAT;
						imgCreateInfo.memoryFeatures = prosper::util::MemoryFeatureFlags::GPUBulk;
						imgCreateInfo.postCreateLayout = Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
						imgCreateInfo.tiling = Anvil::ImageTiling::OPTIMAL;
						imgCreateInfo.usage = Anvil::ImageUsageFlagBits::COLOR_ATTACHMENT_BIT | Anvil::ImageUsageFlagBits::TRANSFER_SRC_BIT;

						imgCreateInfo.width = pBumpMap->GetWidth();
						imgCreateInfo.height = pBumpMap->GetHeight();
						auto &dev = context.GetDevice();
						auto imgNormal = prosper::util::create_image(dev,imgCreateInfo);

						prosper::util::ImageViewCreateInfo imgViewCreateInfo {};
						auto texNormal = prosper::util::create_texture(dev,{},imgNormal,&imgViewCreateInfo);
						auto rt = prosper::util::create_render_target(dev,{texNormal},shaderSSBumpMapToNormalMap->GetRenderPass());

						auto dsg = shaderSSBumpMapToNormalMap->CreateDescriptorSetGroup(msys::ShaderSSBumpMapToNormalMap::DESCRIPTOR_SET_TEXTURE.setIndex);
						auto &ds = *dsg->GetDescriptorSet();
						auto &vkBumpMapTex = pBumpMap->GetVkTexture();
						prosper::util::set_descriptor_set_binding_texture(ds,*vkBumpMapTex,umath::to_integral(msys::ShaderSSBumpMapToNormalMap::TextureBinding::SSBumpMap));
						auto &setupCmd = context.GetSetupCommandBuffer();
						if(prosper::util::record_begin_render_pass(**setupCmd,*rt))
						{
							if(shaderSSBumpMapToNormalMap->BeginDraw(setupCmd))
							{
								shaderSSBumpMapToNormalMap->Draw(*ds);
								shaderSSBumpMapToNormalMap->EndDraw();
							}
							prosper::util::record_end_render_pass(**setupCmd);
						}
						context.FlushSetupCommandBuffer();

						auto bumpMapTextureNoExt = normalMapPath;
						ufile::remove_extension_from_filename(bumpMapTextureNoExt);

						auto errHandler = [](const std::string &err) {
							std::cout<<"WARNING: Unable to save converted source 2 normal map as DDS: "<<err<<std::endl;
						};

						auto rootPath = "addons/converted/" +MaterialManager::GetRootMaterialLocation();
						uimg::TextureInfo texInfo {};
						texInfo.containerFormat = uimg::TextureInfo::ContainerFormat::DDS;
						texInfo.alphaMode = uimg::TextureInfo::AlphaMode::None;
						texInfo.flags = uimg::TextureInfo::Flags::GenerateMipmaps | uimg::TextureInfo::Flags::NormalMap;
						texInfo.inputFormat = uimg::TextureInfo::InputFormat::R32G32B32A32_Float;
						texInfo.outputFormat = uimg::TextureInfo::OutputFormat::NormalMap;
						prosper::util::save_texture(rootPath +'/' +bumpMapTextureNoExt,*texNormal->GetImage(),texInfo,errHandler);

						// Extract roughness (blue channel of normal map)
						auto roughnessPath = bumpMapTextureNoExt +"_roughness";
						auto imgRoughness = shaderExtractImageChannel->ExtractImageChannel(
							context,*texNormal,
							msys::ShaderExtractImageChannel::Channel::Blue,msys::ShaderExtractImageChannel::Pipeline::RGBA32
						);
						umath::set_flag(texInfo.flags,uimg::TextureInfo::Flags::NormalMap,false);
						texInfo.outputFormat = uimg::TextureInfo::OutputFormat::GradientMap;
						prosper::util::save_texture(rootPath +'/' +roughnessPath,*imgRoughness,texInfo,errHandler);

						rootData.AddData("roughness_map",std::make_shared<ds::Texture>(settings,roughnessPath));
					}
				}
			}
		}
	}

	info.saveOnDisk = true;
	GetTextureManager().ClearUnused();
	return true;
}
#pragma optimize("",on)
#endif
