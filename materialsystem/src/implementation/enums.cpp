// SPDX-FileCopyrightText: (c) 2020 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.materialsystem;

import :enums;

std::string pragma::material::to_string(DetailMode detailMode)
{
	switch(detailMode) {
	case DetailMode::DecalModulate:
		return "decal_modulate";
	case DetailMode::Additive:
		return "additive";
	case DetailMode::TranslucentDetail:
		return "translucent_detail";
	case DetailMode::BlendFactorFade:
		return "blend_factor_fade";
	case DetailMode::TranslucentBase:
		return "translucent_base";
	case DetailMode::UnlitAdditive:
		return "unlit_additive";
	case DetailMode::UnlitAdditiveThresholdFade:
		return "unlit_additive_threshold_fade";
	case DetailMode::TwoPatternDecalModulate:
		return "two_pattern_decal_modulate";
	case DetailMode::Multiply:
		return "multiply";
	case DetailMode::BaseMaskViaDetailAlpha:
		return "base_mask_via_detail_alpha";
	case DetailMode::SelfShadowedBumpmap:
		return "self_shadowed_bumpmap";
	case DetailMode::SSBumpAlbedo:
		return "ssbump_albedo";
	}
	return "";
}
pragma::material::DetailMode pragma::material::to_detail_mode(const std::string &detailMode)
{
	if(pragma::string::compare<std::string>(detailMode, "decal_modulate", false))
		return DetailMode::DecalModulate;
	else if(pragma::string::compare<std::string>(detailMode, "additive", false))
		return DetailMode::Additive;
	else if(pragma::string::compare<std::string>(detailMode, "translucent_detail", false))
		return DetailMode::TranslucentDetail;
	else if(pragma::string::compare<std::string>(detailMode, "blend_factor_fade", false))
		return DetailMode::BlendFactorFade;
	else if(pragma::string::compare<std::string>(detailMode, "translucent_base", false))
		return DetailMode::TranslucentBase;
	else if(pragma::string::compare<std::string>(detailMode, "unlit_additive", false))
		return DetailMode::UnlitAdditive;
	else if(pragma::string::compare<std::string>(detailMode, "unlit_additive_threshold_fade", false))
		return DetailMode::UnlitAdditiveThresholdFade;
	else if(pragma::string::compare<std::string>(detailMode, "two_pattern_decal_modulate", false))
		return DetailMode::TwoPatternDecalModulate;
	else if(pragma::string::compare<std::string>(detailMode, "multiply", false))
		return DetailMode::Multiply;
	else if(pragma::string::compare<std::string>(detailMode, "base_mask_via_detail_alpha", false))
		return DetailMode::BaseMaskViaDetailAlpha;
	else if(pragma::string::compare<std::string>(detailMode, "self_shadowed_bumpmap", false))
		return DetailMode::SelfShadowedBumpmap;
	else if(pragma::string::compare<std::string>(detailMode, "ssbump_albedo", false))
		return DetailMode::SSBumpAlbedo;
	return DetailMode::Invalid;
}
