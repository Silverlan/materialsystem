/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "cmaterialmanager.h"
#include "cmaterial.h"
#include <sharedutils/util_string.h>
#include <prosper_context.hpp>

CMaterialManager::CMaterialManager(prosper::Context &context)
	: MaterialManager{},prosper::ContextObject(context),m_textureManager(context)
{}

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
