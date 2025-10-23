// SPDX-FileCopyrightText: (c) 2020 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <string>

export module pragma.materialsystem:vmat;

export import pragma.util;

export namespace vmat {
	util::Path get_vmat_texture_path(const std::string &strPath, std::string *optOutInputPath = nullptr)
	{
		::util::Path path {strPath};
		path.RemoveFileExtension();
		path += ".vtex_c";

		std::string inputPath = path.GetString();
		auto front = path.GetFront();
		path.PopFront();

		auto outputPath = path;
		if(ustring::compare<std::string_view>(front, "materials", false) == false)
			outputPath = "models/" + outputPath;

		if(optOutInputPath)
			*optOutInputPath = inputPath;
		return outputPath;
	}
};
