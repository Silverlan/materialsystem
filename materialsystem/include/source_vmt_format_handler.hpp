/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __SOURCE_VMT_FORMAT_HANDLER_HPP__
#define __SOURCE_VMT_FORMAT_HANDLER_HPP__

#ifndef DISABLE_VMT_SUPPORT
#include "matsysdefinitions.h"
#include <sharedutils/asset_loader/asset_format_handler.hpp>

namespace VTFLib {
	class CVMTFile;
};
namespace ds {
	class Block;
};
namespace ValveKeyValueFormat {
	class KVNode;
	class KVBranch;
};
namespace msys {
	class DLLMATSYS SourceVmtFormatHandler : public util::IImportAssetFormatHandler {
	  public:
		SourceVmtFormatHandler(util::IAssetManager &assetManager);
		virtual bool Import(const std::string &outputPath, std::string &outFilePath) override;
	  protected:
		virtual bool LoadVMTData(VTFLib::CVMTFile &vmt, const std::string &vmtShader, ds::Block &rootData, std::string &matShader);
		bool LoadVMT(VTFLib::CVMTFile &vmt, const std::string &outputPath, std::string &outFilePath);
	};
#ifdef ENABLE_VKV_PARSER
	class DLLMATSYS SourceVmtFormatHandler2 : public util::IImportAssetFormatHandler {
	  public:
		SourceVmtFormatHandler2(util::IAssetManager &assetManager);
		virtual bool Import(const std::string &outputPath, std::string &outFilePath) override;
	  protected:
		static std::optional<std::string> GetStringValue(ValveKeyValueFormat::KVBranch &node, std::string key);
		virtual bool LoadVMTData(ValveKeyValueFormat::KVNode &vmt, const std::string &vmtShader, ds::Block &rootData, std::string &matShader);
		bool LoadVMT(ValveKeyValueFormat::KVNode &vmt, const std::string &outputPath, std::string &outFilePath);
	};
#endif
};
#endif

#endif
