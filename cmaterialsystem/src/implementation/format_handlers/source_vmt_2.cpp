// SPDX-FileCopyrightText: (c) 2022 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.cmaterialsystem;

import :format_handlers.source_vmt;

#ifndef DISABLE_VMT_SUPPORT
#ifdef ENABLE_VKV_PARSER
#include "util_vmt.hpp"
#include <VKVParser/library.h>
msys::CSourceVmtFormatHandler2::CSourceVmtFormatHandler2(util::IAssetManager &assetManager) : SourceVmtFormatHandler2 {assetManager} {}
bool msys::CSourceVmtFormatHandler2::LoadVmtData(const std::string &vmtShader, ds::Block &rootData, std::string &matShader)
{
	auto r = SourceVmtFormatHandler2::LoadVmtData(vmtShader, rootData, matShader);
	if(!r)
		return false;
	return load_vmt_data(*this, vmtShader, rootData, matShader);
}
#endif
#endif
