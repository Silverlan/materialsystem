// SPDX-FileCopyrightText: (c) 2021 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __MSYS_TEXTURE_FORMAT_HANDLER_VTEX_HPP__
#define __MSYS_TEXTURE_FORMAT_HANDLER_VTEX_HPP__

#ifndef DISABLE_VTEX_SUPPORT
#include "texturemanager/load/texture_format_handler.hpp"

namespace source2::resource {
	class Texture;
};
namespace msys {
	class DLLCMATSYS TextureFormatHandlerVtex : public ITextureFormatHandler {
	  public:
		TextureFormatHandlerVtex(util::IAssetManager &assetManager) : ITextureFormatHandler {assetManager} {}
		virtual bool GetDataPtr(uint32_t layer, uint32_t mipmapIdx, void **outPtr, size_t &outSize) override;
	  protected:
		virtual bool LoadData(InputTextureInfo &texInfo) override;
	  private:
		std::shared_ptr<source2::resource::Texture> m_texture = nullptr;
		std::vector<uint8_t> m_mipmapData;
	};
};
#endif

#endif
