// SPDX-FileCopyrightText: (c) 2020 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __DETAIL_MODE_HPP__
#define __DETAIL_MODE_HPP__

#include "matsysdefinitions.h"
#include <string>
#include <cinttypes>
#include <limits>

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
};

#endif
