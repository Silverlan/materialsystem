// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __TEXTURE_TYPE_H__
#define __TEXTURE_TYPE_H__

#include <cinttypes>

enum class TextureType : uint32_t {
	Invalid = 0,
	DDS,
	KTX,
	PNG,
	TGA,
	JPG,
	BMP,
	PSD,
	GIF,
	HDR,
	PIC,
#ifndef DISABLE_VTF_SUPPORT
	VTF,
#endif
#ifndef DISABLE_VTEX_SUPPORT
	VTex,
#endif

	Count
};

enum class TextureMipmapMode : int32_t {
	Ignore = -1,        // Don't load or generate mipmaps
	LoadOrGenerate = 0, // Try to load mipmaps, generate if no mipmaps exist in the file
	Load = 1,           // Load if mipmaps exist, otherwise do nothing
	Generate = 2        // Always generate mipmaps, ignore mipmaps in file
};

#endif
