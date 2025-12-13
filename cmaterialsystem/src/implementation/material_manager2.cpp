// SPDX-FileCopyrightText: (c) 2021 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.cmaterialsystem;

import :format_handlers;
import :material_manager2;

void pragma::material::CMaterialManager::SetFlipTexturesVerticallyOnLoad(bool flip) { ITextureFormatHandler ::SetFlipTexturesVertically(flip); }
bool pragma::material::CMaterialManager::ShouldFlipTextureVerticallyOnLoad() { return ITextureFormatHandler::ShouldFlipTextureVertically(); }

std::shared_ptr<pragma::material::CMaterialManager> pragma::material::CMaterialManager::Create(prosper::IPrContext &context)
{
	auto matManager = std::shared_ptr<CMaterialManager> {new CMaterialManager {context}};
	matManager->Initialize();
	return matManager;
}
pragma::material::CMaterialManager::CMaterialManager(prosper::IPrContext &context) : m_context {context}
{
	m_textureManager = std::make_unique<TextureManager>(context);
	m_textureManager->SetRootDirectory("materials");

	// TODO: Move this into an importer interface
	context.GetShaderManager().RegisterShader("decompose_cornea", [](prosper::IPrContext &context, const std::string &identifier) { return new ShaderDecomposeCornea(context, identifier); });
	context.GetShaderManager().RegisterShader("ssbumpmap_to_normalmap", [](prosper::IPrContext &context, const std::string &identifier) { return new ShaderSSBumpMapToNormalMap(context, identifier); });
	context.GetShaderManager().RegisterShader("source2_generate_tangent_space_normal_map", [](prosper::IPrContext &context, const std::string &identifier) { return new source2::ShaderGenerateTangentSpaceNormalMap(context, identifier); });
	context.GetShaderManager().RegisterShader("source2_generate_tangent_space_normal_map_proto", [](prosper::IPrContext &context, const std::string &identifier) { return new source2::ShaderGenerateTangentSpaceNormalMapProto(context, identifier); });
	context.GetShaderManager().RegisterShader("source2_decompose_metalness_reflectance", [](prosper::IPrContext &context, const std::string &identifier) { return new source2::ShaderDecomposeMetalnessReflectance(context, identifier); });
	context.GetShaderManager().RegisterShader("source2_decompose_pbr", [](prosper::IPrContext &context, const std::string &identifier) { return new source2::ShaderDecomposePBR(context, identifier); });
	context.GetShaderManager().RegisterShader("extract_image_channel", [](prosper::IPrContext &context, const std::string &identifier) { return new ShaderExtractImageChannel(context, identifier); });
	context.GetShaderManager().GetShader("copy_image"); // Make sure copy_image shader has been initialized
}
pragma::material::CMaterialManager::~CMaterialManager()
{
	MaterialManager::Reset();
	m_textureManager = nullptr;
}
std::shared_ptr<pragma::material::Material> pragma::material::CMaterialManager::CreateMaterialObject(const std::string &shader, const std::shared_ptr<datasystem::Block> &data) { return std::shared_ptr<CMaterial> {CMaterial::Create(*this, shader, data)}; }
void pragma::material::CMaterialManager::InitializeImportHandlers()
{
#ifndef DISABLE_VMT_SUPPORT
#ifdef ENABLE_VKV_PARSER
	if(should_use_vkv_vmt_parser())
		RegisterImportHandler<CSourceVmtFormatHandler2>("vmt");
	else
#endif
		RegisterImportHandler<CSourceVmtFormatHandler>("vmt");
#endif
#ifndef DISABLE_VMAT_SUPPORT
	RegisterImportHandler<CSource2VmatFormatHandler>("vmat_c");
#endif
}
void pragma::material::CMaterialManager::SetShaderHandler(const std::function<void(Material *)> &handler) { m_shaderHandler = handler; }
void pragma::material::CMaterialManager::ReloadMaterialShaders()
{
	if(m_shaderHandler == nullptr)
		return;
	GetContext().WaitIdle();
	for(auto &assetInfo : GetAssets()) {
		if(!assetInfo.asset)
			continue;
		auto hMat = GetAssetObject(*assetInfo.asset);
		if(hMat && hMat->IsLoaded() == true)
			m_shaderHandler(hMat.get());
	}
}
void pragma::material::CMaterialManager::MarkForReload(CMaterial &mat) { m_reloadShaderQueue.push(mat.GetHandle()); }
void pragma::material::CMaterialManager::Poll()
{
	MaterialManager::Poll();
	m_textureManager->Poll();
	if(!m_reloadShaderQueue.empty()) {
		std::unordered_set<Material *> traversed;
		while(!m_reloadShaderQueue.empty()) {
			auto hMat = m_reloadShaderQueue.front();
			m_reloadShaderQueue.pop();

			if(!hMat.IsValid())
				continue;
			auto *pmat = hMat.get();
			auto it = traversed.find(pmat);
			if(it != traversed.end())
				continue;
			traversed.insert(pmat);
			m_shaderHandler(pmat);
		}
	}
}
