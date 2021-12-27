/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2021 Silverlan
 */

#include "impl_texture_formats.h"
#include "cmaterial_manager2.hpp"
#include "cmaterial.h"
#include "texturemanager/texture_manager2.hpp"
#include "c_source_vmt_format_handler.hpp"
#include "c_source2_vmat_format_handler.hpp"

#include <shader/prosper_shader_manager.hpp>
#include "shaders/c_shader_decompose_cornea.hpp"
#include "shaders/c_shader_ssbumpmap_to_normalmap.hpp"
#include "shaders/c_shader_extract_image_channel.hpp"
#include "shaders/source2/c_shader_generate_tangent_space_normal_map.hpp"
#include "shaders/source2/c_shader_decompose_metalness_reflectance.hpp"
#include "shaders/source2/c_shader_decompose_pbr.hpp"

std::shared_ptr<msys::CMaterialManager> msys::CMaterialManager::Create(prosper::IPrContext &context)
{
	auto matManager = std::shared_ptr<CMaterialManager>{new CMaterialManager{context}};
	matManager->Initialize();
	return matManager;
}
msys::CMaterialManager::CMaterialManager(prosper::IPrContext &context)
	: m_context{context}
{
	m_textureManager = std::make_unique<msys::TextureManager>(context);
	m_textureManager->SetRootDirectory("materials");

	// TODO: Move this into an importer interface
	context.GetShaderManager().RegisterShader("decompose_cornea",[](prosper::IPrContext &context,const std::string &identifier) {return new msys::ShaderDecomposeCornea(context,identifier);});
	context.GetShaderManager().RegisterShader("ssbumpmap_to_normalmap",[](prosper::IPrContext &context,const std::string &identifier) {return new msys::ShaderSSBumpMapToNormalMap(context,identifier);});
	context.GetShaderManager().RegisterShader("source2_generate_tangent_space_normal_map",[](prosper::IPrContext &context,const std::string &identifier) {return new msys::source2::ShaderGenerateTangentSpaceNormalMap(context,identifier);});
	context.GetShaderManager().RegisterShader("source2_generate_tangent_space_normal_map_proto",[](prosper::IPrContext &context,const std::string &identifier) {return new msys::source2::ShaderGenerateTangentSpaceNormalMapProto(context,identifier);});
	context.GetShaderManager().RegisterShader("source2_decompose_metalness_reflectance",[](prosper::IPrContext &context,const std::string &identifier) {return new msys::source2::ShaderDecomposeMetalnessReflectance(context,identifier);});
	context.GetShaderManager().RegisterShader("source2_decompose_pbr",[](prosper::IPrContext &context,const std::string &identifier) {return new msys::source2::ShaderDecomposePBR(context,identifier);});
	context.GetShaderManager().RegisterShader("extract_image_channel",[](prosper::IPrContext &context,const std::string &identifier) {return new msys::ShaderExtractImageChannel(context,identifier);});
	context.GetShaderManager().GetShader("copy_image"); // Make sure copy_image shader has been initialized
}
msys::CMaterialManager::~CMaterialManager()
{
	MaterialManager::Reset();
	m_textureManager = nullptr;
}
std::shared_ptr<Material> msys::CMaterialManager::CreateMaterialObject(const std::string &shader,const std::shared_ptr<ds::Block> &data)
{
	return std::shared_ptr<CMaterial>{CMaterial::Create(*this,shader,data)};
}
void msys::CMaterialManager::InitializeImportHandlers()
{
#ifndef DISABLE_VMT_SUPPORT
	RegisterImportHandler<CSourceVmtFormatHandler>("vmt");
#endif
#ifndef DISABLE_VMAT_SUPPORT
	RegisterImportHandler<CSource2VmatFormatHandler>("vmat_c");
#endif
}
void msys::CMaterialManager::SetShaderHandler(const std::function<void(Material*)> &handler) {m_shaderHandler = handler;}
void msys::CMaterialManager::ReloadMaterialShaders()
{
	if(m_shaderHandler == nullptr)
		return;
	GetContext().WaitIdle();
	for(auto &assetInfo : GetAssets())
	{
		if(!assetInfo.asset)
			continue;
		auto hMat = GetAssetObject(*assetInfo.asset);
		if(hMat && hMat->IsLoaded() == true)
			m_shaderHandler(hMat.get());
	}
}
void msys::CMaterialManager::MarkForReload(CMaterial &mat) {m_reloadShaderQueue.push(mat.GetHandle());}
void msys::CMaterialManager::Poll()
{
	MaterialManager::Poll();
	m_textureManager->Poll();
	if(!m_reloadShaderQueue.empty())
	{
		std::unordered_set<Material*> traversed;
		while(!m_reloadShaderQueue.empty())
		{
			auto hMat = m_reloadShaderQueue.front();
			m_reloadShaderQueue.pop();

			if(!hMat.expired())
				continue;
			auto *pmat = hMat.lock().get();
			auto it = traversed.find(pmat);
			if(it != traversed.end())
				continue;
			traversed.insert(pmat);
			m_shaderHandler(pmat);
		}
	}
}
