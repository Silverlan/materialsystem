/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

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
