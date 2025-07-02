// SPDX-FileCopyrightText: © 2021 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __SOURCE2_VMAT_FORMAT_HANDLER_HPP__
#define __SOURCE2_VMAT_FORMAT_HANDLER_HPP__

#ifndef DISABLE_VMAT_SUPPORT
#include "matsysdefinitions.h"
#include <sharedutils/asset_loader/asset_format_handler.hpp>

#ifdef _WIN32
namespace source2::resource {
	class Resource;
	class Material;
};
#else
import source2;
#endif

namespace ds {
	class Block;
	class Settings;
};
namespace msys {
	enum class VMatOrigin : uint8_t { Source2 = 0, SteamVR, Dota2 };
	class DLLMATSYS Source2VmatFormatHandler : public util::IImportAssetFormatHandler {
	  public:
		Source2VmatFormatHandler(util::IAssetManager &assetManager);
		virtual bool Import(const std::string &outputPath, std::string &outFilePath) override;
	  protected:
		virtual bool ImportTexture(const std::string &fpath, const std::string &outputPath) { return false; }
		virtual bool InitializeVMatData(::source2::resource::Resource &resource, ::source2::resource::Material &vmat, ds::Block &rootData, ds::Settings &settings, const std::string &shader, VMatOrigin origin);
	  private:
		bool LoadVMat(::source2::resource::Resource &resource, const std::string &outputPath, std::string &outFilePath);
	};
};
#endif

#endif
