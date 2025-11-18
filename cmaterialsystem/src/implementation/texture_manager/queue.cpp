// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;
#ifndef DISABLE_VTF_SUPPORT
#include <VTFWrapper.h>
#endif

module pragma.cmaterialsystem;

import :texture_manager.texture_queue;

TextureQueueItem::TextureQueueItem() : valid(true), mipmapMode(msys::TextureMipmapMode::Load), cubemap(false), texturetype(msys::TextureType::Invalid), mipmap(NULL), mipmapid(-1), dxtformat(NULL), format(NULL), initialized(false), context() {}

TextureQueueItem::~TextureQueueItem()
{
	if(dxtformat != NULL)
		delete[] dxtformat;
	if(format != NULL)
		delete[] format;
}

////////////////////////

TextureQueueItemPNG::TextureQueueItemPNG() : TextureQueueItem(), pnginfo(NULL) { texturetype = msys::TextureType::PNG; }

TextureQueueItemPNG::~TextureQueueItemPNG()
{
	if(mipmap != NULL) {
		delete[] mipmap[0];
		delete[] mipmap;
	}
}

////////////////////////

TextureQueueItemStbi::TextureQueueItemStbi(msys::TextureType texType) : TextureQueueItem(), imageBuffer() { texturetype = texType; }

////////////////////////

#ifndef DISABLE_VTF_SUPPORT
TextureQueueItemVTF::TextureQueueItemVTF() : TextureQueueItem() { texturetype = msys::TextureType::VTF; }

TextureQueueItemVTF::~TextureQueueItemVTF()
{
	if(mipmap != nullptr) {
		delete[] mipmap[0];
		delete[] mipmap;
	}
}
#endif

////////////////////////

#ifndef DISABLE_VTEX_SUPPORT
TextureQueueItemVTex::TextureQueueItemVTex() : TextureQueueItem() { texturetype = msys::TextureType::VTex; }

TextureQueueItemVTex::~TextureQueueItemVTex()
{
	if(mipmap != nullptr) {
		delete[] mipmap[0];
		delete[] mipmap;
	}
}
#endif

////////////////////////

TextureQueueItemSurface::TextureQueueItemSurface(msys::TextureType type) : TextureQueueItem(), /*ddsimg(NULL),*/ compressed(nullptr), compressedsize(nullptr) { texturetype = type; }

TextureQueueItemSurface::~TextureQueueItemSurface()
{
	/*if(ddsimg != NULL)
	{
		if(!cubemap)
			delete ddsimg[0];
		else
		{
			for(unsigned int i=0;i<6;i++)
				delete ddsimg[i];
		}
		delete[] ddsimg;
	}*/
	if(mipmap != NULL) {
		if(!cubemap)
			delete mipmap[0];
		else {
			for(unsigned int i = 0; i < 6; i++)
				delete[] mipmap[i];
		}
		delete[] mipmap;
	}
	if(compressed != NULL) {
		if(!cubemap)
			delete compressed[0];
		else {
			for(unsigned int i = 0; i < 6; i++)
				delete[] compressed[i];
		}
		delete[] compressed;
	}
	if(compressedsize != NULL)
		delete[] compressedsize;
}
