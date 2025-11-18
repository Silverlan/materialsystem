// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <png.h>

module pragma.materialsystem;

import :png_info;

void PNGInfo::Release()
{
	png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
	free(image_data);
	free(row_pointers);
}

static void ReadPNGDataFromFile(png_structp png_ptr, png_bytep outBytes, png_size_t byteCountToRead)
{
	if(png_get_io_ptr(png_ptr) == NULL)
		return;
	VFilePtr &fp = *static_cast<VFilePtr *>(png_get_io_ptr(png_ptr));
	fp->Read(&outBytes[0], byteCountToRead);
}

bool material::load_png_data(std::shared_ptr<VFilePtrInternal> &fp, PNGInfo &info)
{
	png_byte header[8];

	// read the header
	fp->Read(&header[0], 8);

	if(png_sig_cmp(header, 0, 8)) {
		// fprintf(stderr, "error: %s is not a PNG.\n", file_name);
		return false;
	}

	info.png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if(!info.png_ptr) {
		// fprintf(stderr, "error: png_create_read_struct returned 0.\n");
		return false;
	}

	// create png info struct
	info.info_ptr = png_create_info_struct(info.png_ptr);
	if(!info.info_ptr) {
		// fprintf(stderr, "error: png_create_info_struct returned 0.\n");
		png_destroy_read_struct(&info.png_ptr, (png_infopp)NULL, (png_infopp)NULL);
		return false;
	}

	// create png info struct
	info.end_info = png_create_info_struct(info.png_ptr);
	if(!info.end_info) {
		// fprintf(stderr, "error: png_create_info_struct returned 0.\n")
		png_destroy_read_struct(&info.png_ptr, &info.info_ptr, (png_infopp)NULL);
		return false;
	}

	// the code in this if statement gets called if libpng encounters an error
#pragma warning(disable : 4611)
	if(setjmp(png_jmpbuf(info.png_ptr))) {
#pragma warning(default : 4611)
		//fprintf(stderr, "error from libpng\n");
		png_destroy_read_struct(&info.png_ptr, &info.info_ptr, &info.end_info);
		return false;
	}

	// init png reading
	png_set_read_fn(info.png_ptr, &fp, ReadPNGDataFromFile);

	// let libpng know you already read the first 8 bytes
	png_set_sig_bytes(info.png_ptr, 8);

	// read all the info up to the image data
	png_read_info(info.png_ptr, info.info_ptr);

	// variables to pass to get info
	int bit_depth, color_type;
	png_uint_32 temp_width, temp_height;

	// get info about png
	png_get_IHDR(info.png_ptr, info.info_ptr, &temp_width, &temp_height, &bit_depth, &color_type, NULL, NULL, NULL);

	info.width = temp_width;
	info.height = temp_height;

	if(bit_depth != 8) {
		// fprintf(stderr, "%s: Unsupported bit depth %d.  Must be 8.\n", file_name, bit_depth);
		return false;
	}

	if(!(color_type & PNG_COLOR_MASK_COLOR))
		return false;
	info.format = !(color_type & PNG_COLOR_MASK_ALPHA) ? 23u : // vk::Format::eR8G8B8Unorm
	  37u;                                                     // vk::Format::eR8G8B8A8Unorm

	// Update the png info struct.
	png_read_update_info(info.png_ptr, info.info_ptr);

	// Row size in bytes.
	int rowbytes = static_cast<int>(png_get_rowbytes(info.png_ptr, info.info_ptr));

	// glTexImage2d requires rows to be 4-byte aligned
	rowbytes += 3 - ((rowbytes - 1) % 4);

	// Allocate the image_data as a big block, to be given to opengl
	info.image_data = (png_byte *)malloc(rowbytes * temp_height * sizeof(png_byte) + 15);
	if(info.image_data == NULL) {
		// fprintf(stderr,"error: could not allocate memory for PNG image data\n");
		png_destroy_read_struct(&info.png_ptr, &info.info_ptr, &info.end_info);
		return false;
	}

	// row_pointers is for pointing to image_data for reading the png with libpng
	info.row_pointers = (png_byte **)malloc(temp_height * sizeof(png_byte *));
	if(info.row_pointers == NULL) {
		// fprintf(stderr, "error: could not allocate memory for PNG row pointers\n");
		png_destroy_read_struct(&info.png_ptr, &info.info_ptr, &info.end_info);
		free(info.image_data);
		return false;
	}

	// set the individual row_pointers to point at the correct offsets of image_data
	for(unsigned int i = 0; i < temp_height; i++)
		info.row_pointers[temp_height - 1 - i] = info.image_data + i * rowbytes;

	// read the png into image_data through row_pointers
	png_read_image(info.png_ptr, info.row_pointers);

	if(info.format == 23u) {
		auto numPixels = info.width * info.height;
		auto *rgbaData = static_cast<uint8_t *>(malloc(numPixels * 4u));
		for(auto i = decltype(numPixels) {0u}; i < numPixels; ++i) {
			auto srcPxIdx = i * 3u;
			auto dstPxIdx = i * 4u;
			for(auto j = 0u; j < 3u; ++j)
				rgbaData[dstPxIdx + j] = info.image_data[srcPxIdx + j];
			rgbaData[dstPxIdx + 3u] = std::numeric_limits<uint8_t>::max();
		}
		free(info.image_data);
		info.image_data = rgbaData;
		info.format = 37u;
	}
	return true;
}
bool material::load_png_data(const char *path, PNGInfo &info)
{
	auto fp = FileManager::OpenFile(path, "rb");
	if(fp == NULL)
		return false;
	return load_png_data(fp, info);
}
