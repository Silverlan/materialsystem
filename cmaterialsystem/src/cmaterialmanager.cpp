/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "cmaterialmanager.h"
#include "cmaterial.h"
#include "shaders/c_shader_decompose_cornea.hpp"
#include <sharedutils/util_string.h>
#include <sharedutils/util_file.h>
#include <prosper_context.hpp>
#include <prosper_util.hpp>
#include <prosper_command_buffer.hpp>
#include <prosper_descriptor_set_group.hpp>
#include <image/prosper_render_target.hpp>
#include <util_texture_info.hpp>
#include <util_image.hpp>
#ifdef ENABLE_VMT_SUPPORT
#include <VMTFile.h>
#include "util_vmt.hpp"
#endif

#pragma optimize("",off)
CMaterialManager::CMaterialManager(prosper::Context &context)
	: MaterialManager{},prosper::ContextObject(context),m_textureManager(context)
{
	context.GetShaderManager().RegisterShader("decompose_cornea",[](prosper::Context &context,const std::string &identifier) {return new msys::ShaderDecomposeCornea(context,identifier);});
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
	if(MaterialManager::InitializeVMTData(vmt,info,rootData,settings,shader) == false)
		return false;
	if(ustring::compare(shader,"eyerefract",false))
	{
		info.shader = "eye";
		auto *vmtRoot = vmt.GetRoot();
		VTFLib::Nodes::CVMTNode *node = nullptr;
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
				imgCreateInfo.format = Anvil::Format::R8G8B8A8_UNORM;
				imgCreateInfo.memoryFeatures = prosper::util::MemoryFeatureFlags::GPUBulk;
				imgCreateInfo.postCreateLayout = Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
				imgCreateInfo.tiling = Anvil::ImageTiling::OPTIMAL;
				imgCreateInfo.usage = Anvil::ImageUsageFlagBits::COLOR_ATTACHMENT_BIT | Anvil::ImageUsageFlagBits::TRANSFER_SRC_BIT;

				imgCreateInfo.width = umath::max(pIrisMap->GetWidth(),pCorneaMap->GetWidth());
				imgCreateInfo.height = umath::max(pIrisMap->GetHeight(),pCorneaMap->GetHeight());
				auto &dev = context.GetDevice();
				auto imgAlbedo = prosper::util::create_image(dev,imgCreateInfo);

				imgCreateInfo.format = Anvil::Format::R32G32B32A32_SFLOAT;
				auto imgNormal = prosper::util::create_image(context.GetDevice(),imgCreateInfo);
				auto imgParallax = prosper::util::create_image(context.GetDevice(),imgCreateInfo);
				auto imgNoise = prosper::util::create_image(context.GetDevice(),imgCreateInfo);

				prosper::util::ImageViewCreateInfo imgViewCreateInfo {};
				auto texAlbedo = prosper::util::create_texture(dev,{},imgAlbedo,&imgViewCreateInfo);
				auto texNormal = prosper::util::create_texture(dev,{},imgNormal,&imgViewCreateInfo);
				auto texParallax = prosper::util::create_texture(dev,{},imgParallax,&imgViewCreateInfo);
				auto texNoise = prosper::util::create_texture(dev,{},imgNoise,&imgViewCreateInfo);
				auto rt = prosper::util::create_render_target(dev,{texAlbedo,texNormal,texParallax,texNoise},shaderDecomposeCornea->GetRenderPass());

				auto dsg = shaderDecomposeCornea->CreateDescriptorSetGroup(msys::ShaderDecomposeCornea::DESCRIPTOR_SET_TEXTURE.setIndex);
				auto &ds = *dsg->GetDescriptorSet();
				auto &vkIrisTex = pIrisMap->GetVkTexture();
				auto &vkCorneaTex = pCorneaMap->GetVkTexture();
				prosper::util::set_descriptor_set_binding_texture(ds,*vkIrisTex,umath::to_integral(msys::ShaderDecomposeCornea::TextureBinding::IrisMap));
				prosper::util::set_descriptor_set_binding_texture(ds,*vkCorneaTex,umath::to_integral(msys::ShaderDecomposeCornea::TextureBinding::CorneaMap));
				auto &setupCmd = context.GetSetupCommandBuffer();
				if(prosper::util::record_begin_render_pass(**setupCmd,*rt))
				{
					if(shaderDecomposeCornea->BeginDraw(setupCmd))
					{
						shaderDecomposeCornea->Draw(*ds);
						shaderDecomposeCornea->EndDraw();
					}
					prosper::util::record_end_render_pass(**setupCmd);
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
				prosper::util::save_texture(rootPath +'/' +albedoTexName,*texAlbedo->GetImage(),texInfo,errHandler);

				texInfo.outputFormat = uimg::TextureInfo::OutputFormat::GradientMap;
				texInfo.inputFormat = uimg::TextureInfo::InputFormat::R32G32B32A32_Float;
				prosper::util::save_texture(rootPath +'/' +parallaxTexName,*texParallax->GetImage(),texInfo,errHandler);
				prosper::util::save_texture(rootPath +'/' +noiseTexName,*texNoise->GetImage(),texInfo,errHandler);

				texInfo.outputFormat = uimg::TextureInfo::OutputFormat::NormalMap;
				texInfo.flags |= uimg::TextureInfo::Flags::NormalMap;
				prosper::util::save_texture(rootPath +'/' +normalTexName,*texNormal->GetImage(),texInfo,errHandler);

				// TODO: These should be Material::ALBEDO_MAP_IDENTIFIER/Material::NORMAL_MAP_IDENTIFIER/Material::PARALLAX_MAP_IDENTIFIER, but
				// for some reason the linker complains about unresolved symbols?
				rootData.AddData("albedo_map",std::make_shared<ds::Texture>(settings,albedoTexName));
				rootData.AddData("normal_map",std::make_shared<ds::Texture>(settings,normalTexName));
				rootData.AddData("parallax_map",std::make_shared<ds::Texture>(settings,parallaxTexName));
				rootData.AddData("noise_map",std::make_shared<ds::Texture>(settings,noiseTexName));

				// Default subsurface scattering values
				rootData.AddValue("float","subsurface_multiplier","0.0375");
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
		auto shaderHandler = m_shaderHandler;
		auto texLoadFlags = TextureLoadFlags::None;
		umath::set_flag(texLoadFlags,TextureLoadFlags::LoadInstantly,bLoadInstantly);
		umath::set_flag(texLoadFlags,TextureLoadFlags::Reload,bReload);
		static_cast<CMaterial*>(info.material)->InitializeTextures(info.material->GetDataBlock(),[shaderHandler,onMaterialLoaded,mat]() {
			mat->SetLoaded(true);
			if(onMaterialLoaded != nullptr)
				onMaterialLoaded(mat);
			if(shaderHandler != nullptr)
				shaderHandler(mat);
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

	return info.material;
}

Material *CMaterialManager::Load(const std::string &path,bool bReload,bool *bFirstTimeError)
{
	return Load(path,nullptr,nullptr,bReload,bFirstTimeError);
}
#pragma optimize("",on)
