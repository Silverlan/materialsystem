/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <prosper_context.hpp>
#include "cmaterialmanager.h"
#include "cmaterial.h"
#include "shaders/c_shader_decompose_cornea.hpp"
#include "shaders/c_shader_ssbumpmap_to_normalmap.hpp"
#include "shaders/c_shader_extract_image_channel.hpp"
#include "shaders/source2/c_shader_generate_tangent_space_normal_map.hpp"
#include "shaders/source2/c_shader_decompose_metalness_reflectance.hpp"
#include "shaders/source2/c_shader_decompose_pbr.hpp"
#include <sharedutils/util_string.h>
#include <sharedutils/util_file.h>
#include <prosper_util.hpp>
#include <prosper_command_buffer.hpp>
#include <prosper_descriptor_set_group.hpp>
#include <image/prosper_render_target.hpp>
#include <util_texture_info.hpp>
#include <util_image.hpp>
#ifndef DISABLE_VMT_SUPPORT
#include <VMTFile.h>
#include "util_vmt.hpp"
#endif

#pragma optimize("",off)
CMaterialManager::CMaterialManager(prosper::IPrContext &context)
	: MaterialManager{},prosper::ContextObject(context),m_textureManager(context)
{
	context.GetShaderManager().RegisterShader("decompose_cornea",[](prosper::IPrContext &context,const std::string &identifier) {return new msys::ShaderDecomposeCornea(context,identifier);});
	context.GetShaderManager().RegisterShader("ssbumpmap_to_normalmap",[](prosper::IPrContext &context,const std::string &identifier) {return new msys::ShaderSSBumpMapToNormalMap(context,identifier);});
	context.GetShaderManager().RegisterShader("source2_generate_tangent_space_normal_map",[](prosper::IPrContext &context,const std::string &identifier) {return new msys::source2::ShaderGenerateTangentSpaceNormalMap(context,identifier);});
	context.GetShaderManager().RegisterShader("source2_generate_tangent_space_normal_map_proto",[](prosper::IPrContext &context,const std::string &identifier) {return new msys::source2::ShaderGenerateTangentSpaceNormalMapProto(context,identifier);});
	context.GetShaderManager().RegisterShader("source2_decompose_metalness_reflectance",[](prosper::IPrContext &context,const std::string &identifier) {return new msys::source2::ShaderDecomposeMetalnessReflectance(context,identifier);});
	context.GetShaderManager().RegisterShader("source2_decompose_pbr",[](prosper::IPrContext &context,const std::string &identifier) {return new msys::source2::ShaderDecomposePBR(context,identifier);});
	context.GetShaderManager().RegisterShader("extract_image_channel",[](prosper::IPrContext &context,const std::string &identifier) {return new msys::ShaderExtractImageChannel(context,identifier);});
}

CMaterialManager::~CMaterialManager()
{}

TextureManager &CMaterialManager::GetTextureManager() {return m_textureManager;}

void CMaterialManager::SetShaderHandler(const std::function<void(Material*)> &handler) {m_shaderHandler = handler;}
std::function<void(Material*)> CMaterialManager::GetShaderHandler() const {return m_shaderHandler;}

void CMaterialManager::Update() {m_textureManager.Update();}

Material *CMaterialManager::CreateMaterial(const std::string *identifier,const std::string &shader,std::shared_ptr<ds::Block> root)
{
	auto &context = GetContext();
	auto &shaderManager = context.GetShaderManager();
	std::string matId;
	if(identifier != nullptr)
	{
		matId = *identifier;
		ustring::to_lower(matId);
		auto *mat = FindMaterial(matId);
		if(mat != nullptr)
		{
			mat->SetShaderInfo(shaderManager.PreRegisterShader(shader));
			return mat;
		}
	}
	else
		matId = "__anonymous" +(m_unnamedIdx++);
	if(root == nullptr)
	{
		auto dataSettings = CreateDataSettings();
		root = std::make_shared<ds::Block>(*dataSettings);
	}
	auto *mat = CreateMaterial<CMaterial>(shaderManager.PreRegisterShader(shader),root); // Claims ownership of 'root' and frees the memory at destruction
	mat->SetLoaded(true);
	mat->SetName(matId);
	m_materials.insert(decltype(m_materials)::value_type{ToMaterialIdentifier(matId),mat->GetHandle()});
	return mat;
}
Material *CMaterialManager::CreateMaterial(const std::string &identifier,const std::string &shader,const std::shared_ptr<ds::Block> &root) {return CreateMaterial(&identifier,shader,root);}
Material *CMaterialManager::CreateMaterial(const std::string &shader,const std::shared_ptr<ds::Block> &root) {return CreateMaterial(nullptr,shader,root);}

