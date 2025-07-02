// SPDX-FileCopyrightText: (c) 2020 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __UTIL_VMAT_HPP__
#define __UTIL_VMAT_HPP__

#include <string>
#include <sharedutils/util_path.hpp>
#include <sharedutils/util_string.h>

namespace vmat {
	static util::Path get_vmat_texture_path(const std::string &strPath, std::string *optOutInputPath = nullptr)
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

#endif
