/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "impl_texture_formats.h"
#include <fsys/filesystem.h>

std::vector<ImageFormat> get_perfered_image_format_order()
{
	return std::vector<ImageFormat>{ // Order of preference
		{TextureType::KTX,".ktx"},
		{TextureType::DDS,".dds"},
		{TextureType::PNG,".png"},
		{TextureType::TGA,".tga"},
#ifdef ENABLE_VTF_SUPPORT
		{TextureType::VTF,".vtf"}
#endif
	};
}

std::string translate_image_path(const std::string &imgFile,bool bCubemap,TextureType &type,std::string path)
{
	path += FileManager::GetNormalizedPath(imgFile);
	ustring::to_lower(path);
	auto formats = get_perfered_image_format_order();
	type = formats.front().type;

	auto ext = path.substr(path.length() -4);
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
			if(bCubemap == true)
				formatPath += "bk";
			formatPath += format.extension;
			if(FileManager::Exists(formatPath))
			{
				path = (bCubemap == false) ? formatPath : path;
				type = format.type;
				bFoundType = true;
				break;
			}
		}
		if(bFoundType == false)
		{
			if(bCubemap == false)
				path += formats.front().extension;
#ifdef ENABLE_VTF_SUPPORT
			else
				type = TextureType::VTF; // Hack: Assume it's a VTF cubemap if no texture files have been found!
#endif
		}
	}
	if(bCubemap == true)
	{
		if(type != TextureType::DDS)
		{
#ifdef ENABLE_VTF_SUPPORT
			if(type != TextureType::VTF)
#endif
				type = TextureType::KTX;
		}
	}
	return path;
}
