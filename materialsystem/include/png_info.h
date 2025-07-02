// SPDX-FileCopyrightText: © 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __PNG_INFO_H__
#define __PNG_INFO_H__

#include "matsysdefinitions.h"
#include <cinttypes>
#include <png.h>
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
