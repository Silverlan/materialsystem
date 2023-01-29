/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __TEXTUREQUEUE_H__
#define __TEXTUREQUEUE_H__

#include "cmatsysdefinitions.h"
#include <string>
#include <sharedutils/functioncallback.h>
#include "texture_type.h"
#include <png_info.h>
#include <mathutil/glmutil.h>
#include <fsys/ifile.hpp>
#include <gli/gli.hpp>
#include <gli/texture2d.hpp>
#ifndef DISABLE_VTF_SUPPORT
#include <VTFFile.h>
#endif

namespace prosper {
	class IPrContext;
	class ISampler;
};

class VFilePtrInternal;

namespace uimg {
	class ImageBuffer;
};
class TCallback;
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
	std::shared_ptr<VFilePtrInternal> file = nullptr; // Optional
	unsigned char **mipmap;
	unsigned int *dxtformat;
	unsigned int *format;
	int mipmapid;
	TextureMipmapMode mipmapMode;
	std::shared_ptr<prosper::ISampler> sampler = nullptr;
	bool valid;
	bool initialized;
	bool cubemap;
	bool addToCache = true;
	TextureType texturetype;
};

class DLLCMATSYS TextureQueueItemPNG : public TextureQueueItem {
  public:
	TextureQueueItemPNG();
	virtual ~TextureQueueItemPNG() override;
	std::shared_ptr<uimg::ImageBuffer> pnginfo = nullptr;
};

class DLLCMATSYS TextureQueueItemStbi : public TextureQueueItem {
  public:
	TextureQueueItemStbi(TextureType texType);
	std::shared_ptr<uimg::ImageBuffer> imageBuffer = nullptr;
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
namespace source2::resource {
	class Texture;
};
class DLLCMATSYS TextureQueueItemVTex : public TextureQueueItem {
  public:
	TextureQueueItemVTex();
	virtual ~TextureQueueItemVTex() override;
	std::shared_ptr<source2::resource::Texture> texture = nullptr;
	std::unique_ptr<fsys::File> fptr = nullptr;
};
#endif
class DLLCMATSYS TextureQueueItemSurface : public TextureQueueItem {
  public:
	TextureQueueItemSurface(TextureType type);
	TextureQueueItemSurface(const TextureQueueItemSurface &) = delete;
	TextureQueueItemSurface &operator=(const TextureQueueItemSurface &) = delete;
	virtual ~TextureQueueItemSurface() override;
	std::shared_ptr<gli::texture2d> texture = nullptr;
	unsigned char **compressed;
	unsigned int *compressedsize;
};

#endif
