// SPDX-FileCopyrightText: (c) 2021 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "definitions.hpp"

export module pragma.cmaterialsystem:texture_manager.manager2;

export import :texture_manager.texture;

export namespace pragma::material {
	struct DLLCMATSYS TextureLoadInfo : public pragma::util::AssetLoadInfo {
		TextureLoadInfo(pragma::util::AssetLoadFlags flags = pragma::util::AssetLoadFlags::None);
		TextureMipmapMode mipmapMode;
		std::shared_ptr<udm::Property> textureData;
	};
	class DLLCMATSYS TextureManager : public pragma::util::TFileAssetManager<Texture, TextureLoadInfo> {
	  public:
		using AssetType = Texture;

		TextureManager(prosper::IPrContext &context);
		virtual ~TextureManager() override;

		std::shared_ptr<Texture> GetErrorTexture();
		void SetErrorTexture(const std::shared_ptr<Texture> &tex);

		void Test();
	  protected:
		virtual void InitializeProcessor(pragma::util::IAssetProcessor &processor) override;
		virtual pragma::util::AssetObject InitializeAsset(const pragma::util::Asset &asset, const pragma::util::AssetLoadJob &job) override;

		prosper::IPrContext &m_context;
		std::shared_ptr<Texture> m_error;
	};
};
