/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2021 Silverlan
 */

#include "matsysdefinitions.h"
#include "c_source2_vmat_format_handler.hpp"
#include "cmaterial_manager2.hpp"
#include "shaders/source2/c_shader_generate_tangent_space_normal_map.hpp"
#include "shaders/source2/c_shader_decompose_metalness_reflectance.hpp"
#include "shaders/source2/c_shader_decompose_pbr.hpp"
#include "shaders/c_shader_ssbumpmap_to_normalmap.hpp"
#include "shaders/c_shader_extract_image_channel.hpp"
#include "texturemanager/texture_manager2.hpp"
#include "texturemanager/texture.h"
#include <prosper_context.hpp>
#include <prosper_util.hpp>
#include <image/prosper_render_target.hpp>
#include <image/prosper_image.hpp>
#include <prosper_command_buffer.hpp>
#include <prosper_descriptor_set_group.hpp>
#include <sharedutils/util_file.h>
#include <sharedutils/util_path.hpp>
#include <util_texture_info.hpp>
#include <util_vmat.hpp>
#include <sharedutils/alpha_mode.hpp>
#include "impl_texture_formats.h"

#ifndef DISABLE_VMAT_SUPPORT
import source2;

static bool g_downScaleRMATextures = true;
static std::shared_ptr<prosper::Texture> load_texture(msys::CMaterialManager &matManager, const std::string &texPath, bool reload = false)
{
	auto &textureManager = matManager.GetTextureManager();
	auto &context = matManager.GetContext();
	auto map = reload ? textureManager.ReloadAsset(texPath) : textureManager.LoadAsset(texPath);
	if(map == nullptr || map->HasValidVkTexture() == false)
		return nullptr;
	return map->GetVkTexture();
}

static std::shared_ptr<prosper::Texture> get_texture(msys::CMaterialManager &matManager, ds::Block &rootData, const std::string &identifier, std::string *optOutName = nullptr)
{
	auto dsMap = std::dynamic_pointer_cast<ds::Texture>(rootData.GetValue(identifier));
	if(dsMap == nullptr)
		return nullptr;
	if(optOutName)
		*optOutName = dsMap->GetString();
	return load_texture(matManager, dsMap->GetString());
}

