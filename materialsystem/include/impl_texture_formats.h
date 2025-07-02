// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __IMPL_TEXTURE_FORMATS_H__
#define __IMPL_TEXTURE_FORMATS_H__

#include "matsysdefinitions.h"
#include <vector>
#include <functional>
#include <memory>
#include "texture_type.h"
#include "materialmanager.h"
#include <sharedutils/util_string.h>

class VFilePtrInternal;
DLLMATSYS std::string translate_image_path(const std::string &imgFile, TextureType &type, std::string path, const std::function<std::shared_ptr<VFilePtrInternal>(const std::string &)> &fileHandler = nullptr, bool *optOutFound = nullptr);
DLLMATSYS std::string translate_image_path(const std::string &imgFile, TextureType &type, const std::function<std::shared_ptr<VFilePtrInternal>(const std::string &)> &fileHandler = nullptr, bool *optOutFound = nullptr);

#endif
