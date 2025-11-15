// SPDX-FileCopyrightText: (c) 2021 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#ifndef DISABLE_VMT_SUPPORT
#include <VMTFile.h>
#include <VTFLib.h>
#endif

module pragma.materialsystem;

import :format_handlers.source_vmt;
import :vmt;

#ifndef DISABLE_VMT_SUPPORT
msys::ISourceVmtFormatHandler::ISourceVmtFormatHandler(util::IAssetManager &assetManager) : util::IImportAssetFormatHandler {assetManager} {}
std::optional<std::string> msys::ISourceVmtFormatHandler::GetStringValue(const std::string &key, const IVmtNode *optParent, const std::optional<std::string> &defaultValue) const
{
	auto node = GetNode(key, optParent);
	if(!node)
		return {};
	auto val = GetStringValue(*node);
	if(!val)
		return defaultValue;
	return val;
}
std::optional<bool> msys::ISourceVmtFormatHandler::GetBooleanValue(const std::string &key, const IVmtNode *optParent, const std::optional<bool> &defaultValue) const
{
	auto node = GetNode(key, optParent);
	if(!node)
		return {};
	auto val = GetBooleanValue(*node);
	if(!val)
		return defaultValue;
	return val;
}
std::optional<float> msys::ISourceVmtFormatHandler::GetFloatValue(const std::string &key, const IVmtNode *optParent, const std::optional<float> &defaultValue) const
{
	auto node = GetNode(key, optParent);
	if(!node)
		return {};
	auto val = GetFloatValue(*node);
	if(!val)
		return defaultValue;
	return val;
}
std::optional<Vector3> msys::ISourceVmtFormatHandler::GetColorValue(const std::string &key, const IVmtNode *optParent, const std::optional<Vector3> &defaultValue) const
{
	auto node = GetNode(key, optParent);
	if(!node)
		return {};
	auto val = GetColorValue(*node);
	if(!val)
		return defaultValue;
	return val;
}
std::optional<uint8_t> msys::ISourceVmtFormatHandler::GetUint8Value(const std::string &key, const IVmtNode *optParent, const std::optional<uint8_t> &defaultValue) const
{
	auto node = GetNode(key, optParent);
	if(!node)
		return {};
	auto val = GetUint8Value(*node);
	if(!val)
		return defaultValue;
	return val;
}
std::optional<int32_t> msys::ISourceVmtFormatHandler::GetInt32Value(const std::string &key, const IVmtNode *optParent, const std::optional<uint8_t> &defaultValue) const
{
	auto node = GetNode(key, optParent);
	if(!node)
		return {};
	auto val = GetInt32Value(*node);
	if(!val)
		return defaultValue;
	return val;
}
bool msys::ISourceVmtFormatHandler::AssignStringValue(ds::Block &dsData, const IVmtNode &vmtNode, const std::string &vmtKey, const std::string &pragmaKey) const
{
	auto node = GetNode(vmtKey, &vmtNode);
	if(!node)
		return false;
	auto value = GetStringValue(*node);
	if(!value)
		return false;
	dsData.AddValue("string", pragmaKey, *value);
	return true;
}
bool msys::ISourceVmtFormatHandler::AssignTextureValue(ds::Block &dsData, const IVmtNode &vmtNode, const std::string &vmtKey, const std::string &pragmaKey) const
{
	auto node = GetNode(vmtKey, &vmtNode);
	if(!node)
		return false;
	auto value = GetStringValue(*node);
	if(!value)
		return false;
	dsData.AddValue("texture", pragmaKey, *value);
	return true;
}
bool msys::ISourceVmtFormatHandler::AssignBooleanValue(ds::Block &dsData, const IVmtNode &vmtNode, const std::string &vmtKey, const std::string &pragmaKey, std::optional<bool> defaultValue) const
{
	auto node = GetNode(vmtKey, &vmtNode);
	if(!node)
		return false;
	auto value = GetBooleanValue(*node);
	if(!value)
		value = defaultValue;
	if(!value)
		return false;
	dsData.AddValue("bool", pragmaKey, *value ? "1" : "0");
	return true;
}
bool msys::ISourceVmtFormatHandler::AssignFloatValue(ds::Block &dsData, const IVmtNode &vmtNode, const std::string &vmtKey, const std::string &pragmaKey, std::optional<float> defaultValue) const
{
	auto node = GetNode(vmtKey, &vmtNode);
	if(!node)
		return false;
	auto value = GetFloatValue(*node);
	if(!value)
		value = defaultValue;
	if(!value)
		return false;
	dsData.AddValue("float", pragmaKey, std::to_string(*value));
	return true;
}
bool msys::ISourceVmtFormatHandler::LoadVMT(const IVmtNode &rootNode, const std::string &outputPath, std::string &outFilePath)
{
	std::string shader = GetShader();
	ustring::to_lower(shader);

	auto phongOverride = std::numeric_limits<float>::quiet_NaN();
	auto bWater = false;
	std::string shaderName;
	std::shared_ptr<const IVmtNode> node = nullptr;
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
		if((node = GetNode("$bumpmap", &rootNode)) != nullptr) {
			auto bumpMap = GetStringValue(*node);
			if(bumpMap) {
				root->AddData(material::DUDV_MAP_IDENTIFIER, std::make_shared<ds::Texture>(*dataSettings, *bumpMap));
				hasDudv = true;
			}
		}
		if(!hasDudv) {
			std::string defaultDudvMap = "nature/water_coast01_dudv"; // Should be shipped with SFM or HL2
			root->AddData(material::DUDV_MAP_IDENTIFIER, std::make_shared<ds::Texture>(*dataSettings, defaultDudvMap));
		}
		AssignTextureValue(*root, *m_rootNode, "$normalmap", material::NORMAL_MAP_IDENTIFIER);

		auto fog = root->AddBlock("fog");
		AssignBooleanValue(*fog, rootNode, "$fogenable", "enabled", true);

		auto fInchesToMeters = [](float inches) { return inches * 0.0254f; };
		auto fogStart = GetFloatValue("$fogstart", nullptr, 0.f);
		if(fogStart)
			fog->AddValue("float", "start", std::to_string(fInchesToMeters(*fogStart)));

		auto fogEnd = GetFloatValue("$fogend", nullptr, 0.f);
		if(fogEnd)
			fog->AddValue("float", "end", std::to_string(fInchesToMeters(*fogEnd)));

		auto fogColor = GetColorValue("$fogcolor", nullptr);
		if(fogColor) {
			Color color {*fogColor};
			fog->AddValue("color", "color", std::to_string(color.r) + ' ' + std::to_string(color.g) + ' ' + std::to_string(color.b));
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
		auto val = GetBooleanValue("$vertexalpha", nullptr, false);
		if(val)
			isParticleShader = *val;
	}

	auto additive = GetBooleanValue("$additive", nullptr, false);
	if(additive) {
		if(*additive)
			root->AddValue("bool", "additive", "1");
		if(*additive && isParticleShader) {
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
	if((node = GetNode("$selfillummask", &rootNode)) != nullptr) {
		auto selfIllumMask = GetStringValue(*node);
		if(selfIllumMask) {
			root->AddData(material::GLOW_MAP_IDENTIFIER, std::make_shared<ds::Texture>(*dataSettings, *selfIllumMask));
			bHasGlowMap = true;
			bHasGlow = true;
		}
	}
	auto hasDiffuseMap = false;
	// Prefer HDR textures over LDR
	if((node = GetNode("$hdrcompressedTexture", &rootNode)) != nullptr) {
		auto hdrCompressedTexture = GetStringValue(*node);
		if(hdrCompressedTexture) {
			hasDiffuseMap = true;
			root->AddData(material::ALBEDO_MAP_IDENTIFIER, std::make_shared<ds::Texture>(*dataSettings, *hdrCompressedTexture));
		}
	}
	if(hasDiffuseMap == false && (node = GetNode("$hdrbasetexture", &rootNode)) != nullptr) {
		auto hdrBaseTexture = GetStringValue(*node);
		if(hdrBaseTexture) {
			hasDiffuseMap = true;
			root->AddData(material::ALBEDO_MAP_IDENTIFIER, std::make_shared<ds::Texture>(*dataSettings, *hdrBaseTexture));
		}
	}
	if(hasDiffuseMap == false && (node = GetNode("$basetexture", &rootNode)) != nullptr) {
		auto baseTexture = GetStringValue(*node);
		if(baseTexture) {
			root->AddData(material::ALBEDO_MAP_IDENTIFIER, std::make_shared<ds::Texture>(*dataSettings, *baseTexture));

			if(bHasGlowMap == false && (node = GetNode("$selfillum", &rootNode)) != nullptr) {
				root->AddData(material::GLOW_MAP_IDENTIFIER, std::make_shared<ds::Texture>(*dataSettings, *baseTexture));
				root->AddValue("int", "glow_blend_diffuse_mode", "3");
				root->AddValue("float", "glow_blend_diffuse_scale", "1");
				root->AddValue("bool", "glow_alpha_only", "1");
				bHasGlow = true;
			}
		}
	}

	if((node = GetNode("$detail", &rootNode)) != nullptr) {
		auto detail = GetStringValue(*node);
		if(detail) {
			auto detailBlendMode = msys::DetailMode::Invalid;
			static_assert(std::is_same_v<std::underlying_type_t<decltype(detailBlendMode)>, uint8_t>);
			auto vmtDetailBlendMode = GetUint8Value("$detailblendmode");
			if(vmtDetailBlendMode)
				detailBlendMode = static_cast<msys::DetailMode>(*vmtDetailBlendMode);
			if(umath::to_integral(detailBlendMode) >= 0 && umath::to_integral(detailBlendMode) < umath::to_integral(msys::DetailMode::Count)) {
				root->AddValue("string", "detail_blend_mode", msys::to_string(detailBlendMode));
				root->AddData("detail_map", std::make_shared<ds::Texture>(*dataSettings, *detail));

				node = GetNode("$detailscale", &rootNode);
				if(node) {
					Vector2 uvScale {4.f, 4.f};
					auto values = GetMatrixValue(*node);
					if(values) {
						if(values->size() > 0) {
							uvScale[0] = values->at(0);
							uvScale[1] = values->at(0);
						}
						if(values->size() > 1)
							uvScale[1] = values->at(1);
					}
					else {
						auto val = GetFloatValue(*node);
						if(val)
							uvScale.x = *val;
						uvScale.y = uvScale.x;
					}
					root->AddValue("vector2", "detail_uv_scale", std::to_string(uvScale[0]) + ' ' + std::to_string(uvScale[1]));
				}

				node = GetNode("$detailblendfactor", &rootNode);
				if(node) {
					auto detailBlendFactor = 1.f;
					auto values = GetMatrixValue(*node);
					if(values) {
						for(uint8_t i = 0; i < umath::min<uint32_t>(static_cast<uint32_t>(values->size()), static_cast<uint32_t>(1u)); ++i)
							detailBlendFactor = values->at(i);
					}
					else {
						auto val = GetFloatValue(*node);
						if(val)
							detailBlendFactor = *val;
					}
					root->AddValue("float", "detail_factor", std::to_string(detailBlendFactor));
				}

				Vector3 detailColorFactor {1.f, 1.f, 1.f};
				auto vmtDetailTint = GetColorValue("$detailtint");
				if(vmtDetailTint)
					detailColorFactor = *vmtDetailTint;
				root->AddValue("vector", "detail_color_factor", std::to_string(detailColorFactor[0]) + ' ' + std::to_string(detailColorFactor[1]) + ' ' + std::to_string(detailColorFactor[2]));
			}
		}
	}

	// These are custom parameters; Used to make it easier to import PBR assets into Pragma
	AssignTextureValue(*root, *m_rootNode, "$rmatexture", material::RMA_MAP_IDENTIFIER);
	AssignTextureValue(*root, *m_rootNode, "$emissiontexture", material::EMISSION_MAP_IDENTIFIER);
	AssignFloatValue(*root, *m_rootNode, "$metalnessfactor", "metalness_factor", 0.f);
	AssignFloatValue(*root, *m_rootNode, "$roughnessfactor", "roughness_factor", 0.f);
	AssignFloatValue(*root, *m_rootNode, "$specularfactor", "specular_factor", 0.f);
	AssignFloatValue(*root, *m_rootNode, "$emissionfactor", "emission_factor", 0.f);

	if(bHasGlow) {
		auto nodeFresnel = GetNode("$selfillumfresnelminmaxexp", &rootNode);
		if(nodeFresnel) {
			auto selfIllumFresnelMinMapExp = GetMatrixValue(*nodeFresnel);
			if(selfIllumFresnelMinMapExp) {
				if(selfIllumFresnelMinMapExp->at(2) == 0.f) // TODO: Not entirely sure this is sensible
				{
					root->AddValue("int", "glow_blend_diffuse_mode", "4");
					root->AddValue("float", "glow_blend_diffuse_scale", "1");
				}
			}
		}
	}
	if(shader == "worldvertextransition") {
		if(AssignTextureValue(*root, *m_rootNode, "$basetexture2", material::ALBEDO_MAP2_IDENTIFIER))
			shaderName = "pbr_blend";
	}
	if(bWater == false)
		AssignTextureValue(*root, *m_rootNode, "$bumpmap", material::NORMAL_MAP_IDENTIFIER);
	AssignTextureValue(*root, *m_rootNode, "$envmapmask", "specular_map");
	if((node = GetNode("$additive", &rootNode)) != nullptr) {
		root->AddValue("int", "alpha_mode", std::to_string(umath::to_integral(AlphaMode::Blend)));
		AssignBooleanValue(*root, *m_rootNode, "$additive", "black_to_alpha");
	}
	AssignBooleanValue(*root, *m_rootNode, "$phong", "phong_normal_alpha");
	if((node = GetNode("$phongexponent", &rootNode)) != nullptr) {
		if(std::isnan(phongOverride)) {
			auto val = GetFloatValue(*node);
			if(val)
				root->AddValue("float", "phong_intensity", std::to_string(sqrtf(*val)));
		}
		else
			root->AddValue("float", "phong_intensity", std::to_string(phongOverride));
	}
	if((node = GetNode("$phongboost", &rootNode)) != nullptr) {
		auto val = GetFloatValue(*node);
		if(val)
			root->AddValue("float", "phong_shininess", std::to_string(*val * 2.f));
	}
	AssignTextureValue(*root, *m_rootNode, "$parallaxmap", material::PARALLAX_MAP_IDENTIFIER);
	if((node = GetNode("$parallaxmapscale", &rootNode)) != nullptr) {
		auto val = GetFloatValue(*node);
		if(val)
			root->AddValue("float", "parallax_height_scale", std::to_string(*val * 0.025f));
	}

	auto translucent = GetBooleanValue("$translucent");
	if(translucent && *translucent)
		root->AddValue("int", "alpha_mode", std::to_string(umath::to_integral(AlphaMode::Blend)));

	auto alphaFactor = GetFloatValue("$alpha");
	if(alphaFactor)
		root->AddValue("float", "alpha_factor", std::to_string(*alphaFactor));

	auto alphaTest = GetBooleanValue("$alphatest");
	if(alphaTest) {
		root->AddValue("int", "alpha_mode", std::to_string(umath::to_integral(AlphaMode::Mask)));
		auto alphaCutoff = 0.5f; // TODO: Confirm that the default for Source is 0.5
		auto alphaTestReference = GetFloatValue("$alphatestreference");
		if(alphaTestReference)
			alphaCutoff = *alphaTestReference;
		root->AddValue("float", "alpha_cutoff", std::to_string(alphaCutoff));
	}

	auto surfaceProp = GetStringValue("$surfaceprop");
	if(surfaceProp) {
		ustring::to_lower(*surfaceProp);
		std::string surfaceMaterial = "concrete";
		std::unordered_map<std::string, std::string> translateMaterial = {
#include "impl_surfacematerials.h"

		};
		auto it = translateMaterial.find(*surfaceProp);
		if(it != translateMaterial.end())
			surfaceMaterial = it->second;
		root->AddData("surfacematerial", std::make_shared<ds::String>(*dataSettings, surfaceMaterial));
	}
	AssignTextureValue(*root, *m_rootNode, "$compress", material::WRINKLE_COMPRESS_MAP_IDENTIFIER);
	AssignTextureValue(*root, *m_rootNode, "$stretch", material::WRINKLE_STRETCH_MAP_IDENTIFIER);
	AssignTextureValue(*root, *m_rootNode, "$phongexponenttexture", material::EXPONENT_MAP_IDENTIFIER);

	auto fakePbrLayer = GetInt32Value("$fakepbr_layer");
	if(fakePbrLayer)
		shaderName = "nodraw"; // Fake PBR uses multiple mesh layers; We'll just hide the additional ones

	auto noDraw = GetInt32Value("$no_draw");
	if(noDraw)
		shaderName = "nodraw";

	AssignTextureValue(*root, *m_rootNode, "$ambientoccltexture", "ao_map");

	if(shaderName == "pbr" || shaderName == "pbr_blend") {
		if(root->HasValue(material::RMA_MAP_IDENTIFIER) == false) {
			auto rmaInfo = root->AddBlock("rma_info");
			rmaInfo->AddValue("bool", "requires_metalness_update", "1");
			rmaInfo->AddValue("bool", "requires_roughness_update", "1");
			if(root->HasValue("ao_map") == false)
				rmaInfo->AddValue("bool", "requires_ao_update", "1");
		}
	}

	if(isParticleShader == false) {
		auto color = GetColorValue("$color");
		if(!color)
			color = GetColorValue("$color2");
		if(color) {
			root->AddValue("vector4", "color_factor", std::to_string(color->r) + ' ' + std::to_string(color->g) + ' ' + std::to_string(color->b) + " 1.0");
			if(root->HasValue(material::ALBEDO_MAP_IDENTIFIER) == false) // $color / $color2 attributes work without a diffuse texture
				root->AddData(material::ALBEDO_MAP_IDENTIFIER, std::make_shared<ds::Texture>(*dataSettings, "white"));
		}
	}

	if(!LoadVmtData(shader, *root, shaderName))
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

/////////////////////////

msys::SourceVmtFormatHandler::SourceVmtFormatHandler(util::IAssetManager &assetManager) : ISourceVmtFormatHandler {assetManager} {}
const VTFLib::Nodes::CVMTNode &msys::SourceVmtFormatHandler::GetVtfLibNode(const IVmtNode &vmtNode) const { return static_cast<const VtfLibVmtNode &>(vmtNode).vtfLibNode; }
std::string msys::SourceVmtFormatHandler::GetShader() const
{
	auto &vtfLibNode = GetVtfLibNode(*m_rootNode);
	return vtfLibNode.GetName();
}
std::shared_ptr<const msys::IVmtNode> msys::SourceVmtFormatHandler::GetNode(const std::string &key, const IVmtNode *optParent) const
{
	if(!optParent)
		return GetNode(key, m_rootNode.get());
	auto &vtfLibParent = GetVtfLibNode(*optParent);
	if(vtfLibParent.GetType() != VMTNodeType::NODE_TYPE_GROUP)
		return nullptr;
	auto *child = static_cast<const VTFLib::Nodes::CVMTGroupNode &>(vtfLibParent).GetNode(key.c_str());
	if(!child)
		return nullptr;
	return std::make_shared<VtfLibVmtNode>(*child);
}
std::optional<std::string> msys::SourceVmtFormatHandler::GetStringValue(const IVmtNode &node) const
{
	auto &vtfLibNode = GetVtfLibNode(node);
	if(vtfLibNode.GetType() != VMTNodeType::NODE_TYPE_STRING)
		return {};
	auto &strNode = static_cast<const VTFLib::Nodes::CVMTStringNode &>(vtfLibNode);
	return strNode.GetValue();
}
std::optional<float> msys::SourceVmtFormatHandler::GetFloatValue(const IVmtNode &node) const
{
	auto &vtfLibNode = GetVtfLibNode(node);
	float value;
	if(!vmt_parameter_to_numeric_type<float>(&vtfLibNode, value))
		return {};
	return value;
}
std::optional<bool> msys::SourceVmtFormatHandler::GetBooleanValue(const IVmtNode &node) const
{
	auto &vtfLibNode = GetVtfLibNode(node);
	bool value;
	if(!vmt_parameter_to_numeric_type<bool>(&vtfLibNode, value))
		return {};
	return value;
}
std::optional<Vector3> msys::SourceVmtFormatHandler::GetColorValue(const IVmtNode &node) const
{
	auto &vtfLibNode = GetVtfLibNode(node);
	return vmt_parameter_to_color(vtfLibNode);
}
std::optional<uint8_t> msys::SourceVmtFormatHandler::GetUint8Value(const IVmtNode &node) const
{
	auto &vtfLibNode = GetVtfLibNode(node);
	uint8_t value;
	if(!vmt_parameter_to_numeric_type<uint8_t>(&vtfLibNode, value))
		return {};
	return value;
}
std::optional<int32_t> msys::SourceVmtFormatHandler::GetInt32Value(const IVmtNode &node) const
{
	auto &vtfLibNode = GetVtfLibNode(node);
	int32_t value;
	if(!vmt_parameter_to_numeric_type<int32_t>(&vtfLibNode, value))
		return {};
	return value;
}
std::optional<std::array<float, 3>> msys::SourceVmtFormatHandler::GetMatrixValue(const IVmtNode &node) const
{
	auto &vtfLibNode = GetVtfLibNode(node);
	auto str = GetStringValue(node);
	if(!str)
		return {};
	return get_vmt_matrix(*str);
}

// Find highest dx node version and merge its values with the specified node
static void merge_dx_node_values(VTFLib::Nodes::CVMTGroupNode &node)
{
	enum class DXVersion : uint8_t {
		Undefined = 0,
		dx90,
		dx90_20b // dxlevel 95
	};
	const std::unordered_map<std::string, DXVersion> dxStringToEnum = {{"dx90", DXVersion::dx90}, {"dx90_20b", DXVersion::dx90_20b}};
	enum class Operator : int8_t {
		None = -1,
		LessThan = 0,
		LessThanOrEqual,
		GreaterThanOrEqual,
		GreaterThan,
	}; // Operators ordered by significance!
	const std::array<std::string, 4> acceptedOperators = {"<", "<=", ">=", ">"};
	auto numNodes = node.GetNodeCount();
	VTFLib::Nodes::CVMTNode *dxNode = nullptr;
	auto bestDxVersion = DXVersion::Undefined;
	auto bestOperator = Operator::None;
	for(auto i = decltype(numNodes) {0u}; i < numNodes; ++i) {
		auto *pNode = node.GetNode(i);
		auto *name = pNode->GetName();
		std::string dxLevelValue {};
		auto op = Operator::None;
		for(auto opIdx : {2, 3, 1, 0}) // Order is important! (Ordered by string length per operator type (e.g. '<=' has to come before '<'))
		{
			auto &candidate = acceptedOperators.at(opIdx);
			if(ustring::compare(name, candidate.c_str(), true, candidate.length()) == false)
				continue;
			op = static_cast<Operator>(opIdx);
		}
		if(op == Operator::None)
			dxLevelValue = name;
		else
			dxLevelValue = ustring::substr(std::string {name}, acceptedOperators.at(umath::to_integral(op)).length());
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
	if(dxNode != nullptr && dxNode->GetType() == VMTNodeType::NODE_TYPE_GROUP) {
		auto *dxGroupNode = static_cast<VTFLib::Nodes::CVMTGroupNode *>(dxNode);
		auto numNodesDx = dxGroupNode->GetNodeCount();
		for(auto i = decltype(numNodesDx) {0u}; i < numNodesDx; ++i)
			node.AddNode(dxGroupNode->GetNode(i)->Clone());
	}
}

bool msys::SourceVmtFormatHandler::Import(const std::string &outputPath, std::string &outFilePath)
{
	auto size = m_file->GetSize();
	if(size == 0)
		return false;
	std::vector<uint8_t> data;
	data.resize(size);
	size = m_file->Read(data.data(), size);
	if(size == 0)
		return false;
	data.resize(size);

	VTFLib::CVMTFile vmt {};
	if(vmt.Load(data.data(), static_cast<vlUInt>(size)) != vlTrue) {
		m_error = "VMT Parsing error in material: " + std::string {vlGetLastError()};
		return false;
	}
	auto *vmtRoot = vmt.GetRoot();
	merge_dx_node_values(*vmtRoot);
	m_rootNode = std::make_shared<VtfLibVmtNode>(*vmtRoot);
	return LoadVMT(*m_rootNode, outputPath, outFilePath);
}
#endif
