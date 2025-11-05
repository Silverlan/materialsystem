// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "definitions.hpp"

export module pragma.materialsystem:util;

export import :enums;
export import pragma.filesystem;

export {
	DLLMATSYS std::string translate_image_path(const std::string &imgFile, msys::TextureType &type, std::string path, const std::function<std::shared_ptr<VFilePtrInternal>(const std::string &)> &fileHandler = nullptr, bool *optOutFound = nullptr);
	DLLMATSYS std::string translate_image_path(const std::string &imgFile, msys::TextureType &type, const std::function<std::shared_ptr<VFilePtrInternal>(const std::string &)> &fileHandler = nullptr, bool *optOutFound = nullptr);
}
