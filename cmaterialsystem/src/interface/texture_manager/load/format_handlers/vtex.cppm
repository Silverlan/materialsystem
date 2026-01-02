// SPDX-FileCopyrightText: (c) 2021 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.cmaterialsystem:texture_manager.format_handlers.vtex;

export import :texture_manager.texture_format_handler;

#ifndef DISABLE_VTEX_SUPPORT
import source2;

export namespace pragma::material {
	class DLLCMATSYS TextureFormatHandlerVtex : public ITextureFormatHandler {
	  public:
		TextureFormatHandlerVtex(pragma::util::IAssetManager &assetManager) : ITextureFormatHandler {assetManager} {}
		virtual bool GetDataPtr(uint32_t layer, uint32_t mipmapIdx, void **outPtr, size_t &outSize) override;
	  protected:
		virtual bool LoadData(InputTextureInfo &texInfo) override;
	  private:
		std::shared_ptr<source2::resource::Texture> m_texture = nullptr;
		std::vector<uint8_t> m_mipmapData;
	};
};
#endif
