/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2021 Silverlan
 */

#include "matsysdefinitions.h"
#include "c_source_vmt_format_handler.hpp"
#include "cmaterial_manager2.hpp"
#include "shaders/c_shader_decompose_cornea.hpp"
#include "shaders/c_shader_ssbumpmap_to_normalmap.hpp"
#include "shaders/c_shader_extract_image_channel.hpp"
#include "shaders/source2/c_shader_generate_tangent_space_normal_map.hpp"
#include "shaders/source2/c_shader_decompose_metalness_reflectance.hpp"
#include "shaders/source2/c_shader_decompose_pbr.hpp"
#include "texturemanager/texture.h"
#include "sprite_sheet_animation.hpp"
#include "texturemanager/texture_manager2.hpp"
#include <prosper_util.hpp>
#include <prosper_descriptor_set_group.hpp>
#include <prosper_command_buffer.hpp>
#include <textureinfo.h>
#include <image/prosper_texture.hpp>
#include <sharedutils/util_string.h>
#include <sharedutils/util_path.hpp>
#include <sharedutils/datastream.h>
#include <util_texture_info.hpp>
#include <datasystem.h>
#include <datasystem_vector.h>
#include <fsys/ifile.hpp>

#pragma optimize("",off)
#ifndef DISABLE_VMT_SUPPORT
#include <VMTFile.h>
#include <VTFLib.h>
#include "util_vmt.hpp"
msys::CSourceVmtFormatHandler::CSourceVmtFormatHandler(util::IAssetManager &assetManager)
	: SourceVmtFormatHandler{assetManager}
{}
bool msys::CSourceVmtFormatHandler::LoadVMTData(VTFLib::CVMTFile &vmt,const std::string &vmtShader,ds::Block &rootData,std::string &matShader)
{
	auto r = SourceVmtFormatHandler::LoadVMTData(vmt,vmtShader,rootData,matShader);
	if(!r)
		return false;
	//TODO: These do not work if the textures haven't been imported yet!!
	VTFLib::Nodes::CVMTNode *node = nullptr;
	auto *vmtRoot = vmt.GetRoot();
	auto settings = ds::create_data_settings({});
	auto &matManager = static_cast<CMaterialManager&>(GetAssetManager());
	auto rootPath = matManager.GetImportDirectory();
	if(ustring::compare<std::string>(vmtShader,"eyes",false))
	{
		matShader = "eye_legacy";
		if((node = vmtRoot->GetNode("$iris")) != nullptr)
		{
			if(node->GetType() == VMTNodeType::NODE_TYPE_STRING)
			{
				auto *irisNode = static_cast<VTFLib::Nodes::CVMTStringNode*>(node);
				rootData.AddData("iris_map",std::make_shared<ds::Texture>(*settings,irisNode->GetValue()));
			}
		}
		if((node = vmtRoot->GetNode("$basetexture")) != nullptr)
		{
			if(node->GetType() == VMTNodeType::NODE_TYPE_STRING)
			{
				auto *irisNode = static_cast<VTFLib::Nodes::CVMTStringNode*>(node);
				rootData.AddData("sclera_map",std::make_shared<ds::Texture>(*settings,irisNode->GetValue()));
			}
		}
		rootData.AddValue("float","iris_scale","0.5");
	}
	else if(ustring::compare<std::string>(vmtShader,"eyerefract",false))
	{
		matShader = "eye";
		std::string irisTexture = "";
		if((node = vmtRoot->GetNode("$iris")) != nullptr)
		{
			if(node->GetType() == VMTNodeType::NODE_TYPE_STRING)
			{
				auto *irisNode = static_cast<VTFLib::Nodes::CVMTStringNode*>(node);
				irisTexture = irisNode->GetValue();
			}
		}

		std::string corneaTexture = "";
		if((node = vmtRoot->GetNode("$corneatexture")) != nullptr)
		{
			if(node->GetType() == VMTNodeType::NODE_TYPE_STRING)
			{
				auto *corneaNode = static_cast<VTFLib::Nodes::CVMTStringNode*>(node);
				corneaTexture = corneaNode->GetValue();
			}
		}

		// Some conversions are required for the iris and cornea textures for usage in Pragma
		auto &context = matManager.GetContext();
		auto *shaderDecomposeCornea = static_cast<msys::ShaderDecomposeCornea*>(context.GetShader("decompose_cornea").get());
		if(shaderDecomposeCornea)
		{
			auto &textureManager = matManager.GetTextureManager();

			auto irisMap = textureManager.LoadAsset(irisTexture);
			if(irisMap == nullptr)
				irisMap = textureManager.GetErrorTexture();

			auto corneaMap = textureManager.LoadAsset(corneaTexture);
			if(corneaMap == nullptr)
				corneaMap = textureManager.GetErrorTexture();

			if(irisMap && irisMap->HasValidVkTexture() && corneaMap && corneaMap->HasValidVkTexture())
			{
				// Prepare output textures (albedo, normal, parallax)
				prosper::util::ImageCreateInfo imgCreateInfo {};
				//imgCreateInfo.flags |= prosper::util::ImageCreateInfo::Flags::FullMipmapChain;
				imgCreateInfo.format = prosper::Format::R8G8B8A8_UNorm;
				imgCreateInfo.memoryFeatures = prosper::MemoryFeatureFlags::GPUBulk;
				imgCreateInfo.postCreateLayout = prosper::ImageLayout::ColorAttachmentOptimal;
				imgCreateInfo.tiling = prosper::ImageTiling::Optimal;
				imgCreateInfo.usage = prosper::ImageUsageFlags::ColorAttachmentBit | prosper::ImageUsageFlags::TransferSrcBit;

				imgCreateInfo.width = umath::max(irisMap->GetWidth(),corneaMap->GetWidth());
				imgCreateInfo.height = umath::max(irisMap->GetHeight(),corneaMap->GetHeight());
				auto imgAlbedo = context.CreateImage(imgCreateInfo);

				imgCreateInfo.format = prosper::Format::R32G32B32A32_SFloat;
				auto imgNormal = context.CreateImage(imgCreateInfo);
				auto imgParallax = context.CreateImage(imgCreateInfo);
				auto imgNoise = context.CreateImage(imgCreateInfo);

				prosper::util::ImageViewCreateInfo imgViewCreateInfo {};
				auto texAlbedo = context.CreateTexture({},*imgAlbedo,imgViewCreateInfo);
				auto texNormal = context.CreateTexture({},*imgNormal,imgViewCreateInfo);
				auto texParallax = context.CreateTexture({},*imgParallax,imgViewCreateInfo);
				auto texNoise = context.CreateTexture({},*imgNoise,imgViewCreateInfo);
				auto rt = context.CreateRenderTarget({texAlbedo,texNormal,texParallax,texNoise},shaderDecomposeCornea->GetRenderPass());

				auto dsg = shaderDecomposeCornea->CreateDescriptorSetGroup(msys::ShaderDecomposeCornea::DESCRIPTOR_SET_TEXTURE.setIndex);
				auto &ds = *dsg->GetDescriptorSet();
				auto &vkIrisTex = irisMap->GetVkTexture();
				auto &vkCorneaTex = corneaMap->GetVkTexture();
				ds.SetBindingTexture(*vkIrisTex,umath::to_integral(msys::ShaderDecomposeCornea::TextureBinding::IrisMap));
				ds.SetBindingTexture(*vkCorneaTex,umath::to_integral(msys::ShaderDecomposeCornea::TextureBinding::CorneaMap));
				auto &setupCmd = context.GetSetupCommandBuffer();
				if(setupCmd->RecordBeginRenderPass(*rt))
				{
					if(shaderDecomposeCornea->BeginDraw(setupCmd))
					{
						shaderDecomposeCornea->Draw(ds);
						shaderDecomposeCornea->EndDraw();
					}
					setupCmd->RecordEndRenderPass();
				}
				context.FlushSetupCommandBuffer();

				auto irisTextureNoExt = irisTexture;
				ufile::remove_extension_from_filename(irisTextureNoExt);
				auto corneaTextureNoExt = corneaTexture;
				ufile::remove_extension_from_filename(corneaTextureNoExt);

				auto albedoTexName = irisTextureNoExt +"_albedo";
				auto normalTexName = corneaTextureNoExt +"_normal";
				auto parallaxTexName = corneaTextureNoExt +"_parallax";
				auto noiseTexName = corneaTextureNoExt +"_noise";

				auto errHandler = [](const std::string &err) {
					std::cout<<"WARNING: Unable to save eyeball image(s) as DDS: "<<err<<std::endl;
				};

				// TODO: Change width/height
				uimg::TextureInfo texInfo {};
				texInfo.containerFormat = uimg::TextureInfo::ContainerFormat::DDS;
				texInfo.alphaMode = uimg::TextureInfo::AlphaMode::Auto;
				texInfo.flags = uimg::TextureInfo::Flags::GenerateMipmaps;
				texInfo.inputFormat = uimg::TextureInfo::InputFormat::R8G8B8A8_UInt;
				prosper::util::save_texture((rootPath +('/' +albedoTexName)).GetString(),texAlbedo->GetImage(),texInfo,errHandler);

				texInfo.outputFormat = uimg::TextureInfo::OutputFormat::GradientMap;
				texInfo.inputFormat = uimg::TextureInfo::InputFormat::R32G32B32A32_Float;
				prosper::util::save_texture((rootPath +('/' +parallaxTexName)).GetString(),texParallax->GetImage(),texInfo,errHandler);
				prosper::util::save_texture((rootPath +('/' +noiseTexName)).GetString(),texNoise->GetImage(),texInfo,errHandler);

				texInfo.outputFormat = uimg::TextureInfo::OutputFormat::NormalMap;
				texInfo.SetNormalMap();
				prosper::util::save_texture((rootPath +('/' +normalTexName)).GetString(),texNormal->GetImage(),texInfo,errHandler);

				// TODO: These should be Material::ALBEDO_MAP_IDENTIFIER/Material::NORMAL_MAP_IDENTIFIER/Material::PARALLAX_MAP_IDENTIFIER, but
				// for some reason the linker complains about unresolved symbols?
				rootData.AddData("albedo_map",std::make_shared<ds::Texture>(*settings,albedoTexName));
				rootData.AddData("normal_map",std::make_shared<ds::Texture>(*settings,normalTexName));
				rootData.AddData("parallax_map",std::make_shared<ds::Texture>(*settings,parallaxTexName));
				rootData.AddData("noise_map",std::make_shared<ds::Texture>(*settings,noiseTexName));
				rootData.AddValue("float","metalness_factor","0.0");
				rootData.AddValue("float","roughness_factor","0.0");

				// Default subsurface scattering values
				rootData.AddValue("float","subsurface_multiplier","0.01");
				rootData.AddValue("color","subsurface_color","242 210 157");
				rootData.AddValue("int","subsurface_method","5");
				rootData.AddValue("vector","subsurface_radius","112 52.8 1.6");
			}

			auto ptrRoot = std::static_pointer_cast<ds::Block>(rootData.shared_from_this());
			if((node = vmtRoot->GetNode("$eyeballradius")) != nullptr)
				get_vmt_data<ds::Bool,int32_t>(ptrRoot,*settings,"eyeball_radius",node);
			if((node = vmtRoot->GetNode("$dilation")) != nullptr)
				get_vmt_data<ds::Bool,int32_t>(ptrRoot,*settings,"pupil_dilation",node);
		}
	}
	else if(ustring::compare<std::string>(vmtShader,"spritecard",false))
	{
		// Some Source Engine textures contain embedded animation sheet data.
		// Since our texture formats don't support that, we'll have to extract it and
		// store it separately.
		matShader = "particle";
		if((node = vmtRoot->GetNode("$basetexture")) != nullptr)
		{
			if(node->GetType() == VMTNodeType::NODE_TYPE_STRING)
			{
				auto *baseTexture = static_cast<VTFLib::Nodes::CVMTStringNode*>(node);
				auto &textureManager = matManager.GetTextureManager();

				auto &context = matManager.GetContext();
				auto baseTexMap = textureManager.LoadAsset(baseTexture->GetValue());
				if(baseTexMap != nullptr)
				{
					auto &baseTexName = baseTexMap->GetName();
					auto texPath = matManager.GetRootDirectory();
					texPath += util::Path::CreateFile(baseTexName +".vtf");
					auto fptr = FileManager::OpenFile(texPath.GetString().c_str(),"rb");
					if(fptr)
					{
						fsys::File f {fptr};
						VTFLib::CVTFFile fVtf{};
						if(fVtf.Load(&f,false))
						{
							vlUInt resSize;
							auto *ptr = fVtf.GetResourceData(tagVTFResourceEntryType::VTF_RSRC_SHEET,resSize);
							if(ptr)
							{
								DataStream ds {ptr,resSize};
								ds->SetOffset(0);
								auto version = ds->Read<int32_t>();
								assert(version == 0 || version == 1);
								auto numSequences = ds->Read<int32_t>();

								SpriteSheetAnimation anim {};
								auto &sequences = anim.sequences;
								sequences.reserve(numSequences);
								for(auto i=decltype(numSequences){0};i<numSequences;++i)
								{
									auto seqIdx = ds->Read<int32_t>();
									if(seqIdx >= sequences.size())
										sequences.resize(seqIdx +1);
									auto &seq = sequences.at(seqIdx);
									seq.loop = !static_cast<bool>(ds->Read<int32_t>());
									auto numFrames = ds->Read<int32_t>();
									seq.frames.resize(numFrames);
									auto sequenceLength = ds->Read<float>();
									for(auto j=decltype(numFrames){0};j<numFrames;++j)
									{
										auto &frame = seq.frames.at(j);
										frame.duration = ds->Read<float>();

										auto constexpr MAX_IMAGES_PER_FRAME = 4u;
										frame.uvStart = ds->Read<Vector2>();
										frame.uvEnd = ds->Read<Vector2>();

										// Animation data can contain multiple images per frame.
										// I'm not sure what the purpose of that is (multi-texture?), but we'll ignore it for
										// the time being.
										if(version > 0)
											ds->SetOffset(ds->GetOffset() +sizeof(Vector2) *2 *(MAX_IMAGES_PER_FRAME -1));
#if 0
										for(auto t=decltype(MAX_IMAGES_PER_FRAME){0u};t<MAX_IMAGES_PER_FRAME;++t)
										{
											auto &img = frame.images.at(t);
											img.xMin = ds->Read<float>();
											img.yMin = ds->Read<float>();
											img.xMax = ds->Read<float>();
											img.yMax = ds->Read<float>();

											if(version == 0)
											{
												for(uint8_t i=1;i<MAX_IMAGES_PER_FRAME;++i)
													frame.images.at(i) = img;
												break;
											}
										}
#endif
									}
								}
								auto sequenceFilePath = rootPath +util::Path::CreateFile(baseTexName +".psd");
								FileManager::CreatePath(sequenceFilePath.GetPath().data());
								auto fSeq = FileManager::OpenFile<VFilePtrReal>(sequenceFilePath.GetString().c_str(),"wb");
								if(fSeq)
								{
									anim.Save(fSeq);
									rootData.AddValue("string","animation",util::Path{baseTexName}.GetString());
								}
							}
						}
					}
				}
			}
		}

		auto *nodeOverbrightFactor = vmtRoot->GetNode("$overbrightfactor");
		if(nodeOverbrightFactor == nullptr)
			nodeOverbrightFactor = vmtRoot->GetNode("srgb?$overbrightfactor");
		if(nodeOverbrightFactor)
		{
			float overbrightFactor;
			if(vmt_parameter_to_numeric_type(nodeOverbrightFactor,overbrightFactor) && overbrightFactor != 0.f)
			{
				// Overbright factors can get fairly large (e.g. 31 -> "particle/blood1/blood_goop3_spray"), we'll scale it down for Pragma
				// so that a factor of 30 roughly equals 1.9
				overbrightFactor = umath::max(overbrightFactor,1.2f);
				overbrightFactor = logf(overbrightFactor) /logf(6.f); // log base 6
				Vector4 colorFactor{0.f,0.f,0.f,0.f};
				auto vColorFactor = rootData.GetValue("bloom_color_factor");
				if(vColorFactor && typeid(*vColorFactor) == typeid(ds::Vector4))
					colorFactor = static_cast<ds::Vector4&>(*vColorFactor).GetValue();
				colorFactor += Vector4{overbrightFactor,overbrightFactor,overbrightFactor,0.f};

				rootData.AddValue("vector4","bloom_color_factor",std::to_string(colorFactor.r) +' ' +std::to_string(colorFactor.g) +' ' +std::to_string(colorFactor.b) +" 1.0");
			}
		}

		auto *nodeAddSelf = vmtRoot->GetNode("$addself");
		float addSelf;
		if(nodeAddSelf && vmt_parameter_to_numeric_type(nodeAddSelf,addSelf))
		{
			Vector4 colorFactor{1.f,1.f,1.f,1.f};
			auto vColorFactor = rootData.GetValue("color_factor");
			if(vColorFactor && typeid(*vColorFactor) == typeid(ds::Vector4))
				colorFactor = static_cast<ds::Vector4&>(*vColorFactor).GetValue();
			colorFactor += Vector4{addSelf,addSelf,addSelf,0.f};

			rootData.AddValue("vector4","color_factor",std::to_string(colorFactor.r) +' ' +std::to_string(colorFactor.g) +' ' +std::to_string(colorFactor.b) +" 1.0");
		}
	}
	int32_t ssBumpmap;
	if((node = vmtRoot->GetNode("$ssbump")) != nullptr && vmt_parameter_to_numeric_type<int32_t>(node,ssBumpmap) && ssBumpmap != 0)
	{
		// Material is using a self-shadowing bump map, which Pragma doesn't support, so we'll convert it to a normal map.
		std::string bumpMapTexture = "";
		if((node = vmtRoot->GetNode("$bumpmap")) != nullptr)
		{
			if(node->GetType() == VMTNodeType::NODE_TYPE_STRING)
			{
				auto *bumpNapNode = static_cast<VTFLib::Nodes::CVMTStringNode*>(node);
				bumpMapTexture = bumpNapNode->GetValue();
			}
		}

		auto &context = matManager.GetContext();
		auto *shaderSSBumpMapToNormalMap = static_cast<msys::ShaderSSBumpMapToNormalMap*>(context.GetShader("ssbumpmap_to_normalmap").get());
		context.GetShaderManager().GetShader("copy_image"); // Make sure copy_image shader has been initialized
		if(shaderSSBumpMapToNormalMap)
		{
			auto &textureManager = matManager.GetTextureManager();

			auto bumpMap = textureManager.LoadAsset(bumpMapTexture);
			if(bumpMap && bumpMap->HasValidVkTexture())
			{
				// Prepare output texture (normal map)
				prosper::util::ImageCreateInfo imgCreateInfo {};
				//imgCreateInfo.flags |= prosper::util::ImageCreateInfo::Flags::FullMipmapChain;
				imgCreateInfo.format = prosper::Format::R32G32B32A32_SFloat;
				imgCreateInfo.memoryFeatures = prosper::MemoryFeatureFlags::GPUBulk;
				imgCreateInfo.postCreateLayout = prosper::ImageLayout::ColorAttachmentOptimal;
				imgCreateInfo.tiling = prosper::ImageTiling::Optimal;
				imgCreateInfo.usage = prosper::ImageUsageFlags::ColorAttachmentBit | prosper::ImageUsageFlags::TransferSrcBit;

				imgCreateInfo.width = bumpMap->GetWidth();
				imgCreateInfo.height = bumpMap->GetHeight();
				auto imgNormal = context.CreateImage(imgCreateInfo);

				prosper::util::ImageViewCreateInfo imgViewCreateInfo {};
				auto texNormal = context.CreateTexture({},*imgNormal,imgViewCreateInfo);
				auto rt = context.CreateRenderTarget({texNormal},shaderSSBumpMapToNormalMap->GetRenderPass());

				auto dsg = shaderSSBumpMapToNormalMap->CreateDescriptorSetGroup(msys::ShaderSSBumpMapToNormalMap::DESCRIPTOR_SET_TEXTURE.setIndex);
				auto &ds = *dsg->GetDescriptorSet();
				auto &vkBumpMapTex = bumpMap->GetVkTexture();
				ds.SetBindingTexture(*vkBumpMapTex,umath::to_integral(msys::ShaderSSBumpMapToNormalMap::TextureBinding::SSBumpMap));
				auto &setupCmd = context.GetSetupCommandBuffer();
				if(setupCmd->RecordBeginRenderPass(*rt))
				{
					if(shaderSSBumpMapToNormalMap->BeginDraw(setupCmd))
					{
						shaderSSBumpMapToNormalMap->Draw(ds);
						shaderSSBumpMapToNormalMap->EndDraw();
					}
					setupCmd->RecordEndRenderPass();
				}
				context.FlushSetupCommandBuffer();

				auto bumpMapTextureNoExt = bumpMapTexture;
				ufile::remove_extension_from_filename(bumpMapTexture);

				auto normalTexName = bumpMapTexture +"_normal";

				auto errHandler = [](const std::string &err) {
					std::cout<<"WARNING: Unable to save converted ss bumpmap as DDS: "<<err<<std::endl;
				};

				uimg::TextureInfo texInfo {};
				texInfo.containerFormat = uimg::TextureInfo::ContainerFormat::DDS;
				texInfo.alphaMode = uimg::TextureInfo::AlphaMode::None;
				texInfo.flags |= uimg::TextureInfo::Flags::GenerateMipmaps;
				texInfo.inputFormat = uimg::TextureInfo::InputFormat::R32G32B32A32_Float;
				texInfo.outputFormat = uimg::TextureInfo::OutputFormat::NormalMap;
				texInfo.SetNormalMap();
				prosper::util::save_texture((rootPath +('/' +normalTexName)).GetString(),texNormal->GetImage(),texInfo,errHandler);

				// TODO: This should be Material::NORMAL_MAP_IDENTIFIER, but
				// for some reason the linker complains about unresolved symbols?
				rootData.AddData("normal_map",std::make_shared<ds::Texture>(*settings,normalTexName));
			}
		}
	}
	matManager.GetTextureManager().ClearUnused();
	return true;
}
#endif
#pragma optimize("",on)
