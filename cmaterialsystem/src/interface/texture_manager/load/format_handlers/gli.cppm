// SPDX-FileCopyrightText: (c) 2021 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "cmatsysdefinitions.hpp"
#include <gli/load.hpp>

export module pragma.cmaterialsystem:texture_manager.format_handlers.gli;

export import :texture_manager.texture_format_handler;

export namespace msys {
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
