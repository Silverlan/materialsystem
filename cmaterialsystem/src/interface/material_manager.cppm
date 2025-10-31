// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "cmatsysdefinitions.hpp"
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
		std::function<void(msys::Material *)> m_shaderHandler;
		using MaterialManager::CreateMaterial;
		std::unique_ptr<msys::TextureManager> m_textureManager = nullptr;
		std::queue<msys::MaterialHandle> m_reloadShaderQueue;
		virtual msys::Material *CreateMaterial(const std::string *identifier, const std::string &shader, std::shared_ptr<ds::Block> root = nullptr) override;
	#ifndef DISABLE_VMT_SUPPORT
		virtual bool InitializeVMTData(VTFLib::CVMTFile &vmt, LoadInfo &info, ds::Block &rootData, ds::Settings &settings, const std::string &shader) override;
	#endif
	#ifndef DISABLE_VMAT_SUPPORT
		virtual bool InitializeVMatData(source2::resource::Resource &resource, source2::resource::Material &vmat, LoadInfo &info, ds::Block &rootData, ds::Settings &settings, const std::string &shader, VMatOrigin origin) override;
	#endif
	  public:
		CMaterialManager(prosper::IPrContext &context);
		virtual ~CMaterialManager() override;
		msys::Material *CreateMaterial(const std::string &identifier, const std::string &shader, const std::shared_ptr<ds::Block> &root = nullptr);
		msys::Material *CreateMaterial(const std::string &shader, const std::shared_ptr<ds::Block> &root = nullptr);
		msys::TextureManager &GetTextureManager();
		void Update();
		virtual void SetErrorMaterial(msys::Material *mat) override;
		void SetShaderHandler(const std::function<void(msys::Material *)> &handler);
		std::function<void(msys::Material *)> GetShaderHandler() const;
		msys::Material *Load(const std::string &path, const std::function<void(msys::Material *)> &onMaterialLoaded, const std::function<void(std::shared_ptr<msys::Texture>)> &onTextureLoaded = nullptr, bool bReload = false, bool *bFirstTimeError = nullptr, bool bLoadInstantly = false);
		virtual msys::Material *Load(const std::string &path, bool bReload = false, bool loadInstantly = true, bool *bFirstTimeError = nullptr) override;
		void ReloadMaterialShaders();
		void MarkForReload(msys::CMaterial &mat);

		void SetDownscaleImportedRMATextures(bool downscale);
	};
	#pragma warning(pop)
}
