/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2021 Silverlan
 */

#include "matsysdefinitions.h"
#include "source_vmt_format_handler.hpp"
#include "material.h"
#include "material_manager2.hpp"
#include "textureinfo.h"
#include "detail_mode.hpp"
#include <sharedutils/util_string.h>
#include <datasystem.h>

#ifndef DISABLE_VMT_SUPPORT
#ifdef ENABLE_VKV_PARSER
#include "util_vmt.hpp"
#include <VKVParser/library.h>
msys::SourceVmtFormatHandler2::SourceVmtFormatHandler2(util::IAssetManager &assetManager) : util::IImportAssetFormatHandler {assetManager} {}
bool msys::SourceVmtFormatHandler2::Import(const std::string &outputPath, std::string &outFilePath)
{
	auto size = m_file->GetSize();
	if(size == 0)
		return false;
	std::string data;
	data.resize(size);
	size = m_file->Read(data.data(), size);
	if(size == 0)
		return false;
	data.resize(size);

	ValveKeyValueFormat::setLogCallback([](const std::string &message, ValveKeyValueFormat::LogLevel severity) { std::cout << message << std::endl; });
	auto kvNode = ValveKeyValueFormat::parseKVBuffer(data);
	if(!kvNode) {
		m_error = "TODO";
		return false;
	}
	return LoadVMT(*kvNode, outputPath, outFilePath);
}

// Find highest dx node version and merge its values with the specified node
static void merge_dx_node_values(ValveKeyValueFormat::KVBranch &node)
{
#if 0
	enum class DXVersion : uint8_t
	{
		Undefined = 0,
		dx90,
		dx90_20b // dxlevel 95
	};
	const std::unordered_map<std::string,DXVersion> dxStringToEnum = {
		{"dx90",DXVersion::dx90},
		{"dx90_20b",DXVersion::dx90_20b}
	};
	enum class Operator : int8_t
	{
		None = -1,
		LessThan = 0,
		LessThanOrEqual,
		GreaterThanOrEqual,
		GreaterThan
	}; // Operators ordered by significance!
	const std::array<std::string,4> acceptedOperators = {"<","<=",">=",">"};
	auto numNodes = node.branches.size();
	KVBranch *dxNode = nullptr;
	auto bestDxVersion = DXVersion::Undefined;
	auto bestOperator = Operator::None;
	for(auto i=decltype(numNodes){0u};i<numNodes;++i)
	{
		auto *pNode = node.Get
		auto *name = pNode->GetName();
		std::string dxLevelValue {};
		auto op = Operator::None;
		for(auto opIdx : {2,3,1,0}) // Order is important! (Ordered by string length per operator type (e.g. '<=' has to come before '<'))
		{
			auto &candidate = acceptedOperators.at(opIdx);
			if(ustring::compare(name,candidate.c_str(),true,candidate.length()) == false)
				continue;
			op = static_cast<Operator>(opIdx);
		}
		if(op == Operator::None)
			dxLevelValue = name;
		else
			dxLevelValue = ustring::substr(std::string{name},acceptedOperators.at(umath::to_integral(op)).length());
		ustring::to_lower(dxLevelValue);
		auto it = dxStringToEnum.find(dxLevelValue);
		if(it == dxStringToEnum.end())
			continue;
		if(umath::to_integral(it->second) <= umath::to_integral(bestDxVersion) && umath::to_integral(op) <= umath::to_integral(bestOperator))
			continue;
		dxNode = pNode;
		bestDxVersion = it->second;
		bestOperator = op;
	}
	if(dxNode != nullptr && dxNode->GetType() == VMTNodeType::NODE_TYPE_GROUP)
	{
		auto *dxGroupNode = static_cast<VTFLib::Nodes::CVMTGroupNode*>(dxNode);
		auto numNodesDx = dxGroupNode->GetNodeCount();
		for(auto i=decltype(numNodesDx){0u};i<numNodesDx;++i)
			node.AddNode(dxGroupNode->GetNode(i)->Clone());
	}
#endif
}

static std::vector<float> get_vmt_matrix(std::string value, bool *optOutIsMatrix = nullptr)
{
	if(optOutIsMatrix)
		*optOutIsMatrix = false;
	if(value.front() == '[') {
		value = value.substr(1);
		if(optOutIsMatrix)
			*optOutIsMatrix = true;
	}
	if(value.back() == ']')
		value = value.substr(0, value.length() - 1);
	std::vector<std::string> substrings {};
	ustring::explode_whitespace(value, substrings);

	std::vector<float> data;
	data.reserve(substrings.size());
	for(auto &str : substrings)
		data.push_back(util::to_float(str));
	return data;
}

