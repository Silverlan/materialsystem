// SPDX-FileCopyrightText: (c) 2021 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;


#ifndef DISABLE_VMAT_SUPPORT
#include "definitions.hpp"
#endif

export module pragma.materialsystem:format_handlers.source2_vmat;

import pragma.datasystem;
import source2;

#ifndef DISABLE_VMAT_SUPPORT
export namespace msys {
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
