// SPDX-FileCopyrightText: (c) 2021 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "definitions.hpp"
#include <VTFFile.h>


export module pragma.cmaterialsystem:texture_manager.format_handlers.vtf;

export import :texture_manager.texture_format_handler;

#ifndef DISABLE_VTF_SUPPORT
export namespace msys {
	namespace detail {
		struct VulkanImageData {
			// Original image format
			prosper::Format format = prosper::Format::R8G8B8A8_UNorm;

			// Some formats may not be supported in optimal layout by common GPUs, in which case we need to convert it
			std::optional<prosper::Format> conversionFormat = {};

			std::array<prosper::ComponentSwizzle, 4> swizzle = {prosper::ComponentSwizzle::R, prosper::ComponentSwizzle::G, prosper::ComponentSwizzle::B, prosper::ComponentSwizzle::A};
		};
	};
	class DLLCMATSYS TextureFormatHandlerVtf : public ITextureFormatHandler {
	  public:
		TextureFormatHandlerVtf(util::IAssetManager &assetManager);
		virtual bool GetDataPtr(uint32_t layer, uint32_t mipmapIdx, void **outPtr, size_t &outSize) override;
	  protected:
		virtual bool LoadData(InputTextureInfo &texInfo) override;
	  private:
		std::shared_ptr<VTFLib::CVTFFile> m_texture = nullptr;
	};
};
#endif
