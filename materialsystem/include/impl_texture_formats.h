/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __IMPL_TEXTURE_FORMATS_H__
#define __IMPL_TEXTURE_FORMATS_H__

#include "matsysdefinitions.h"
#include <vector>
#include "texture_type.h"
#include <sharedutils/util_string.h>

struct DLLMATSYS ImageFormat
{
	ImageFormat(TextureType _type,std::string _extension)
		: type(_type),extension(_extension)
	{}
	TextureType type;
	std::string extension;
};

DLLMATSYS std::vector<ImageFormat> get_perfered_image_format_order();
DLLMATSYS std::string translate_image_path(const std::string &imgFile,bool bCubemap,TextureType &type,std::string path="materials/");

#endif
