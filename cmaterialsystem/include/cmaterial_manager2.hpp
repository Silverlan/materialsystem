// SPDX-FileCopyrightText: (c) 2021 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __MSYS_CMATERIAL_MANAGER_HPP__
#define __MSYS_CMATERIAL_MANAGER_HPP__

#include "cmatsysdefinitions.h"
#include <material_manager2.hpp>

namespace prosper {
	class IPrContext;
};
class CMaterial;
namespace msys {
	class TextureManager;
	class DLLCMATSYS CMaterialManager : public MaterialManager {
	  public:
		static void SetFlipTexturesVerticallyOnLoad(bool flip);
		static bool ShouldFlipTextureVerticallyOnLoad();

		static std::shared_ptr<CMaterialManager> Create(prosper::IPrContext &context);
		virtual ~CMaterialManager() override;

		virtual std::shared_ptr<Material> CreateMaterialObject(const std::string &shader, const std::shared_ptr<ds::Block> &data) override;
		void MarkForReload(CMaterial &mat);

		void SetShaderHandler(const std::function<void(Material *)> &handler);
		const std::function<void(Material *)> &GetShaderHandler() const { return m_shaderHandler; }
		void ReloadMaterialShaders();

		prosper::IPrContext &GetContext() { return m_context; }
		msys::TextureManager &GetTextureManager() { return *m_textureManager; }
		virtual void Poll() override;
	  private:
		CMaterialManager(prosper::IPrContext &context);
		virtual void InitializeImportHandlers() override;
		std::function<void(Material *)> m_shaderHandler;
		prosper::IPrContext &m_context;
		std::unique_ptr<msys::TextureManager> m_textureManager;
		std::queue<std::weak_ptr<Material>> m_reloadShaderQueue;
	};
};

#endif
