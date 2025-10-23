// SPDX-FileCopyrightText: (c) 2020 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.materialsystem;

import :enums;

std::string msys::to_string(msys::DetailMode detailMode)
{
	switch(detailMode) {
	case msys::DetailMode::DecalModulate:
		return "decal_modulate";
	case msys::DetailMode::Additive:
		return "additive";
	case msys::DetailMode::TranslucentDetail:
		return "translucent_detail";
	case msys::DetailMode::BlendFactorFade:
		return "blend_factor_fade";
	case msys::DetailMode::TranslucentBase:
		return "translucent_base";
	case msys::DetailMode::UnlitAdditive:
		return "unlit_additive";
	case msys::DetailMode::UnlitAdditiveThresholdFade:
		return "unlit_additive_threshold_fade";
	case msys::DetailMode::TwoPatternDecalModulate:
		return "two_pattern_decal_modulate";
	case msys::DetailMode::Multiply:
		return "multiply";
	case msys::DetailMode::BaseMaskViaDetailAlpha:
		return "base_mask_via_detail_alpha";
	case msys::DetailMode::SelfShadowedBumpmap:
		return "self_shadowed_bumpmap";
	case msys::DetailMode::SSBumpAlbedo:
		return "ssbump_albedo";
	}
	return "";
}
msys::DetailMode msys::to_detail_mode(const std::string &detailMode)
{
	if(ustring::compare<std::string>(detailMode, "decal_modulate", false))
		return msys::DetailMode::DecalModulate;
	else if(ustring::compare<std::string>(detailMode, "additive", false))
		return msys::DetailMode::Additive;
	else if(ustring::compare<std::string>(detailMode, "translucent_detail", false))
		return msys::DetailMode::TranslucentDetail;
	else if(ustring::compare<std::string>(detailMode, "blend_factor_fade", false))
		return msys::DetailMode::BlendFactorFade;
	else if(ustring::compare<std::string>(detailMode, "translucent_base", false))
		return msys::DetailMode::TranslucentBase;
	else if(ustring::compare<std::string>(detailMode, "unlit_additive", false))
		return msys::DetailMode::UnlitAdditive;
	else if(ustring::compare<std::string>(detailMode, "unlit_additive_threshold_fade", false))
		return msys::DetailMode::UnlitAdditiveThresholdFade;
	else if(ustring::compare<std::string>(detailMode, "two_pattern_decal_modulate", false))
		return msys::DetailMode::TwoPatternDecalModulate;
	else if(ustring::compare<std::string>(detailMode, "multiply", false))
		return msys::DetailMode::Multiply;
	else if(ustring::compare<std::string>(detailMode, "base_mask_via_detail_alpha", false))
		return msys::DetailMode::BaseMaskViaDetailAlpha;
	else if(ustring::compare<std::string>(detailMode, "self_shadowed_bumpmap", false))
		return msys::DetailMode::SelfShadowedBumpmap;
	else if(ustring::compare<std::string>(detailMode, "ssbump_albedo", false))
		return msys::DetailMode::SSBumpAlbedo;
	return msys::DetailMode::Invalid;
}
