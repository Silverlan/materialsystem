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
#include <gli/gli.hpp>
#include <gli/texture2d.hpp>
#ifdef ENABLE_VTF_SUPPORT
#include <VTFLib/VTFFile.h>
#endif

namespace prosper
{
	class Context;
	class Sampler;
};

namespace uimg {class Image;};
class TCallback;
class DLLCMATSYS TextureQueueItem
{
protected:
	TextureQueueItem();
public:
	TextureQueueItem(const TextureQueueItem&)=delete;
	TextureQueueItem &operator=(const TextureQueueItem&)=delete;
	virtual ~TextureQueueItem();
	std::weak_ptr<prosper::Context> context;
	std::string name;
	std::string path;
	std::string cache;
	unsigned char **mipmap;
	unsigned int *dxtformat;
	unsigned int *format;
	int mipmapid;
	TextureMipmapMode mipmapMode;
	std::shared_ptr<prosper::Sampler> sampler = nullptr;
	bool valid;
	bool initialized;
	bool cubemap;
	TextureType texturetype;
};

class DLLCMATSYS TextureQueueItemPNG
	: public TextureQueueItem
{
public:
	TextureQueueItemPNG();
	virtual ~TextureQueueItemPNG() override;
	std::shared_ptr<uimg::Image> pnginfo;
};

class DLLCMATSYS TextureQueueItemTGA
	: public TextureQueueItem
{
public:
	TextureQueueItemTGA();
	std::shared_ptr<uimg::Image> tgainfo;
};
#ifdef ENABLE_VTF_SUPPORT
class DLLCMATSYS TextureQueueItemVTF
	: public TextureQueueItem
{
public:
	TextureQueueItemVTF();
	virtual ~TextureQueueItemVTF() override;
	std::vector<std::shared_ptr<VTFLib::CVTFFile>> textures;
};
#endif
class DLLCMATSYS TextureQueueItemSurface
	: public TextureQueueItem
{
public:
	TextureQueueItemSurface(TextureType type);
	TextureQueueItemSurface(const TextureQueueItemSurface&)=delete;
	TextureQueueItemSurface &operator=(const TextureQueueItemSurface&)=delete;
	virtual ~TextureQueueItemSurface() override;
	std::vector<std::unique_ptr<gli::texture2d>> textures;
	unsigned char **compressed;
	unsigned int *compressedsize;
};

#endif