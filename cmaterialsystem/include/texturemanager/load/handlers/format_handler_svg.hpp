/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __MSYS_TEXTURE_FORMAT_HANDLER_SVG_HPP__
#define __MSYS_TEXTURE_FORMAT_HANDLER_SVG_HPP__

#include "texturemanager/load/texture_format_handler.hpp"

namespace uimg {
	class ImageBuffer;
};
namespace msys {
	class DLLCMATSYS TextureFormatHandlerSvg : public ITextureFormatHandler {
	  public:
		TextureFormatHandlerSvg(util::IAssetManager &assetManager) : ITextureFormatHandler {assetManager} {}
		virtual bool GetDataPtr(uint32_t layer, uint32_t mipmapIdx, void **outPtr, size_t &outSize) override;
	  protected:
		virtual bool LoadData(InputTextureInfo &texInfo) override;
	  private:
		std::shared_ptr<uimg::ImageBuffer> m_imgBuf = nullptr;
	};
};

#endif
