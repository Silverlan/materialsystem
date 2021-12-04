/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2021 Silverlan
 */

#ifndef __TEXTURE_MANAGER2_HPP__
#define __TEXTURE_MANAGER2_HPP__

#include "cmatsysdefinitions.h"
#include <sharedutils/asset_loader/asset_manager.hpp>
#include <sharedutils/util_path.hpp>
#include <unordered_set>

class Texture;
namespace util {using AssetLoadJobPriority = int32_t; using AssetLoadJobId = uint64_t;};
namespace msys
{
	struct DLLCMATSYS TextureAsset
		: public util::IAsset
	{
	public:
		TextureAsset(const std::shared_ptr<Texture> &texture)
			: texture{texture}
		{}
		virtual bool IsInUse() const override;
		std::shared_ptr<Texture> texture;
	};
	class TextureLoader;
	class ITextureFormatHandler;
	class DLLCMATSYS TextureManager
		: public util::IAssetManager
	{
	public:
		using AssetType = TextureAsset;
		struct DLLCMATSYS PreloadResult
		{
			std::optional<util::AssetLoadJobId> jobId {};
			bool success = false;
		};
		TextureManager(prosper::IPrContext &context);
		virtual ~TextureManager();

		PreloadResult PreloadTexture(const std::string &path);
		Texture *LoadTexture(const std::string &path);
		void SetRootDirectory(const std::string &dir);
		const util::Path &GetRootDirectory() const;

		TextureLoader &GetLoader() {return *m_loader;}
		const TextureLoader &GetLoader() const {return const_cast<TextureManager*>(this)->GetLoader();}
		std::shared_ptr<Texture> GetErrorTexture();
		void SetErrorTexture(const std::shared_ptr<Texture> &tex);
		void Poll();

		void Test();
	protected:
		PreloadResult PreloadTexture(const std::string &path,util::AssetLoadJobPriority priority);
		void RegisterFormatHandler(const std::string &ext,const std::function<std::unique_ptr<ITextureFormatHandler>()> &factory);

		std::unique_ptr<TextureLoader> m_loader;
		prosper::IPrContext &m_context;
		std::shared_ptr<Texture> m_error;
		util::Path m_rootDir;
	};
};

#endif