void CMaterialManager::ReloadMaterialShaders()
{
	if(m_shaderHandler == nullptr)
		return;
	GetContext().WaitIdle();
	for(auto &it : m_materials)
	{
		if(it.second.IsValid() && it.second->IsLoaded() == true)
			m_shaderHandler(it.second.get());
	}
}

bool CMaterialManager::InitializeVMTData(VTFLib::CVMTFile &vmt,LoadInfo &info,ds::Block &rootData,ds::Settings &settings,const std::string &shader)
{
	//TODO: These do not work if the textures haven't been imported yet!!
	if(MaterialManager::InitializeVMTData(vmt,info,rootData,settings,shader) == false)
		return false;
	VTFLib::Nodes::CVMTNode *node = nullptr;
	auto *vmtRoot = vmt.GetRoot();
	if(ustring::compare(shader,"eyerefract",false))
	{
		info.shader = "eye";
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
		auto &context = GetContext();
		auto *shaderDecomposeCornea = static_cast<msys::ShaderDecomposeCornea*>(context.GetShader("decompose_cornea").get());
		if(shaderDecomposeCornea)
		{
			auto &textureManager = GetTextureManager();
			TextureManager::LoadInfo loadInfo {};
			loadInfo.flags = TextureLoadFlags::LoadInstantly;

			std::shared_ptr<void> irisMap = nullptr;
			if(textureManager.Load(context,irisTexture,loadInfo,&irisMap) == false)
				irisMap = textureManager.GetErrorTexture();

			std::shared_ptr<void> corneaMap = nullptr;
			if(textureManager.Load(context,corneaTexture,loadInfo,&corneaMap) == false)
				corneaMap = textureManager.GetErrorTexture();

			auto *pIrisMap = static_cast<Texture*>(irisMap.get());
			auto *pCorneaMap = static_cast<Texture*>(corneaMap.get());
			if(pIrisMap && pIrisMap->HasValidVkTexture() && pCorneaMap && pCorneaMap->HasValidVkTexture())
			{
				// Prepare output textures (albedo, normal, parallax)
				prosper::util::ImageCreateInfo imgCreateInfo {};
				//imgCreateInfo.flags |= prosper::util::ImageCreateInfo::Flags::FullMipmapChain;
				imgCreateInfo.format = prosper::Format::R8G8B8A8_UNorm;
				imgCreateInfo.memoryFeatures = prosper::MemoryFeatureFlags::GPUBulk;
				imgCreateInfo.postCreateLayout = prosper::ImageLayout::ColorAttachmentOptimal;
				imgCreateInfo.tiling = prosper::ImageTiling::Optimal;
				imgCreateInfo.usage = prosper::ImageUsageFlags::ColorAttachmentBit | prosper::ImageUsageFlags::TransferSrcBit;

				imgCreateInfo.width = umath::max(pIrisMap->GetWidth(),pCorneaMap->GetWidth());
				imgCreateInfo.height = umath::max(pIrisMap->GetHeight(),pCorneaMap->GetHeight());
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
				auto &vkIrisTex = pIrisMap->GetVkTexture();
				auto &vkCorneaTex = pCorneaMap->GetVkTexture();
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
				auto rootPath = "addons/converted/" +MaterialManager::GetRootMaterialLocation();
				uimg::TextureInfo texInfo {};
				texInfo.containerFormat = uimg::TextureInfo::ContainerFormat::DDS;
				texInfo.alphaMode = uimg::TextureInfo::AlphaMode::Auto;
				texInfo.flags = uimg::TextureInfo::Flags::GenerateMipmaps;
				texInfo.inputFormat = uimg::TextureInfo::InputFormat::R8G8B8A8_UInt;
				prosper::util::save_texture(rootPath +'/' +albedoTexName,texAlbedo->GetImage(),texInfo,errHandler);

				texInfo.outputFormat = uimg::TextureInfo::OutputFormat::GradientMap;
				texInfo.inputFormat = uimg::TextureInfo::InputFormat::R32G32B32A32_Float;
				prosper::util::save_texture(rootPath +'/' +parallaxTexName,texParallax->GetImage(),texInfo,errHandler);
				prosper::util::save_texture(rootPath +'/' +noiseTexName,texNoise->GetImage(),texInfo,errHandler);

				texInfo.outputFormat = uimg::TextureInfo::OutputFormat::NormalMap;
				texInfo.SetNormalMap();
				prosper::util::save_texture(rootPath +'/' +normalTexName,texNormal->GetImage(),texInfo,errHandler);

				// TODO: These should be Material::ALBEDO_MAP_IDENTIFIER/Material::NORMAL_MAP_IDENTIFIER/Material::PARALLAX_MAP_IDENTIFIER, but
				// for some reason the linker complains about unresolved symbols?
				rootData.AddData("albedo_map",std::make_shared<ds::Texture>(settings,albedoTexName));
				rootData.AddData("normal_map",std::make_shared<ds::Texture>(settings,normalTexName));
				rootData.AddData("parallax_map",std::make_shared<ds::Texture>(settings,parallaxTexName));
				rootData.AddData("noise_map",std::make_shared<ds::Texture>(settings,noiseTexName));
				rootData.AddValue("float","metalness_factor","0.0");
				rootData.AddValue("float","roughness_factor","0.0");

				// Default subsurface scattering values
				rootData.AddValue("float","subsurface_multiplier","0.01");
				rootData.AddValue("color","subsurface_color","242 210 157");
				rootData.AddValue("int","subsurface_method","5");
				rootData.AddValue("vector","subsurface_radius","112 52.8 1.6");
			}
		}

		auto ptrRoot = std::static_pointer_cast<ds::Block>(rootData.shared_from_this());
		if((node = vmtRoot->GetNode("$eyeballradius")) != nullptr)
			get_vmt_data<ds::Bool,int32_t>(ptrRoot,settings,"eyeball_radius",node);
		if((node = vmtRoot->GetNode("$dilation")) != nullptr)
			get_vmt_data<ds::Bool,int32_t>(ptrRoot,settings,"pupil_dilation",node);
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

		auto &context = GetContext();
		auto *shaderSSBumpMapToNormalMap = static_cast<msys::ShaderSSBumpMapToNormalMap*>(context.GetShader("ssbumpmap_to_normalmap").get());
		if(shaderSSBumpMapToNormalMap)
		{
			auto &textureManager = GetTextureManager();
			TextureManager::LoadInfo loadInfo {};
			loadInfo.flags = TextureLoadFlags::LoadInstantly;

			std::shared_ptr<void> bumpMap = nullptr;
			textureManager.Load(context,bumpMapTexture,loadInfo,&bumpMap);

			auto *pBumpMap = static_cast<Texture*>(bumpMap.get());
			if(pBumpMap && pBumpMap->HasValidVkTexture())
			{
				// Prepare output texture (normal map)
				prosper::util::ImageCreateInfo imgCreateInfo {};
				//imgCreateInfo.flags |= prosper::util::ImageCreateInfo::Flags::FullMipmapChain;
				imgCreateInfo.format = prosper::Format::R32G32B32A32_SFloat;
				imgCreateInfo.memoryFeatures = prosper::MemoryFeatureFlags::GPUBulk;
				imgCreateInfo.postCreateLayout = prosper::ImageLayout::ColorAttachmentOptimal;
				imgCreateInfo.tiling = prosper::ImageTiling::Optimal;
				imgCreateInfo.usage = prosper::ImageUsageFlags::ColorAttachmentBit | prosper::ImageUsageFlags::TransferSrcBit;

				imgCreateInfo.width = pBumpMap->GetWidth();
				imgCreateInfo.height = pBumpMap->GetHeight();
				auto imgNormal = context.CreateImage(imgCreateInfo);

				prosper::util::ImageViewCreateInfo imgViewCreateInfo {};
				auto texNormal = context.CreateTexture({},*imgNormal,imgViewCreateInfo);
				auto rt = context.CreateRenderTarget({texNormal},shaderSSBumpMapToNormalMap->GetRenderPass());

				auto dsg = shaderSSBumpMapToNormalMap->CreateDescriptorSetGroup(msys::ShaderSSBumpMapToNormalMap::DESCRIPTOR_SET_TEXTURE.setIndex);
				auto &ds = *dsg->GetDescriptorSet();
				auto &vkBumpMapTex = pBumpMap->GetVkTexture();
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

				auto rootPath = "addons/converted/" +MaterialManager::GetRootMaterialLocation();
				uimg::TextureInfo texInfo {};
				texInfo.containerFormat = uimg::TextureInfo::ContainerFormat::DDS;
				texInfo.alphaMode = uimg::TextureInfo::AlphaMode::None;
				texInfo.flags |= uimg::TextureInfo::Flags::GenerateMipmaps;
				texInfo.inputFormat = uimg::TextureInfo::InputFormat::R32G32B32A32_Float;
				texInfo.outputFormat = uimg::TextureInfo::OutputFormat::NormalMap;
				texInfo.SetNormalMap();
				prosper::util::save_texture(rootPath +'/' +normalTexName,texNormal->GetImage(),texInfo,errHandler);

				// TODO: This should be Material::NORMAL_MAP_IDENTIFIER, but
				// for some reason the linker complains about unresolved symbols?
				rootData.AddData("normal_map",std::make_shared<ds::Texture>(settings,normalTexName));
			}
		}
	}
	info.saveOnDisk = true;
	GetTextureManager().ClearUnused();
	return true;
}

