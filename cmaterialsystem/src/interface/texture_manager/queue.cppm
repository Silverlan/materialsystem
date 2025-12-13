// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "definitions.hpp"
#ifndef DISABLE_VTF_SUPPORT
#include <VTFFile.h>
#endif

export module pragma.cmaterialsystem:texture_manager.texture_queue;

export import gli;
export import pragma.image;
export import pragma.materialsystem;
export import pragma.prosper;
#ifndef DISABLE_VTEX_SUPPORT
import source2;
#endif

export namespace pragma::material {
	class DLLCMATSYS TextureQueueItem {
	  protected:
		TextureQueueItem();
	  public:
		TextureQueueItem(const TextureQueueItem &) = delete;
		TextureQueueItem &operator=(const TextureQueueItem &) = delete;
		virtual ~TextureQueueItem();
		std::weak_ptr<prosper::IPrContext> context;
		std::string name;
		std::string path;
		std::string cache;
		std::shared_ptr<fs::VFilePtrInternal> file = nullptr; // Optional
		unsigned char **mipmap;
		unsigned int *dxtformat;
		unsigned int *format;
		int mipmapid;
		material::TextureMipmapMode mipmapMode;
		std::shared_ptr<prosper::ISampler> sampler = nullptr;
		bool valid;
		bool initialized;
		bool cubemap;
		bool addToCache = true;
		material::TextureType texturetype;
	};

	class DLLCMATSYS TextureQueueItemPNG : public TextureQueueItem {
	  public:
		TextureQueueItemPNG();
		virtual ~TextureQueueItemPNG() override;
		std::shared_ptr<image::ImageBuffer> pnginfo = nullptr;
	};

	class DLLCMATSYS TextureQueueItemStbi : public TextureQueueItem {
	  public:
		TextureQueueItemStbi(material::TextureType texType);
		std::shared_ptr<image::ImageBuffer> imageBuffer = nullptr;
	};

#ifndef DISABLE_VTF_SUPPORT
	class DLLCMATSYS TextureQueueItemVTF : public TextureQueueItem {
	  public:
		TextureQueueItemVTF();
		virtual ~TextureQueueItemVTF() override;
		std::shared_ptr<VTFLib::CVTFFile> texture = nullptr;
	};
#endif
#ifndef DISABLE_VTEX_SUPPORT
	class DLLCMATSYS TextureQueueItemVTex : public TextureQueueItem {
	  public:
		TextureQueueItemVTex();
		virtual ~TextureQueueItemVTex() override;
		std::shared_ptr<source2::resource::Texture> texture = nullptr;
		std::unique_ptr<fs::File> fptr = nullptr;
	};
#endif
	class DLLCMATSYS TextureQueueItemSurface : public TextureQueueItem {
	  public:
		TextureQueueItemSurface(material::TextureType type);
		TextureQueueItemSurface(const TextureQueueItemSurface &) = delete;
		TextureQueueItemSurface &operator=(const TextureQueueItemSurface &) = delete;
		virtual ~TextureQueueItemSurface() override;
		std::shared_ptr<gli::texture2d> texture = nullptr;
		unsigned char **compressed;
		unsigned int *compressedsize;
	};
}
