// SPDX-FileCopyrightText: (c) 2021 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#ifndef DISABLE_VMT_SUPPORT
#include "cmatsysdefinitions.hpp"
#endif

export module pragma.cmaterialsystem:format_handlers.source_vmt;

export import pragma.materialsystem;

#ifndef DISABLE_VMT_SUPPORT
export namespace msys {
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
