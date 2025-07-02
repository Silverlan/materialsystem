// SPDX-FileCopyrightText: © 2021 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __MSYS_TEXTURE_FORMAT_HANDLER_HPP__
#define __MSYS_TEXTURE_FORMAT_HANDLER_HPP__

#include "cmatsysdefinitions.h"
#include <sharedutils/asset_loader/asset_format_handler.hpp>
#include <cinttypes>
#include <prosper_structs.hpp>
#include <fsys/ifile.hpp>

#undef AddJob

namespace udm {
	struct Property;
};
namespace msys {
	class DLLCMATSYS ITextureFormatHandler : public util::IAssetFormatHandler {
	  public:
		static void SetFlipTexturesVertically(bool flip);
		static bool ShouldFlipTextureVertically();
		struct InputTextureInfo {
			enum class Flags : uint32_t {
				None = 0u,
				CubemapBit = 1u,
				SrgbBit = CubemapBit << 1u,
			};
			uint32_t width = 0;
			uint32_t height = 0;
			prosper::Format format = prosper::Format::Unknown;
			uint32_t layerCount = 1;
			uint32_t mipmapCount = 1;
			Flags flags = Flags::None;
			std::array<prosper::ComponentSwizzle, 4> swizzle = {prosper::ComponentSwizzle::R, prosper::ComponentSwizzle::G, prosper::ComponentSwizzle::B, prosper::ComponentSwizzle::A};

			std::optional<prosper::Format> conversionFormat {};
			std::shared_ptr<udm::Property> textureData;
		};

		bool LoadData();
		virtual bool GetDataPtr(uint32_t layer, uint32_t mipmapIdx, void **outPtr, size_t &outSize) = 0;
		void SetTextureData(const std::shared_ptr<udm::Property> &textureData);
		const InputTextureInfo &GetInputTextureInfo() const { return m_inputTextureInfo; }
	  protected:
		ITextureFormatHandler(util::IAssetManager &assetManager);
		void Flip(const InputTextureInfo &texInfo);
		virtual bool LoadData(InputTextureInfo &texInfo) = 0;
		InputTextureInfo m_inputTextureInfo;
		bool m_flipTextureVertically = false;
	};
};
REGISTER_BASIC_BITWISE_OPERATORS(msys::ITextureFormatHandler::InputTextureInfo::Flags)

#endif
