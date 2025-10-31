// SPDX-FileCopyrightText: (c) 2021 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "cmatsysdefinitions.hpp"


export module pragma.cmaterialsystem:texture_manager.manager2;

export import :texture_manager.texture;

export namespace msys {
	struct DLLCMATSYS TextureLoadInfo : public util::AssetLoadInfo {
		TextureLoadInfo(util::AssetLoadFlags flags = util::AssetLoadFlags::None);
		TextureMipmapMode mipmapMode;
		std::shared_ptr<udm::Property> textureData;
	};
	class DLLCMATSYS TextureManager : public util::TFileAssetManager<Texture, TextureLoadInfo> {
	  public:
		using AssetType = Texture;

		TextureManager(prosper::IPrContext &context);
		virtual ~TextureManager() override;

		std::shared_ptr<Texture> GetErrorTexture();
		void SetErrorTexture(const std::shared_ptr<Texture> &tex);

		void Test();
	  protected:
		virtual void InitializeProcessor(util::IAssetProcessor &processor) override;
		virtual util::AssetObject InitializeAsset(const util::Asset &asset, const util::AssetLoadJob &job) override;

		prosper::IPrContext &m_context;
		std::shared_ptr<Texture> m_error;
	};
};