std::optional<std::string> msys::SourceVmtFormatHandler2::GetStringValue(ValveKeyValueFormat::KVBranch &node, std::string key)
{
	for(auto &c : key)
		c = std::tolower(c);
	auto it = node.branches.find(key);
	if(it == node.branches.end())
		return {};
	auto *leaf = it->second->as_leaf();
	if(!leaf)
		return {};
	return std::string {leaf->value};
}

bool msys::SourceVmtFormatHandler2::LoadVMT(ValveKeyValueFormat::KVNode &vmt, const std::string &outputPath, std::string &outFilePath)
{
	ValveKeyValueFormat::KVBranch *vmtRoot = nullptr; // TODO
	std::string shader {vmtRoot->key};
	ustring::to_lower(shader);

	merge_dx_node_values(*vmtRoot);
	auto phongOverride = std::numeric_limits<float>::quiet_NaN();
	auto bWater = false;
	std::string shaderName;
	auto dataSettings = ds::create_data_settings({});
	auto root = std::make_shared<ds::Block>(*dataSettings);
	if(shader == "worldvertextransition")
		shaderName = "pbr";
	else if(shader == "sprite")
		shaderName = "particle";
	else if(shader == "water") {
		bWater = true;
		shaderName = "water";
		auto hasDudv = false;
		auto bumpMap = GetStringValue(*vmtRoot, "$bumpmap");
		if(bumpMap) {
			root->AddData(Material::DUDV_MAP_IDENTIFIER, std::make_shared<ds::Texture>(*dataSettings, *bumpMap));
			hasDudv = true;
		}
		if(!hasDudv) {
			std::string defaultDudvMap = "nature/water_coast01_dudv"; // Should be shipped with SFM or HL2
			root->AddData(Material::DUDV_MAP_IDENTIFIER, std::make_shared<ds::Texture>(*dataSettings, defaultDudvMap));
		}
		auto normalMap = GetStringValue(*vmtRoot, "$normalmap");
		if(normalMap)
			root->AddData(Material::NORMAL_MAP_IDENTIFIER, std::make_shared<ds::Texture>(*dataSettings, *normalMap));

		auto fog = root->AddBlock("fog");
		auto vmtFogEnable = GetStringValue(*vmtRoot, "$fogenable");
		if(vmtFogEnable) {
			auto fogEnabled = true;
			vmt_parameter_to_numeric_type<bool>(*vmtFogEnable, fogEnabled);
			fog->AddValue("bool", "enabled", fogEnabled ? "1" : "0");
		}

		auto fInchesToMeters = [](float inches) { return inches * 0.0254f; };
		auto vmtFogStart = GetStringValue(*vmtRoot, "$fogstart");
		if(vmtFogStart) {
			auto fogStart = 0.f;
			vmt_parameter_to_numeric_type<float>(*vmtFogStart, fogStart);
			fog->AddValue("float", "start", std::to_string(fInchesToMeters(fogStart)));
		}

		auto vmtFogEnd = GetStringValue(*vmtRoot, "$fogend");
		if(vmtFogEnd) {
			auto fogEnd = 0.f;
			vmt_parameter_to_numeric_type<float>(*vmtFogEnd, fogEnd);
			fog->AddValue("float", "end", std::to_string(fInchesToMeters(fogEnd)));
		}

		auto vmtFogColor = GetStringValue(*vmtRoot, "$fogcolor");
		if(vmtFogColor) {
			auto vColor = vmt_parameter_to_color(*vmtFogColor);
			if(vColor.has_value()) {
				Color color {*vColor};
				fog->AddValue("color", "color", std::to_string(color.r) + ' ' + std::to_string(color.g) + ' ' + std::to_string(color.b));
			}
		}
	}
	else if(shader == "teeth") {
		shaderName = "pbr";
		phongOverride = 1.f; // Hack
	}
	else if(shader == "unlitgeneric")
		shaderName = "unlit";
	else //if(shader == "LightmappedGeneric")
		shaderName = "pbr";

	auto isParticleShader = (shaderName == "particle");
	if(isParticleShader == false) {
		// If $vertexalpha parameter is set, it's probably a particle material?
		auto vmtVertexAlpha = GetStringValue(*vmtRoot, "$vertexalpha");
		if(vmtVertexAlpha)
			vmt_parameter_to_numeric_type<bool>(*vmtVertexAlpha, isParticleShader);
	}

	auto vmtAdditive = GetStringValue(*vmtRoot, "$additive");
	if(vmtAdditive) {
		auto additive = false;
		if(vmt_parameter_to_numeric_type<bool>(*vmtAdditive, additive) && additive)
			root->AddValue("bool", "additive", "1");
		if(additive && isParticleShader) {
			// Additive needs lower color values to match Source more closely
			// (Example: "Explosion_Flashup" particle system with "effects/softglow" material)
			root->AddValue("vector4", "color_factor", "0.1 0.1 0.1 1.0");
			root->AddValue("vector4", "bloom_color_factor", "0 0 0 1");
		}
		else {
			// Additive isn't supported right now, so we'll just hide
			// the material for the time being
			root->AddValue("float", "alpha_factor", "0");
		}
	}

	auto bHasGlowMap = false;
	auto bHasGlow = false;
	auto vmtSelfIllumMask = GetStringValue(*vmtRoot, "$selfillummask");
	if(vmtSelfIllumMask) {
		root->AddData(Material::GLOW_MAP_IDENTIFIER, std::make_shared<ds::Texture>(*dataSettings, *vmtSelfIllumMask));
		bHasGlowMap = true;
		bHasGlow = true;
	}
	auto hasDiffuseMap = false;
	// Prefer HDR textures over LDR
	auto vmtHdrCompressedTexture = GetStringValue(*vmtRoot, "$hdrcompressedTexture");
	if(vmtHdrCompressedTexture) {
		hasDiffuseMap = true;
		root->AddData(Material::ALBEDO_MAP_IDENTIFIER, std::make_shared<ds::Texture>(*dataSettings, *vmtHdrCompressedTexture));
	}
	auto vmtHdrBaseTexture = GetStringValue(*vmtRoot, "$hdrbasetexture");
	if(hasDiffuseMap == false && vmtHdrBaseTexture) {
		hasDiffuseMap = true;
		root->AddData(Material::ALBEDO_MAP_IDENTIFIER, std::make_shared<ds::Texture>(*dataSettings, *vmtHdrBaseTexture));
	}
	auto vmtBaseTexture = GetStringValue(*vmtRoot, "$basetexture");
	if(hasDiffuseMap == false && vmtBaseTexture) {
		root->AddData(Material::ALBEDO_MAP_IDENTIFIER, std::make_shared<ds::Texture>(*dataSettings, *vmtBaseTexture));

		auto vmtSelfIllum = GetStringValue(*vmtRoot, "$selfillum");
		if(bHasGlowMap == false && vmtSelfIllum) {
			root->AddData(Material::GLOW_MAP_IDENTIFIER, std::make_shared<ds::Texture>(*dataSettings, *vmtSelfIllum));
			root->AddValue("int", "glow_blend_diffuse_mode", "3");
			root->AddValue("float", "glow_blend_diffuse_scale", "1");
			root->AddValue("bool", "glow_alpha_only", "1");
			bHasGlow = true;
		}
	}

	auto vmtDetail = GetStringValue(*vmtRoot, "$detail");
	if(vmtDetail) {
		auto detailBlendMode = msys::DetailMode::Invalid;
		static_assert(std::is_same_v<std::underlying_type_t<decltype(detailBlendMode)>, uint8_t>);
		auto vmtDetailBlendMode = GetStringValue(*vmtRoot, "$detailblendmode");
		if(vmtDetailBlendMode)
			vmt_parameter_to_numeric_type<uint8_t>(*vmtDetailBlendMode, reinterpret_cast<uint8_t &>(detailBlendMode));
		if(umath::to_integral(detailBlendMode) >= 0 && umath::to_integral(detailBlendMode) < umath::to_integral(msys::DetailMode::Count)) {
			root->AddValue("string", "detail_blend_mode", msys::to_string(detailBlendMode));
			root->AddData("detail_map", std::make_shared<ds::Texture>(*dataSettings, *vmtDetail));

			auto vmtDetailScale = GetStringValue(*vmtRoot, "$detailscale");
			if(vmtDetailScale) {
				Vector2 uvScale {4.f, 4.f};
				auto isMatrix = false;
				auto values = get_vmt_matrix(*vmtDetailScale, &isMatrix);
				if(isMatrix) {
					if(values.size() > 0) {
						uvScale[0] = values.at(0);
						uvScale[1] = values.at(0);
					}
					if(values.size() > 1)
						uvScale[1] = values.at(1);
				}
				else {
					vmt_parameter_to_numeric_type<float>(*vmtDetailScale, uvScale.x);
					uvScale.y = uvScale.x;
				}
				root->AddValue("vector2", "detail_uv_scale", std::to_string(uvScale[0]) + ' ' + std::to_string(uvScale[1]));
			}

			auto vmtDetailBlendFactor = GetStringValue(*vmtRoot, "$detailblendfactor");
			if(vmtDetailBlendFactor) {
				auto detailBlendFactor = 1.f;
				auto isMatrix = false;
				auto values = get_vmt_matrix(*vmtDetailBlendFactor, &isMatrix);
				if(isMatrix) {
					for(uint8_t i = 0; i < umath::min<uint32_t>(static_cast<uint32_t>(values.size()), static_cast<uint32_t>(1u)); ++i)
						detailBlendFactor = values.at(i);
				}
				else
					vmt_parameter_to_numeric_type<float>(*vmtDetailBlendFactor, detailBlendFactor);
				root->AddValue("float", "detail_factor", std::to_string(detailBlendFactor));
			}

			Vector3 detailColorFactor {1.f, 1.f, 1.f};
			auto vmtDetailTint = GetStringValue(*vmtRoot, "$detailtint");
			if(vmtDetailTint) {
				auto color = vmt_parameter_to_color(*vmtDetailTint);
				if(color.has_value())
					detailColorFactor = *color;
			}
			root->AddValue("vector", "detail_color_factor", std::to_string(detailColorFactor[0]) + ' ' + std::to_string(detailColorFactor[1]) + ' ' + std::to_string(detailColorFactor[2]));
		}
	}

	// These are custom parameters; Used to make it easier to import PBR assets into Pragma
	auto vmtRmaTexture = GetStringValue(*vmtRoot, "$rmatexture");
	if(vmtRmaTexture)
		root->AddData(Material::RMA_MAP_IDENTIFIER, std::make_shared<ds::Texture>(*dataSettings, *vmtRmaTexture));

	auto vmtEmissionTexture = GetStringValue(*vmtRoot, "$emissiontexture");
	if(vmtEmissionTexture)
		root->AddData(Material::EMISSION_MAP_IDENTIFIER, std::make_shared<ds::Texture>(*dataSettings, *vmtEmissionTexture));

	auto vmtMetalnessFactor = GetStringValue(*vmtRoot, "$metalnessfactor");
	if(vmtMetalnessFactor) {
		float factor = 0.f;
		if(vmt_parameter_to_numeric_type<float>(*vmtMetalnessFactor, factor))
			root->AddData("metalness_factor", std::make_shared<ds::Float>(*dataSettings, std::to_string(factor)));
	}
	auto vmtRoughnessFactor = GetStringValue(*vmtRoot, "$roughnessfactor");
	if(vmtRoughnessFactor) {
		float factor = 0.f;
		if(vmt_parameter_to_numeric_type<float>(*vmtRoughnessFactor, factor))
			root->AddData("roughness_factor", std::make_shared<ds::Float>(*dataSettings, std::to_string(factor)));
	}
	auto vmtSpecularFactor = GetStringValue(*vmtRoot, "$specularfactor");
	if(vmtSpecularFactor) {
		float factor = 0.f;
		if(vmt_parameter_to_numeric_type<float>(*vmtSpecularFactor, factor))
			root->AddData("specular_factor", std::make_shared<ds::Float>(*dataSettings, std::to_string(factor)));
	}
	auto vmtEmissionFactor = GetStringValue(*vmtRoot, "$emissionfactor");
	if(vmtEmissionFactor) {
		float factor = 0.f;
		if(vmt_parameter_to_numeric_type<float>(*vmtEmissionFactor, factor))
			root->AddData("emission_factor", std::make_shared<ds::Float>(*dataSettings, std::to_string(factor)));
	}

	if(bHasGlow) {
		auto vmtSelfIllumFresnelMinMaxExp = GetStringValue(*vmtRoot, "$selfillumfresnelminmaxexp");
		if(vmtSelfIllumFresnelMinMaxExp) {
			auto values = get_vmt_matrix(*vmtSelfIllumFresnelMinMaxExp);
			if(values.at(2) == 0.f) // TODO: Not entirely sure this is sensible
			{
				root->AddValue("int", "glow_blend_diffuse_mode", "4");
				root->AddValue("float", "glow_blend_diffuse_scale", "1");
			}
		}
	}
	auto vmtBaseTex2 = GetStringValue(*vmtRoot, "$basetexture2");
	if(shader == "worldvertextransition" && vmtBaseTex2) {
		shaderName = "pbr_blend";
		root->AddData(Material::ALBEDO_MAP2_IDENTIFIER, std::make_shared<ds::Texture>(*dataSettings, *vmtBaseTex2));
	}
	auto vmtBumpMap = GetStringValue(*vmtRoot, "$bumpmap");
	if(bWater == false && vmtBumpMap)
		root->AddData(Material::NORMAL_MAP_IDENTIFIER, std::make_shared<ds::Texture>(*dataSettings, *vmtBumpMap));
	auto vmtEnvMapMask = GetStringValue(*vmtRoot, "$envmapmask");
	if(vmtEnvMapMask)
		root->AddData("specular_map", std::make_shared<ds::Texture>(*dataSettings, *vmtEnvMapMask));
	auto vmtAdditive2 = GetStringValue(*vmtRoot, "$additive");
	if(vmtAdditive2) {
		root->AddValue("int", "alpha_mode", std::to_string(umath::to_integral(AlphaMode::Blend)));
		get_vmt_data<ds::Bool, int32_t>(root, *dataSettings, "black_to_alpha", *vmtAdditive2);
	}
	auto vmtPhong = GetStringValue(*vmtRoot, "$phong");
	if(vmtPhong)
		get_vmt_data<ds::Bool, int32_t>(root, *dataSettings, "phong_normal_alpha", *vmtPhong);
	auto vmtPhongExponent = GetStringValue(*vmtRoot, "$phongexponent");
	if(vmtPhongExponent) {
		if(std::isnan(phongOverride)) {
			get_vmt_data<ds::Float, float>(root, *dataSettings, "phong_intensity", *vmtPhongExponent, [](float v) -> float { return sqrtf(v); });
		}
		else
			root->AddData("phong_intensity", std::make_shared<ds::Float>(*dataSettings, std::to_string(phongOverride)));
	}
	auto vmtPhongBoost = GetStringValue(*vmtRoot, "$phongboost");
	if(vmtPhongBoost) {
		get_vmt_data<ds::Float, float>(root, *dataSettings, "phong_shininess", *vmtPhongBoost, [](float v) -> float { return v * 2.f; });
	}
	auto vmtParallaxMap = GetStringValue(*vmtRoot, "$parallaxmap");
	if(vmtParallaxMap)
		root->AddData(Material::PARALLAX_MAP_IDENTIFIER, std::make_shared<ds::Texture>(*dataSettings, *vmtParallaxMap));
	auto vmtParallaxMapScale = GetStringValue(*vmtRoot, "$parallaxmapscale");
	if(vmtParallaxMapScale) {
		get_vmt_data<ds::Float, float>(root, *dataSettings, "parallax_height_scale", *vmtParallaxMapScale, [](float v) -> float { return v * 0.025f; });
	}

	auto translucent = false;
	auto vmtTranslucent = GetStringValue(*vmtRoot, "$translucent");
	if(vmtTranslucent && vmt_parameter_to_numeric_type<bool>(*vmtTranslucent, translucent))
		root->AddValue("int", "alpha_mode", std::to_string(umath::to_integral(AlphaMode::Blend)));

	float alphaFactor = 1.f;
	auto vmtAlpha = GetStringValue(*vmtRoot, "$alpha");
	if(vmtAlpha && vmt_parameter_to_numeric_type<float>(*vmtAlpha, alphaFactor))
		root->AddValue("float", "alpha_factor", std::to_string(alphaFactor));

	auto alphaTest = false;
	auto vmtAlphaTest = GetStringValue(*vmtRoot, "$alphatest");
	if(vmtAlphaTest && vmt_parameter_to_numeric_type<bool>(*vmtAlphaTest, alphaTest)) {
		root->AddValue("int", "alpha_mode", std::to_string(umath::to_integral(AlphaMode::Mask)));
		auto alphaCutoff = 0.5f; // TODO: Confirm that the default for Source is 0.5
		auto vmtAlphaTestRef = GetStringValue(*vmtRoot, "$alphatestreference");
		if(vmtAlphaTestRef)
			vmt_parameter_to_numeric_type<float>(*vmtAlphaTestRef, alphaCutoff);
		root->AddValue("float", "alpha_cutoff", std::to_string(alphaCutoff));
	}

	auto vmtSurfaceProp = GetStringValue(*vmtRoot, "$surfaceprop");
	if(vmtSurfaceProp) {
		std::string surfaceProp = *vmtSurfaceProp;
		ustring::to_lower(surfaceProp);
		std::string surfaceMaterial = "concrete";
		std::unordered_map<std::string, std::string> translateMaterial = {
#include "impl_surfacematerials.h"
		};
		auto it = translateMaterial.find(surfaceProp);
		if(it != translateMaterial.end())
			surfaceMaterial = it->second;
		root->AddData("surfacematerial", std::make_shared<ds::String>(*dataSettings, surfaceMaterial));
	}
	auto vmtCompress = GetStringValue(*vmtRoot, "$compress");
	if(vmtCompress)
		root->AddData(Material::WRINKLE_COMPRESS_MAP_IDENTIFIER, std::make_shared<ds::Texture>(*dataSettings, *vmtCompress));

	auto vmtStretch = GetStringValue(*vmtRoot, "$stretch");
	if(vmtStretch)
		root->AddData(Material::WRINKLE_STRETCH_MAP_IDENTIFIER, std::make_shared<ds::Texture>(*dataSettings, *vmtStretch));
	auto vmtPhonExponentTexture = GetStringValue(*vmtRoot, "$phongexponenttexture");
	if(vmtPhonExponentTexture)
		root->AddData(Material::EXPONENT_MAP_IDENTIFIER, std::make_shared<ds::Texture>(*dataSettings, *vmtPhonExponentTexture));

	auto vmtFakePbrLayer = GetStringValue(*vmtRoot, "$fakepbr_layer");
	if(vmtFakePbrLayer) {
		int32_t fakePbrLayer = 0;
		if(vmt_parameter_to_numeric_type<int32_t>(*vmtFakePbrLayer, fakePbrLayer))
			shaderName = "nodraw"; // Fake PBR uses multiple mesh layers; We'll just hide the additional ones
	}

	auto vmtNoDraw = GetStringValue(*vmtRoot, "$no_draw");
	if(vmtNoDraw) {
		int32_t noDraw = 0;
		if(vmt_parameter_to_numeric_type<int32_t>(*vmtNoDraw, noDraw))
			shaderName = "nodraw";
	}

	auto vmtAmbientOcclTexture = GetStringValue(*vmtRoot, "$ambientoccltexture");
	if(vmtAmbientOcclTexture)
		root->AddData("ao_map", std::make_shared<ds::Texture>(*dataSettings, *vmtAmbientOcclTexture));

	if(shaderName == "pbr" || shaderName == "pbr_blend") {
		if(root->HasValue(Material::RMA_MAP_IDENTIFIER) == false) {
			auto rmaInfo = root->AddBlock("rma_info");
			rmaInfo->AddValue("bool", "requires_metalness_update", "1");
			rmaInfo->AddValue("bool", "requires_roughness_update", "1");
			if(root->HasValue("ao_map") == false)
				rmaInfo->AddValue("bool", "requires_ao_update", "1");
		}
	}

	if(isParticleShader == false) {
		auto vmtColor = GetStringValue(*vmtRoot, "$color");
		auto vmtColor2 = GetStringValue(*vmtRoot, "$color2");
		if(vmtColor || vmtColor2) {
			auto color = vmt_parameter_to_color(vmtColor ? *vmtColor : *vmtColor2);
			if(color.has_value()) {
				root->AddValue("vector4", "color_factor", std::to_string(color->r) + ' ' + std::to_string(color->g) + ' ' + std::to_string(color->b) + " 1.0");
				if(root->HasValue(Material::ALBEDO_MAP_IDENTIFIER) == false) // $color / $color2 attributes work without a diffuse texture
					root->AddData(Material::ALBEDO_MAP_IDENTIFIER, std::make_shared<ds::Texture>(*dataSettings, "white"));
			}
		}
	}

	if(!LoadVMTData(vmt, shader, *root, shaderName))
		return false;
	auto mat = Material::Create(static_cast<msys::MaterialManager &>(GetAssetManager()), shaderName, root);
	if(!mat)
		return false;
	std::string err;
	outFilePath = outputPath + ".pmat";
	if(!mat->Save(outFilePath, err, true)) {
		m_error = std::move(err);
		return false;
	}
	return true;
}
bool msys::SourceVmtFormatHandler2::LoadVMTData(ValveKeyValueFormat::KVNode &node, const std::string &vmtShader, ds::Block &rootData, std::string &matShader) { return true; }
#endif
#endif
