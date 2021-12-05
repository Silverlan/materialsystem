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
namespace prosper {class IPrContext;};
namespace util {using AssetLoadJobPriority = int32_t; using AssetLoadJobId = uint64_t;};
namespace ufile {struct IFile;};
enum class TextureMipmapMode : int32_t;
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
	enum class TextureLoadFlags : uint32_t
	{
		None = 0u,
		DontCache = 1u,
		IgnoreCache = DontCache<<1u,
		AbsolutePath = IgnoreCache<<1u
	};
	struct DLLCMATSYS TextureLoadInfo
	{
		TextureLoadInfo(TextureLoadFlags flags=TextureLoadFlags::None);
		TextureLoadFlags flags = TextureLoadFlags::None;
		TextureMipmapMode mipmapMode;
		std::function<void(TextureAsset&)> onLoaded = nullptr;
		std::function<void()> onFailure = nullptr;
	};
	class TextureLoader;
	class ITextureFormatHandler;
	class DLLCMATSYS TextureManager
		: public util::IAssetManager
	{
	public:
		using AssetType = TextureAsset;
		static constexpr util::AssetLoadJobPriority DEFAULT_PRIORITY = 0;
		static constexpr util::AssetLoadJobPriority IMMEDIATE_PRIORITY = 10;
		struct DLLCMATSYS PreloadResult
		{
			std::optional<util::AssetLoadJobId> jobId {};
			bool success = false;
		};

		TextureManager(prosper::IPrContext &context);
		virtual ~TextureManager();

		void SetFileHandler(const std::function<std::shared_ptr<ufile::IFile>(const std::string&)> &fileHandler);

		void RemoveFromCache(const std::string &path);
		PreloadResult PreloadTexture(const std::string &path,const TextureLoadInfo &loadInfo={});
		std::shared_ptr<Texture> LoadTexture(const std::string &path,const TextureLoadInfo &loadInfo={});
		std::shared_ptr<Texture> LoadTexture(const std::string &cacheName,const std::shared_ptr<ufile::IFile> &file,const std::string &ext,const TextureLoadInfo &loadInfo={});
		void SetRootDirectory(const std::string &dir);
		const util::Path &GetRootDirectory() const;

		TextureLoader &GetLoader() {return *m_loader;}
		const TextureLoader &GetLoader() const {return const_cast<TextureManager*>(this)->GetLoader();}
		std::shared_ptr<Texture> GetErrorTexture();
		void SetErrorTexture(const std::shared_ptr<Texture> &tex);
		void Poll();

		void Test();
	protected:
		PreloadResult PreloadTexture(const std::string &path,util::AssetLoadJobPriority priority,const TextureLoadInfo &loadInfo);
		PreloadResult PreloadTexture(
			const std::string &cacheName,const std::shared_ptr<ufile::IFile> &file,const std::string &ext,util::AssetLoadJobPriority priority,
			const TextureLoadInfo &loadInfo
		);
		std::shared_ptr<Texture> LoadTexture(const std::string &path,const PreloadResult &r,const TextureLoadInfo &loadInfo);
		void RegisterFormatHandler(const std::string &ext,const std::function<std::unique_ptr<ITextureFormatHandler>()> &factory);

		std::function<std::shared_ptr<ufile::IFile>(const std::string&)> m_fileHandler = nullptr;
		std::unique_ptr<TextureLoader> m_loader;
		prosper::IPrContext &m_context;
		std::shared_ptr<Texture> m_error;
		util::Path m_rootDir;
	};
};
REGISTER_BASIC_BITWISE_OPERATORS(msys::TextureLoadFlags)

#endif
