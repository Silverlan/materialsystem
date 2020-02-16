/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __TEXTURE_TYPE_H__
#define __TEXTURE_TYPE_H__

#include <cinttypes>

enum class TextureType : uint32_t
{
	Invalid = 0,
	DDS,
	KTX,
	PNG,
	TGA,
#ifndef DISABLE_VTF_SUPPORT
	VTF,
#endif
#ifndef DISABLE_VTEX_SUPPORT
	VTex
#endif
};

enum class TextureMipmapMode : int32_t
{
	Ignore = -1, // Don't load or generate mipmaps
	LoadOrGenerate = 0, // Try to load mipmaps, generate if no mipmaps exist in the file
	Load = 1, // Load if mipmaps exist, otherwise do nothing
	Generate = 2 // Always generate mipmaps, ignore mipmaps in file
};

#endif
