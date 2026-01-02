// SPDX-FileCopyrightText: (c) 2022 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module pragma.cmaterialsystem;

import :format_handlers.source_vmt;

#ifndef DISABLE_VMT_SUPPORT
#ifdef ENABLE_VKV_PARSER
#include "util_vmt.hpp"
#include <VKVParser/library.h>
material::CSourceVmtFormatHandler2::CSourceVmtFormatHandler2(pragma::util::IAssetManager &assetManager) : SourceVmtFormatHandler2 {assetManager} {}
bool material::CSourceVmtFormatHandler2::LoadVmtData(const std::string &vmtShader, datasystem::Block &rootData, std::string &matShader)
{
	auto r = SourceVmtFormatHandler2::LoadVmtData(vmtShader, rootData, matShader);
	if(!r)
		return false;
	return load_vmt_data(*this, vmtShader, rootData, matShader);
}
#endif
#endif
