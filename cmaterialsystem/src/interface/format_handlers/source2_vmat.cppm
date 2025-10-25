// SPDX-FileCopyrightText: (c) 2021 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#ifndef DISABLE_VMAT_SUPPORT
#include "cmatsysdefinitions.hpp"
#endif
#include <string>

export module pragma.cmaterialsystem:format_handlers.source2_vmat;

export import pragma.materialsystem;
import source2;

#ifndef DISABLE_VMAT_SUPPORT
export namespace msys {
	class DLLCMATSYS CSource2VmatFormatHandler : public Source2VmatFormatHandler {
	  public:
		CSource2VmatFormatHandler(util::IAssetManager &assetManager);
	  protected:
		virtual bool ImportTexture(const std::string &fpath, const std::string &outputPath) override;
		virtual bool InitializeVMatData(::source2::resource::Resource &resource, ::source2::resource::Material &vmat, ds::Block &rootData, ds::Settings &settings, const std::string &shader, VMatOrigin origin) override;
	};
};

#endif
