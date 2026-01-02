// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#ifndef DISABLE_VMT_SUPPORT
#include <VMTFile.h>
#endif

export module pragma.cmaterialsystem:material_manager;

export import :material;

export {
#pragma warning(push)
#pragma warning(disable : 4251)
	class DLLCMATSYS CMaterialManager : public MaterialManager, public prosper::ContextObject {
	  private:
		std::function<void(pragma::material::Material *)> m_shaderHandler;
		using MaterialManager::CreateMaterial;
		std::unique_ptr<pragma::material::TextureManager> m_textureManager = nullptr;
		std::queue<pragma::material::MaterialHandle> m_reloadShaderQueue;
		virtual pragma::material::Material *CreateMaterial(const std::string *identifier, const std::string &shader, std::shared_ptr<pragma::datasystem::Block> root = nullptr) override;
#ifndef DISABLE_VMT_SUPPORT
		virtual bool InitializeVMTData(VTFLib::CVMTFile &vmt, LoadInfo &info, pragma::datasystem::Block &rootData, pragma::datasystem::Settings &settings, const std::string &shader) override;
#endif
#ifndef DISABLE_VMAT_SUPPORT
		virtual bool InitializeVMatData(source2::resource::Resource &resource, source2::resource::Material &vmat, LoadInfo &info, pragma::datasystem::Block &rootData, pragma::datasystem::Settings &settings, const std::string &shader, VMatOrigin origin) override;
#endif
	  public:
		CMaterialManager(prosper::IPrContext &context);
		virtual ~CMaterialManager() override;
		pragma::material::Material *CreateMaterial(const std::string &identifier, const std::string &shader, const std::shared_ptr<pragma::datasystem::Block> &root = nullptr);
		pragma::material::Material *CreateMaterial(const std::string &shader, const std::shared_ptr<pragma::datasystem::Block> &root = nullptr);
		pragma::material::TextureManager &GetTextureManager();
		void Update();
		virtual void SetErrorMaterial(pragma::material::Material *mat) override;
		void SetShaderHandler(const std::function<void(pragma::material::Material *)> &handler);
		std::function<void(pragma::material::Material *)> GetShaderHandler() const;
		pragma::material::Material *Load(const std::string &path, const std::function<void(pragma::material::Material *)> &onMaterialLoaded, const std::function<void(std::shared_ptr<pragma::material::Texture>)> &onTextureLoaded = nullptr, bool bReload = false, bool *bFirstTimeError = nullptr, bool bLoadInstantly = false);
		virtual pragma::material::Material *Load(const std::string &path, bool bReload = false, bool loadInstantly = true, bool *bFirstTimeError = nullptr) override;
		void ReloadMaterialShaders();
		void MarkForReload(pragma::material::CMaterial &mat);

		void SetDownscaleImportedRMATextures(bool downscale);
	};
#pragma warning(pop)
}
