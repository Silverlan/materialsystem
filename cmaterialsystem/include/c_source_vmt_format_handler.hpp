// SPDX-FileCopyrightText: (c) 2021 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __C_SOURCE_VMT_FORMAT_HANDLER_HPP__
#define __C_SOURCE_VMT_FORMAT_HANDLER_HPP__

#ifndef DISABLE_VMT_SUPPORT
#include "cmatsysdefinitions.h"
#include <source_vmt_format_handler.hpp>

namespace ds {
	class Block;
};
namespace msys {
	template<class T>
	bool load_vmt_data(T &formatHandler, const std::string &vmtShader, ds::Block &rootData, std::string &matShader);
	class DLLCMATSYS CSourceVmtFormatHandler : public SourceVmtFormatHandler {
	  public:
		template<class T>
		friend bool load_vmt_data(T &formatHandler, const std::string &vmtShader, ds::Block &rootData, std::string &matShader);
		CSourceVmtFormatHandler(util::IAssetManager &assetManager);
	  protected:
		virtual bool LoadVmtData(const std::string &vmtShader, ds::Block &rootData, std::string &matShader) override;
	};
#ifdef ENABLE_VKV_PARSER
	class DLLCMATSYS CSourceVmtFormatHandler2 : public SourceVmtFormatHandler2 {
	  public:
		template<class T>
		friend bool load_vmt_data(T &formatHandler, const std::string &vmtShader, ds::Block &rootData, std::string &matShader);
		CSourceVmtFormatHandler2(util::IAssetManager &assetManager);
	  protected:
		virtual bool LoadVmtData(const std::string &vmtShader, ds::Block &rootData, std::string &matShader) override;
	};
#endif
};

#endif
#endif
