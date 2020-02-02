/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "impl_texture_formats.h"
#include <fsys/filesystem.h>
#include <sharedutils/util_file.h>

const std::vector<MaterialManager::ImageFormat> &MaterialManager::get_supported_image_formats()
{
	static std::vector<ImageFormat> s_supportedImageFormats = { // Order of preference
		{TextureType::KTX,"ktx"},
		{TextureType::DDS,"dds"},
		{TextureType::PNG,"png"},
		{TextureType::TGA,"tga"},
	#ifdef ENABLE_VTF_SUPPORT
		{TextureType::VTF,"vtf"}
	#endif
	};
	return s_supportedImageFormats;
}

std::string translate_image_path(const std::string &imgFile,TextureType &type,std::string path)
{
	path += FileManager::GetNormalizedPath(imgFile);
	ustring::to_lower(path);
	auto formats = MaterialManager::get_supported_image_formats();
	type = formats.front().type;

	std::string ext {};
	ufile::get_extension(path,&ext);
	auto bFoundType = false;
	for(auto &format : formats)
	{
		if(ext == format.extension)
		{
			type = format.type;
			bFoundType = true;
			break;
		}
	}
	if(bFoundType == false)
	{
		for(auto &format : formats)
		{
			auto formatPath = path;
			formatPath += '.' +format.extension;
			if(FileManager::Exists(formatPath))
			{
				path = formatPath;
				type = format.type;
				bFoundType = true;
				break;
			}
		}
		// if(bFoundType == false)
		// 	path += '.' +formats.front().extension;
	}
	return path;
}
std::string translate_image_path(const std::string &imgFile,TextureType &type)
{
	return translate_image_path(imgFile,type,MaterialManager::GetRootMaterialLocation());
}
