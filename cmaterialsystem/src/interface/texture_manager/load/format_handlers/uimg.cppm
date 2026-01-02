// SPDX-FileCopyrightText: (c) 2021 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.cmaterialsystem:texture_manager.format_handlers.uimg;

export import :texture_manager.texture_format_handler;
export import pragma.image;

export namespace pragma::material {
	class DLLCMATSYS TextureFormatHandlerUimg : public ITextureFormatHandler {
	  public:
		TextureFormatHandlerUimg(pragma::util::IAssetManager &assetManager) : ITextureFormatHandler {assetManager} {}
		virtual bool GetDataPtr(uint32_t layer, uint32_t mipmapIdx, void **outPtr, size_t &outSize) override;
	  protected:
		virtual bool LoadData(InputTextureInfo &texInfo) override;
	  private:
		std::shared_ptr<image::ImageBuffer> m_imgBuf = nullptr;
	};
};
