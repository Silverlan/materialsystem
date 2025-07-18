// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#include "impl_texture_formats.h"
#include <fsys/filesystem.h>
#include <sharedutils/util_file.h>

const std::vector<MaterialManager::ImageFormat> &MaterialManager::get_supported_image_formats()
{
	static std::vector<ImageFormat> s_supportedImageFormats = {
	  // Order of preference
	  {TextureType::KTX, "ktx"},
	  {TextureType::DDS, "dds"},
	  {TextureType::PNG, "png"},
	  {TextureType::TGA, "tga"},
	  {TextureType::JPG, "jpg"},
	  {TextureType::BMP, "bmp"},
	  {TextureType::PSD, "psd"},
	  {TextureType::GIF, "gif"},
	  {TextureType::HDR, "hdr"},
	  {TextureType::PIC, "pic"},
#ifndef DISABLE_VTF_SUPPORT
	  {TextureType::VTF, "vtf"},
#endif
#ifndef DISABLE_VTEX_SUPPORT
	  {TextureType::VTex, "vtex_c"},
#endif
	};
	static_assert(umath::to_integral(TextureType::Count) == 13, "Update this implementation when new texture types have been added!");
	return s_supportedImageFormats;
}
std::string translate_image_path(const std::string &imgFile, TextureType &type, std::string path, const std::function<VFilePtr(const std::string &)> &fileHandler, bool *optOutFound)
{
	path += FileManager::GetNormalizedPath(imgFile);
	ustring::to_lower(path);
	auto formats = MaterialManager::get_supported_image_formats();
	type = formats.front().type;

	std::string ext {};
	ufile::get_extension(path, &ext);
	auto bFoundType = false;
	auto it = std::find_if(formats.begin(), formats.end(), [&ext](const MaterialManager::ImageFormat &format) { return ext == format.extension; });
	if(it != formats.end()) {
		type = it->type;
		bFoundType = true;
	}
	if(bFoundType == false) {
		auto fFindFormat = [&formats, &path, &type, &bFoundType]() -> bool {
			for(auto &format : formats) {
				auto formatPath = path;
				formatPath += '.' + format.extension;
				if(FileManager::Exists(formatPath)) {
					path = formatPath;
					type = format.type;
					bFoundType = true;
					return true;
				}
			}
			return false;
		};
		if(fFindFormat() == false && fileHandler) {
			// HACK: File handler may import the texture, so we'll have to check again afterwards
			fileHandler(path);
			fFindFormat();
		}
	}
	if(optOutFound)
		*optOutFound = bFoundType;
	return path;
}
std::string translate_image_path(const std::string &imgFile, TextureType &type, const std::function<VFilePtr(const std::string &)> &fileHandler, bool *optOutFound) { return translate_image_path(imgFile, type, MaterialManager::GetRootMaterialLocation() + '/', fileHandler, optOutFound); }
