// SPDX-FileCopyrightText: © 2021 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __MSYS_TEXTURE_FORMAT_HANDLER_GLI_HPP__
#define __MSYS_TEXTURE_FORMAT_HANDLER_GLI_HPP__

#include "texturemanager/load/texture_format_handler.hpp"
#include <gli/load.hpp>

namespace msys {
	class DLLCMATSYS TextureFormatHandlerGli : public ITextureFormatHandler {
	  public:
		TextureFormatHandlerGli(util::IAssetManager &assetManager) : ITextureFormatHandler {assetManager} {}
		virtual bool GetDataPtr(uint32_t layer, uint32_t mipmapIdx, void **outPtr, size_t &outSize) override;
		void SetTexture(gli::texture &&tex);

		void Flip(const InputTextureInfo &texInfo);
	  protected:
		virtual bool LoadData(InputTextureInfo &texInfo) override;
	  private:
		gli::texture m_texture;
	};
};

#endif