void CMaterialManager::SetErrorMaterial(Material *mat)
{
	MaterialManager::SetErrorMaterial(mat);
	if(mat != nullptr)
	{
		auto *diffuse = mat->GetDiffuseMap();
		if(diffuse != nullptr)
		{
			m_textureManager.SetErrorTexture(std::static_pointer_cast<Texture>(diffuse->texture));
			return;
		}
	}
	m_textureManager.SetErrorTexture(nullptr);
}

Material *CMaterialManager::Load(const std::string &path,const std::function<void(Material*)> &onMaterialLoaded,const std::function<void(std::shared_ptr<Texture>)> &onTextureLoaded,bool bReload,bool *bFirstTimeError,bool bLoadInstantly)
{
	auto &context = GetContext();
	auto &shaderManager = context.GetShaderManager();
	if(bFirstTimeError != nullptr)
		*bFirstTimeError = false;
	auto bInitializeTextures = false;
	auto bCopied = false;
	MaterialManager::LoadInfo info{};
	if(!MaterialManager::Load(path,info,bReload))
	{
		if(info.material != nullptr) // bReload was true, the material was already loaded and reloading failed
			return info.material;
		info.material = GetErrorMaterial();
		if(info.material == nullptr)
			return nullptr;
		if(bFirstTimeError != nullptr)
			*bFirstTimeError = true;
		info.material = info.material->Copy(); // Copy error material
		bCopied = true;
		//bInitializeTextures = true; // TODO: Do we need to do this? (Error material should already be initialized); Callbacks will still have to be called!
	}
	else if(info.material != nullptr) // Material already exists, reload textured?
	{
		info.material->SetErrorFlag(false);
		if(bReload == false || !info.material->IsLoaded()) // Can't reload if material hasn't even been fully loaded yet
			return info.material;
		info.material->Initialize(shaderManager.PreRegisterShader(info.shader),info.root);
		bInitializeTextures = true;
	}
	else // Failed to load material
	{
		info.material = CreateMaterial<CMaterial>(shaderManager.PreRegisterShader(info.shader),info.root);
		bInitializeTextures = true;
	}
	info.material->SetName(info.identifier);
	// The material has to be registered before calling the callbacks below
	m_materials.insert(decltype(m_materials)::value_type{ToMaterialIdentifier(info.identifier),info.material->GetHandle()});
	if(bInitializeTextures == true)
	{
		auto *mat = info.material;
		auto texLoadFlags = TextureLoadFlags::None;
		umath::set_flag(texLoadFlags,TextureLoadFlags::LoadInstantly,bLoadInstantly);
		umath::set_flag(texLoadFlags,TextureLoadFlags::Reload,bReload);
		static_cast<CMaterial*>(info.material)->InitializeTextures(info.material->GetDataBlock(),[this,onMaterialLoaded,mat]() {
			mat->SetLoaded(true);
			if(onMaterialLoaded != nullptr)
				onMaterialLoaded(mat);
			if(m_shaderHandler != nullptr)
				m_shaderHandler(mat);
		},[onTextureLoaded](std::shared_ptr<Texture> texture) {
			if(onTextureLoaded != nullptr)
				onTextureLoaded(texture);
		},texLoadFlags);
	}
	else if(bCopied == true) // Material has been copied; Callbacks will have to be called anyway (To make sure descriptor set is initialized properly!)
	{
		if(onMaterialLoaded != nullptr)
			onMaterialLoaded(info.material);
		if(m_shaderHandler != nullptr)
			m_shaderHandler(info.material);
	}
	if(info.saveOnDisk)
		info.material->Save(info.material->GetName(),"addons/converted/");
	return info.material;
}

Material *CMaterialManager::Load(const std::string &path,bool bReload,bool *bFirstTimeError)
{
	return Load(path,nullptr,nullptr,bReload,bFirstTimeError);
}
#pragma optimize("",on)
