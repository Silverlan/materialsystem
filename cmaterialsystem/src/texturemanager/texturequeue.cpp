/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "texturemanager/texturequeue.h"
#include <sharedutils/functioncallback.h>
#include <util_image.hpp>
#ifndef DISABLE_VTF_SUPPORT
#include <VTFWrapper.h>
#endif
#ifndef DISABLE_VTEX_SUPPORT
#include <util_source2.hpp>
#endif

TextureQueueItem::TextureQueueItem()
	: valid(true),mipmapMode(TextureMipmapMode::Load),cubemap(false),texturetype(TextureType::Invalid),
	mipmap(NULL),mipmapid(-1),dxtformat(NULL),format(NULL),initialized(false),context()
{}

TextureQueueItem::~TextureQueueItem()
{
	if(dxtformat != NULL)
		delete[] dxtformat;
	if(format != NULL)
		delete[] format;
}

////////////////////////

TextureQueueItemPNG::TextureQueueItemPNG()
	: TextureQueueItem(),pnginfo(NULL)
{
	texturetype = TextureType::PNG;
}

TextureQueueItemPNG::~TextureQueueItemPNG()
{
	if(mipmap != NULL)
	{
		delete[] mipmap[0];
		delete[] mipmap;
	}
}

////////////////////////

TextureQueueItemTGA::TextureQueueItemTGA()
	: TextureQueueItem(),tgainfo()
{
	texturetype = TextureType::TGA;
}

////////////////////////

#ifndef DISABLE_VTF_SUPPORT
TextureQueueItemVTF::TextureQueueItemVTF()
	: TextureQueueItem()
{
	texturetype = TextureType::VTF;

}

TextureQueueItemVTF::~TextureQueueItemVTF()
{
	if(mipmap != nullptr)
	{
		delete[] mipmap[0];
		delete[] mipmap;
	}
}
#endif

////////////////////////

#ifndef DISABLE_VTEX_SUPPORT
TextureQueueItemVTex::TextureQueueItemVTex()
	: TextureQueueItem()
{
	texturetype = TextureType::VTex;

}

TextureQueueItemVTex::~TextureQueueItemVTex()
{
	if(mipmap != nullptr)
	{
		delete[] mipmap[0];
		delete[] mipmap;
	}
}
#endif

////////////////////////

TextureQueueItemSurface::TextureQueueItemSurface(TextureType type)
	: TextureQueueItem(),/*ddsimg(NULL),*/compressed(nullptr),compressedsize(nullptr)
{
	texturetype = type;
}

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
	if(mipmap != NULL)
	{
		if(!cubemap)
			delete mipmap[0];
		else
		{
			for(unsigned int i=0;i<6;i++)
				delete[] mipmap[i];
		}
		delete[] mipmap;
	}
	if(compressed != NULL)
	{
		if(!cubemap)
			delete compressed[0];
		else
		{
			for(unsigned int i=0;i<6;i++)
				delete[] compressed[i];
		}
		delete[] compressed;
	}
	if(compressedsize != NULL)
		delete[] compressedsize;
}