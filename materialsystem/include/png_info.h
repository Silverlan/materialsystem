/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __PNG_INFO_H__
#define __PNG_INFO_H__

#include "matsysdefinitions.h"
#include <cinttypes>
#include <png.h>
#include <pngstruct.h>
#include <memory>

struct DLLMATSYS PNGInfo {
	uint32_t format; // vk::Format
	unsigned int width;
	unsigned int height;
	png_byte *image_data;
	png_structp png_ptr;
	png_infop info_ptr;
	png_infop end_info;
	png_byte **row_pointers;
	void Release();
};

class VFilePtrInternal;
namespace material {
	DLLMATSYS bool load_png_data(const char *path, PNGInfo &info);
	DLLMATSYS bool load_png_data(std::shared_ptr<VFilePtrInternal> &f, PNGInfo &info);
};

#endif
