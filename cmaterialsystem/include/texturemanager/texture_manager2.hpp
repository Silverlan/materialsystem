/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2021 Silverlan
 */

#ifndef __TEXTURE_MANAGER2_HPP__
#define __TEXTURE_MANAGER2_HPP__

#include "cmatsysdefinitions.h"
#include <sharedutils/asset_loader/file_asset_manager.hpp>
#include <sharedutils/asset_loader/asset_load_info.hpp>
#include <sharedutils/util_path.hpp>
#include <unordered_set>

class Texture;
namespace prosper {class IPrContext;};
namespace util {using AssetLoadJobPriority = int32_t; using AssetLoadJobId = uint64_t; enum class AssetLoaderWaitMode : uint8_t; class IAssetLoader; class IAssetProcessor; struct AssetLoadJob;};
namespace ufile {struct IFile;};
enum class TextureMipmapMode : int32_t;
namespace msys
{
	struct DLLCMATSYS TextureLoadInfo
		: public util::AssetLoadInfo
	{
		TextureLoadInfo(util::AssetLoadFlags flags=util::AssetLoadFlags::None);
		TextureMipmapMode mipmapMode;
	};
	class TextureLoader;
	class ITextureFormatHandler;
	class DLLCMATSYS TextureManager
		: public util::TFileAssetManager<Texture,TextureLoadInfo>
	{
	public:
		using AssetType = Texture;

		TextureManager(prosper::IPrContext &context);
		virtual ~TextureManager() override;

		std::shared_ptr<Texture> GetErrorTexture();
		void SetErrorTexture(const std::shared_ptr<Texture> &tex);

		void Test();
	protected:
		virtual void InitializeProcessor(util::IAssetProcessor &processor) override;
		virtual util::AssetObject InitializeAsset(const util::Asset &asset,const util::AssetLoadJob &job) override;

		prosper::IPrContext &m_context;
		std::shared_ptr<Texture> m_error;
	};
};

#endif