msys::CSource2VmatFormatHandler::CSource2VmatFormatHandler(util::IAssetManager &assetManager) : Source2VmatFormatHandler {assetManager} {}
bool msys::CSource2VmatFormatHandler::ImportTexture(const std::string &fpath, const std::string &outputPath)
{
	auto importHandler = static_cast<CMaterialManager &>(GetAssetManager()).GetExternalSourceFileImportHandler();
	if(!importHandler)
		return false;
	return importHandler(fpath, outputPath).has_value();
}
bool msys::CSource2VmatFormatHandler::InitializeVMatData(::source2::resource::Resource &resource, ::source2::resource::Material &vmat, ds::Block &rootData, ds::Settings &settings, const std::string &shader, VMatOrigin origin)
{
	//TODO: These do not work if the textures haven't been imported yet!!
	if(Source2VmatFormatHandler::InitializeVMatData(resource, vmat, rootData, settings, shader, origin) == false)
		return false;
	// TODO: Steam VR assets seem to behave differently than Source 2 assets.
	// How do we determine what type it is?
	auto isSteamVrMat = (origin == VMatOrigin::SteamVR);
	auto isDota2Mat = (origin == VMatOrigin::Dota2);
	auto isSource2Mat = (origin == VMatOrigin::Source2);

	auto &matManager = static_cast<CMaterialManager &>(GetAssetManager());
	auto rootPath = matManager.GetImportDirectory();
	auto &context = matManager.GetContext();

	auto *shaderExtractImageChannel = static_cast<msys::ShaderExtractImageChannel *>(context.GetShader("extract_image_channel").get());
	if(isSteamVrMat) {
		auto *metalnessMap = vmat.FindTextureParam("g_tMetalnessReflectance");
		if(metalnessMap) {
			auto &context = matManager.GetContext();
			auto *shaderDecomposeMetalnessReflectance = static_cast<msys::source2::ShaderDecomposeMetalnessReflectance *>(context.GetShader("source2_decompose_metalness_reflectance").get());
			if(shaderDecomposeMetalnessReflectance) {
				auto &textureManager = matManager.GetTextureManager();

				std::shared_ptr<void> normalMap = nullptr;
				auto metalnessReflectancePath = vmat::get_vmat_texture_path(*metalnessMap).GetString();
				auto pMetalnessReflectanceMap = textureManager.LoadAsset(metalnessReflectancePath);
				if(pMetalnessReflectanceMap && pMetalnessReflectanceMap->HasValidVkTexture()) {
					prosper::util::ImageCreateInfo imgCreateInfo {};
					//imgCreateInfo.flags |= prosper::util::ImageCreateInfo::Flags::FullMipmapChain;
					imgCreateInfo.format = prosper::Format::R8G8B8A8_UNorm;
					imgCreateInfo.memoryFeatures = prosper::MemoryFeatureFlags::GPUBulk;
					imgCreateInfo.postCreateLayout = prosper::ImageLayout::ColorAttachmentOptimal;
					imgCreateInfo.tiling = prosper::ImageTiling::Optimal;
					imgCreateInfo.usage = prosper::ImageUsageFlags::ColorAttachmentBit | prosper::ImageUsageFlags::TransferSrcBit;

					imgCreateInfo.width = pMetalnessReflectanceMap->GetWidth();
					imgCreateInfo.height = pMetalnessReflectanceMap->GetHeight();
					auto imgRMA = context.CreateImage(imgCreateInfo);

					prosper::util::ImageViewCreateInfo imgViewCreateInfo {};
					auto texRMA = context.CreateTexture({}, *imgRMA, imgViewCreateInfo);
					auto rt = context.CreateRenderTarget({texRMA}, shaderDecomposeMetalnessReflectance->GetRenderPass());

					auto dsg = shaderDecomposeMetalnessReflectance->CreateDescriptorSetGroup(msys::source2::ShaderGenerateTangentSpaceNormalMap::DESCRIPTOR_SET_TEXTURE.setIndex);
					auto &ds = *dsg->GetDescriptorSet();
					auto &vkMetalnessReflectanceTex = pMetalnessReflectanceMap->GetVkTexture();
					ds.SetBindingTexture(*vkMetalnessReflectanceTex, umath::to_integral(msys::source2::ShaderGenerateTangentSpaceNormalMap::TextureBinding::NormalMap));
					auto &setupCmd = context.GetSetupCommandBuffer();
					if(setupCmd->RecordBeginRenderPass(*rt)) {
						prosper::ShaderBindState bindState {*setupCmd};
						if(shaderDecomposeMetalnessReflectance->RecordBeginDraw(bindState)) {
							shaderDecomposeMetalnessReflectance->RecordDraw(bindState, ds);
							shaderDecomposeMetalnessReflectance->RecordEndDraw(bindState);
						}
						setupCmd->RecordEndRenderPass();
					}
					context.FlushSetupCommandBuffer();

					auto pathNoExt = metalnessReflectancePath;
					ufile::remove_extension_from_filename(pathNoExt);

					auto errHandler = [](const std::string &err) { std::cout << "WARNING: Unable to save map image as DDS: " << err << std::endl; };

					// TODO: Change width/height
					uimg::TextureInfo texInfo {};
					texInfo.containerFormat = uimg::TextureInfo::ContainerFormat::DDS;
					texInfo.alphaMode = uimg::TextureInfo::AlphaMode::Auto;
					texInfo.inputFormat = uimg::TextureInfo::InputFormat::R8G8B8A8_UInt;
					texInfo.outputFormat = uimg::TextureInfo::OutputFormat::ColorMap;
					auto rmaPath = pathNoExt + "_rma";
					prosper::util::save_texture((rootPath + ('/' + rmaPath)).GetString(), texRMA->GetImage(), texInfo, errHandler);

					rootData.AddData("rma_map", std::make_shared<ds::Texture>(settings, rmaPath));

					auto rmaInfo = rootData.AddBlock("rma_info");
					rmaInfo->AddValue("bool", "requires_ao_update", "1");
				}
			}
		}
	}
	else if(isDota2Mat) {
		std::cout << "TODO" << std::endl;
	}
	else {
		auto *shaderDecomposePbr = static_cast<msys::source2::ShaderDecomposePBR *>(context.GetShader("source2_decompose_pbr").get());
		std::string texPath;
		auto albedoTex = get_texture(matManager, rootData, "albedo_map", &texPath);
		auto normalTex = get_texture(matManager, rootData, "normal_map");
		if(normalTex == nullptr)
			normalTex = load_texture(matManager, "white");
		if(albedoTex && normalTex) {
			auto &textureManager = matManager.GetTextureManager();

			auto pathNoExt = texPath;
			ufile::remove_extension_from_filename(pathNoExt);

			prosper::Texture *anisoGlossMap = nullptr;

			// Decompose Source 2 textures into albedo and metalness-roughness
			auto *alphaTest = vmat.FindIntParam("F_ALPHA_TEST");
			auto *translucent = vmat.FindIntParam("F_TRANSLUCENT");
			auto *specular = vmat.FindIntParam("F_SPECULAR");
			auto flags = msys::source2::ShaderDecomposePBR::Flags::None;
			if((alphaTest && *alphaTest) || (translucent && *translucent))
				flags |= msys::source2::ShaderDecomposePBR::Flags::TreatAlphaAsTransparency;
			if(specular && *specular) {
				flags |= msys::source2::ShaderDecomposePBR::Flags::SpecularWorkflow;

				auto *s2AnisoGlossMap = vmat.FindTextureParam("g_tSelfIllumMask");
				if(s2AnisoGlossMap) {
					::util::Path path {*s2AnisoGlossMap};
					path.RemoveFileExtension();
					path += ".vtex_c";
					path.PopFront();

					anisoGlossMap = load_texture(matManager, path.GetString()).get();
					if(anisoGlossMap == nullptr)
						umath::set_flag(flags, msys::source2::ShaderDecomposePBR::Flags::SpecularWorkflow, false);
				}
				else
					anisoGlossMap = normalTex.get();
			}

			// Note: While the original roughness and metalness maps have the
			// same resolution as the albedo or normal maps (which is usually quite high),
			// we'll have to lower resolution, since we store them as a separate map
			// and it would require too much GPU memory otherwise.
			prosper::Extent2D metallicRoughnessResolution {};
			auto *s2AoMap = vmat.FindTextureParam("g_tAmbientOcclusion");
			prosper::Texture *aoTex = nullptr;
			if(s2AoMap) {
				::util::Path path {*s2AoMap};
				path.RemoveFileExtension();
				path += ".vtex_c";

				aoTex = load_texture(matManager, path.GetString()).get();
			}

			auto hasAoMap = true;
			if(aoTex == nullptr) {
				aoTex = load_texture(matManager, "white").get();
				hasAoMap = false;

				auto &albedoImg = albedoTex->GetImage();
				auto extents = albedoImg.GetExtents();
				metallicRoughnessResolution = {static_cast<uint32_t>(extents.width) / 4, static_cast<uint32_t>(extents.height) / 4};
				if(metallicRoughnessResolution.width == 0)
					metallicRoughnessResolution.width = 0;
				if(metallicRoughnessResolution.height == 0)
					metallicRoughnessResolution.height = 0;
			}
			else {
				auto &aoImg = aoTex->GetImage();
				auto extents = aoImg.GetExtents();
				metallicRoughnessResolution = {static_cast<uint32_t>(extents.width), static_cast<uint32_t>(extents.height)};
			}

			auto pbrSet = shaderDecomposePbr->DecomposePBR(context, *albedoTex, *normalTex, *aoTex, flags, anisoGlossMap);

			auto albedoPath = pathNoExt + "_albedo";
			uimg::TextureInfo texInfo {};
			texInfo.containerFormat = uimg::TextureInfo::ContainerFormat::DDS;
			texInfo.alphaMode = uimg::TextureInfo::AlphaMode::None;
			texInfo.outputFormat = uimg::TextureInfo::OutputFormat::ColorMap;
			auto useAlpha = umath::is_flag_set(flags, msys::source2::ShaderDecomposePBR::Flags::TreatAlphaAsTransparency);
			if(useAlpha) {
				texInfo.alphaMode = uimg::TextureInfo::AlphaMode::Transparency;
				texInfo.outputFormat = uimg::TextureInfo::OutputFormat::ColorMapSmoothAlpha;
			}
			texInfo.flags = uimg::TextureInfo::Flags::GenerateMipmaps;
			texInfo.inputFormat = uimg::TextureInfo::InputFormat::R8G8B8A8_UInt;
			prosper::util::save_texture((rootPath + ('/' + albedoPath)).GetString(), *pbrSet.albedoMap, texInfo, [](const std::string &err) { std::cout << "WARNING: Unable to save albedo image as DDS: " << err << std::endl; });

			// TODO
			if(metallicRoughnessResolution.width > 1'024)
				metallicRoughnessResolution.width = 1'024;
			if(metallicRoughnessResolution.height > 1'024)
				metallicRoughnessResolution.height = 1'024;

			auto mrExtents = pbrSet.rmaMap->GetExtents();
			if(g_downScaleRMATextures && mrExtents.width > metallicRoughnessResolution.width && mrExtents.height > metallicRoughnessResolution.height) {
				std::cout << "Downscaling RMA map from " << mrExtents.width << "x" << mrExtents.height << " to " << metallicRoughnessResolution.width << "x" << metallicRoughnessResolution.height << std::endl;
				prosper::util::ImageCreateInfo imgCreateInfo {};
				imgCreateInfo.format = prosper::Format::R8G8B8A8_UNorm;
				imgCreateInfo.memoryFeatures = prosper::MemoryFeatureFlags::GPUBulk;
				imgCreateInfo.postCreateLayout = prosper::ImageLayout::TransferDstOptimal;
				imgCreateInfo.tiling = prosper::ImageTiling::Optimal;
				imgCreateInfo.usage = prosper::ImageUsageFlags::TransferDstBit;

				imgCreateInfo.width = metallicRoughnessResolution.width;
				imgCreateInfo.height = metallicRoughnessResolution.height;
				auto imgRescaled = context.CreateImage(imgCreateInfo);
				auto &setupCmd = context.GetSetupCommandBuffer();
				prosper::util::BlitInfo blitInfo {};
				blitInfo.extentsSrc = mrExtents;
				blitInfo.extentsDst = metallicRoughnessResolution;
				setupCmd->RecordImageBarrier(*pbrSet.rmaMap, prosper::ImageLayout::ShaderReadOnlyOptimal, prosper::ImageLayout::TransferSrcOptimal);
				setupCmd->RecordBlitImage(blitInfo, *pbrSet.rmaMap, *imgRescaled);
				context.FlushSetupCommandBuffer();

				pbrSet.rmaMap = imgRescaled;
			}

			auto metalnessRoughnessPath = pathNoExt + "_rma";
			texInfo.outputFormat = uimg::TextureInfo::OutputFormat::ColorMap;
			prosper::util::save_texture((rootPath + ('/' + metalnessRoughnessPath)).GetString(), *pbrSet.rmaMap, texInfo, [](const std::string &err) { std::cout << "WARNING: Unable to save RMA image as DDS: " << err << std::endl; });

			rootData.AddData("albedo_map", std::make_shared<ds::Texture>(settings, albedoPath));
			rootData.AddData("rma_map", std::make_shared<ds::Texture>(settings, metalnessRoughnessPath));

			if(hasAoMap == false) {
				auto rmaInfo = rootData.AddBlock("rma_info");
				rmaInfo->AddValue("bool", "requires_ao_update", "1");
			}

			// Ao map is now in rma map, so we won't need it anymore
			auto aoValue = rootData.GetValue("ao_map");
			if(aoValue)
				rootData.DetachData(*aoValue);

			if(useAlpha)
				rootData.AddValue("int", "alpha_mode", std::to_string(umath::to_integral(AlphaMode::Blend)));
		}
	}

	auto dsNormalMap = std::dynamic_pointer_cast<ds::Texture>(rootData.GetValue("normal_map"));
	if(dsNormalMap) {
		auto normalMapPath = dsNormalMap->GetString();
		auto path = matManager.GetTextureManager().FindAssetFilePath(normalMapPath);
		auto isVtexFormat = false;
		if(path.has_value()) {
			std::string ext;
			if(ufile::get_extension(*path, &ext) && (ext == "vtex_c"))
				isVtexFormat = true;
		}
		if(isVtexFormat) {
			//if(isSteamVrMat || isDota2Mat)
			{
				std::string texPath;
				auto albedoTex = get_texture(matManager, rootData, "albedo_map", &texPath);
				if(albedoTex) {
					auto pathNoExt = texPath;
					ufile::remove_extension_from_filename(pathNoExt);

					auto albedoPath = pathNoExt;
					uimg::TextureInfo texInfo {};
					texInfo.containerFormat = uimg::TextureInfo::ContainerFormat::DDS;
					texInfo.alphaMode = uimg::TextureInfo::AlphaMode::None;
					texInfo.outputFormat = uimg::TextureInfo::OutputFormat::ColorMap;
					texInfo.flags = uimg::TextureInfo::Flags::GenerateMipmaps;
					texInfo.inputFormat = uimg::TextureInfo::InputFormat::R8G8B8A8_UInt;
					prosper::util::save_texture((rootPath + ('/' + albedoPath)).GetString(), albedoTex->GetImage(), texInfo, [](const std::string &err) { std::cout << "WARNING: Unable to save albedo image as DDS: " << err << std::endl; });
				}

				msys::source2::ShaderGenerateTangentSpaceNormalMap *shaderGenerateTangentSpaceNormalMap = nullptr;
				if(isSteamVrMat || isDota2Mat)
					shaderGenerateTangentSpaceNormalMap = static_cast<msys::source2::ShaderGenerateTangentSpaceNormalMapProto *>(context.GetShader("source2_generate_tangent_space_normal_map_proto").get());
				else
					shaderGenerateTangentSpaceNormalMap = static_cast<msys::source2::ShaderGenerateTangentSpaceNormalMap *>(context.GetShader("source2_generate_tangent_space_normal_map").get());
				if(shaderGenerateTangentSpaceNormalMap) {
					auto &textureManager = matManager.GetTextureManager();

					auto pNormalMap = textureManager.LoadAsset(normalMapPath);
					if(pNormalMap && pNormalMap->HasValidVkTexture()) {
						prosper::util::ImageCreateInfo imgCreateInfo {};
						//imgCreateInfo.flags |= prosper::util::ImageCreateInfo::Flags::FullMipmapChain;
						imgCreateInfo.format = prosper::Format::R16G16B16A16_SFloat;
						imgCreateInfo.memoryFeatures = prosper::MemoryFeatureFlags::GPUBulk;
						imgCreateInfo.postCreateLayout = prosper::ImageLayout::ColorAttachmentOptimal;
						imgCreateInfo.tiling = prosper::ImageTiling::Optimal;
						imgCreateInfo.usage = prosper::ImageUsageFlags::ColorAttachmentBit | prosper::ImageUsageFlags::TransferSrcBit;

						imgCreateInfo.width = pNormalMap->GetWidth();
						imgCreateInfo.height = pNormalMap->GetHeight();
						auto imgNormal = context.CreateImage(imgCreateInfo);

						prosper::util::ImageViewCreateInfo imgViewCreateInfo {};
						auto texNormal = context.CreateTexture({}, *imgNormal, imgViewCreateInfo);
						auto rt = context.CreateRenderTarget({texNormal}, shaderGenerateTangentSpaceNormalMap->GetRenderPass());

						auto dsg = shaderGenerateTangentSpaceNormalMap->CreateDescriptorSetGroup(msys::source2::ShaderGenerateTangentSpaceNormalMap::DESCRIPTOR_SET_TEXTURE.setIndex);
						auto &ds = *dsg->GetDescriptorSet();
						auto &vkNormalTex = pNormalMap->GetVkTexture();
						ds.SetBindingTexture(*vkNormalTex, umath::to_integral(msys::source2::ShaderGenerateTangentSpaceNormalMap::TextureBinding::NormalMap));
						auto &setupCmd = context.GetSetupCommandBuffer();
						if(setupCmd->RecordBeginRenderPass(*rt)) {
							prosper::ShaderBindState bindState {*setupCmd};
							if(shaderGenerateTangentSpaceNormalMap->RecordBeginDraw(bindState)) {
								shaderGenerateTangentSpaceNormalMap->RecordDraw(bindState, ds);
								shaderGenerateTangentSpaceNormalMap->RecordEndDraw(bindState);
							}
							setupCmd->RecordEndRenderPass();
						}
						context.FlushSetupCommandBuffer();

						auto normalMapPathNoExt = normalMapPath;
						ufile::remove_extension_from_filename(normalMapPathNoExt);

						auto errHandler = [](const std::string &err) { std::cout << "WARNING: Unable to save normal map image as DDS: " << err << std::endl; };

						// TODO: Change width/height
						uimg::TextureInfo texInfo {};
						texInfo.containerFormat = uimg::TextureInfo::ContainerFormat::DDS;
						texInfo.alphaMode = uimg::TextureInfo::AlphaMode::Auto;
						texInfo.inputFormat = uimg::TextureInfo::InputFormat::R16G16B16A16_Float;
						texInfo.outputFormat = uimg::TextureInfo::OutputFormat::NormalMap;
						texInfo.SetNormalMap();
						prosper::util::save_texture((rootPath + ('/' + normalMapPathNoExt)).GetString(), texNormal->GetImage(), texInfo, errHandler);

						load_texture(matManager, normalMapPathNoExt, true);
						rootData.AddData("normal_map", std::make_shared<ds::Texture>(settings, normalMapPathNoExt));
					}
				}
			}
#if 0
			// Obsolete?
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
						texInfo.flags = uimg::TextureInfo::Flags::GenerateMipmaps;
						texInfo.inputFormat = uimg::TextureInfo::InputFormat::R16G16B16A16_Float;
						texInfo.outputFormat = uimg::TextureInfo::OutputFormat::NormalMap;
						auto nmapTexInfo = texInfo;
						nmapTexInfo.SetNormalMap();
						prosper::util::save_texture(rootPath +'/' +bumpMapTextureNoExt,*texNormal->GetImage(),nmapTexInfo,errHandler);

						// Reload the normal map
						load_texture(*this,bumpMapTextureNoExt,true);
						rootData.AddData("normal_map",std::make_shared<ds::Texture>(settings,bumpMapTextureNoExt));

						// Extract roughness (blue channel of normal map)
						auto rmaPath = bumpMapTextureNoExt +"_rma";
						auto imgRma = shaderExtractImageChannel->ExtractImageChannel(
							context,*texNormal,
							std::array<msys::ShaderExtractImageChannel::Channel,4>{
								msys::ShaderExtractImageChannel::Channel::One, /* Ao */
								msys::ShaderExtractImageChannel::Channel::Blue, /* Roughness */
								msys::ShaderExtractImageChannel::Channel::One, /* Metalness */
								msys::ShaderExtractImageChannel::Channel::One /* Alpha */
							},msys::ShaderExtractImageChannel::Pipeline::RGBA8
						);
						texInfo.outputFormat = uimg::TextureInfo::OutputFormat::ColorMap;
						prosper::util::save_texture(rootPath +'/' +rmaPath,*imgRma,texInfo,errHandler);

						rootData.AddData("rma_map",std::make_shared<ds::Texture>(settings,rmaPath));

						auto rmaInfo = rootData.AddBlock("rma_info");
						rmaInfo->AddValue("bool","requires_ao_update","1");
					}
				}
			}
#endif
		}
	}
	matManager.GetTextureManager().ClearUnused();
	return true;
}
#endif
