// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;
#ifndef DISABLE_VTF_SUPPORT
#include <VTFWrapper.h>
#endif

module pragma.cmaterialsystem;

import :texture_manager.texture_queue;

pragma::material::TextureQueueItem::TextureQueueItem() : valid(true), mipmapMode(material::TextureMipmapMode::Load), cubemap(false), texturetype(material::TextureType::Invalid), mipmap(NULL), mipmapid(-1), dxtformat(NULL), format(NULL), initialized(false), context() {}

pragma::material::TextureQueueItem::~TextureQueueItem()
{
	if(dxtformat != NULL)
		delete[] dxtformat;
	if(format != NULL)
		delete[] format;
}

////////////////////////

pragma::material::TextureQueueItemPNG::TextureQueueItemPNG() : TextureQueueItem(), pnginfo(NULL) { texturetype = material::TextureType::PNG; }

pragma::material::TextureQueueItemPNG::~TextureQueueItemPNG()
{
	if(mipmap != NULL) {
		delete[] mipmap[0];
		delete[] mipmap;
	}
}

////////////////////////

pragma::material::TextureQueueItemStbi::TextureQueueItemStbi(material::TextureType texType) : TextureQueueItem(), imageBuffer() { texturetype = texType; }

////////////////////////

#ifndef DISABLE_VTF_SUPPORT
pragma::material::TextureQueueItemVTF::TextureQueueItemVTF() : TextureQueueItem() { texturetype = material::TextureType::VTF; }

pragma::material::TextureQueueItemVTF::~TextureQueueItemVTF()
{
	if(mipmap != nullptr) {
		delete[] mipmap[0];
		delete[] mipmap;
	}
}
#endif

////////////////////////

#ifndef DISABLE_VTEX_SUPPORT
pragma::material::TextureQueueItemVTex::TextureQueueItemVTex() : TextureQueueItem() { texturetype = material::TextureType::VTex; }

pragma::material::TextureQueueItemVTex::~TextureQueueItemVTex()
{
	if(mipmap != nullptr) {
		delete[] mipmap[0];
		delete[] mipmap;
	}
}
#endif

////////////////////////

pragma::material::TextureQueueItemSurface::TextureQueueItemSurface(material::TextureType type) : TextureQueueItem(), /*ddsimg(NULL),*/ compressed(nullptr), compressedsize(nullptr) { texturetype = type; }

pragma::material::TextureQueueItemSurface::~TextureQueueItemSurface()
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
