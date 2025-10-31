// SPDX-FileCopyrightText: (c) 2020 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "matsysdefinitions.hpp"

export module pragma.materialsystem:enums;

import pragma.math;

export {
	namespace msys {
		enum class DetailMode : uint8_t {
			DecalModulate = 0u,
			Additive,
			TranslucentDetail,
			BlendFactorFade,
			TranslucentBase,
			UnlitAdditive,              // Currently not supported
			UnlitAdditiveThresholdFade, // Currently not supported
			TwoPatternDecalModulate,
			Multiply,
			BaseMaskViaDetailAlpha,
			SelfShadowedBumpmap, // Currently not supported
			SSBumpAlbedo,        // Currently not supported

			Count,
			Invalid = std::numeric_limits<uint8_t>::max()
		};

		DLLMATSYS std::string to_string(DetailMode detailMode);
		DLLMATSYS DetailMode to_detail_mode(const std::string &detailMode);

		enum class TextureType : uint32_t {
			Invalid = 0,
			DDS,
			KTX,
			PNG,
			TGA,
			JPG,
			BMP,
			PSD,
			GIF,
			HDR,
			PIC,
		#ifndef DISABLE_VTF_SUPPORT
			VTF,
		#endif
		#ifndef DISABLE_VTEX_SUPPORT
			VTex,
		#endif

			Count
		};

		enum class TextureMipmapMode : int32_t {
			Ignore = -1,        // Don't load or generate mipmaps
			LoadOrGenerate = 0, // Try to load mipmaps, generate if no mipmaps exist in the file
			Load = 1,           // Load if mipmaps exist, otherwise do nothing
			Generate = 2        // Always generate mipmaps, ignore mipmaps in file
		};

		enum class TextureLoadFlags : uint32_t { None = 0u, LoadInstantly = 1u, Reload = LoadInstantly << 1u, DontCache = Reload << 1u };

		using namespace umath::scoped_enum::bitwise;
	}
	namespace umath::scoped_enum::bitwise {
		template<>
		struct enable_bitwise_operators<msys::TextureLoadFlags> : std::true_type {};
	}
}
