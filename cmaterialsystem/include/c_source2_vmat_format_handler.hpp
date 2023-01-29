/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __C_SOURCE2_VMAT_FORMAT_HANDLER_HPP__
#define __C_SOURCE2_VMAT_FORMAT_HANDLER_HPP__

#ifndef DISABLE_VMAT_SUPPORT
#include "cmatsysdefinitions.h"
#include <source2_vmat_format_handler.hpp>

namespace msys {
	class DLLCMATSYS CSource2VmatFormatHandler : public Source2VmatFormatHandler {
	  public:
		CSource2VmatFormatHandler(util::IAssetManager &assetManager);
	  protected:
		virtual bool ImportTexture(const std::string &fpath, const std::string &outputPath) override;
		virtual bool InitializeVMatData(::source2::resource::Resource &resource, ::source2::resource::Material &vmat, ds::Block &rootData, ds::Settings &settings, const std::string &shader, VMatOrigin origin) override;
	};
};

#endif
#endif
