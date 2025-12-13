// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.materialsystem;

import :util;

std::string pragma::material::translate_image_path(const std::string &imgFile, pragma::material::TextureType &type, std::string path, const std::function<pragma::fs::VFilePtr(const std::string &)> &fileHandler, bool *optOutFound)
{
	path += pragma::fs::get_normalized_path(imgFile);
	pragma::string::to_lower(path);
	auto formats = ::MaterialManager::get_supported_image_formats();
	type = formats.front().type;

	std::string ext {};
	ufile::get_extension(path, &ext);
	auto bFoundType = false;
	auto it = std::find_if(formats.begin(), formats.end(), [&ext](const ::MaterialManager::ImageFormat &format) { return ext == format.extension; });
	if(it != formats.end()) {
		type = it->type;
		bFoundType = true;
	}
	if(bFoundType == false) {
		auto fFindFormat = [&formats, &path, &type, &bFoundType]() -> bool {
			for(auto &format : formats) {
				auto formatPath = path;
				formatPath += '.' + format.extension;
				if(pragma::fs::exists(formatPath)) {
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
std::string pragma::material::translate_image_path(const std::string &imgFile, pragma::material::TextureType &type, const std::function<pragma::fs::VFilePtr(const std::string &)> &fileHandler, bool *optOutFound)
{
	return translate_image_path(imgFile, type, ::MaterialManager::GetRootMaterialLocation() + '/', fileHandler, optOutFound);
}
