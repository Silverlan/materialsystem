// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.materialsystem:util;

export import :enums;
export import pragma.filesystem;

export namespace pragma::material {
	DLLMATSYS std::string translate_image_path(const std::string &imgFile, TextureType &type, std::string path, const std::function<std::shared_ptr<fs::VFilePtrInternal>(const std::string &)> &fileHandler = nullptr, bool *optOutFound = nullptr);
	DLLMATSYS std::string translate_image_path(const std::string &imgFile, TextureType &type, const std::function<std::shared_ptr<fs::VFilePtrInternal>(const std::string &)> &fileHandler = nullptr, bool *optOutFound = nullptr);
}
