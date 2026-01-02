// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <png.h>

export module pragma.materialsystem:png_info;

export import pragma.filesystem;

export namespace pragma::material {
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

	DLLMATSYS bool load_png_data(const char *path, PNGInfo &info);
	DLLMATSYS bool load_png_data(std::shared_ptr<fs::VFilePtrInternal> &f, PNGInfo &info);
}
