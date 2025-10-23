// SPDX-FileCopyrightText: (c) 2025 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "cmatsysdefinitions.hpp"

export module pragma.cmaterialsystem:texture_manager.format_handlers.svg;

export import :texture_manager.texture_format_handler;
export import pragma.image;

export namespace msys {
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
