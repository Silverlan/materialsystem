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
#include <VMTFile.h>
#include <VTFLib.h>
#include "util_vmt.hpp"

msys::SourceVmtFormatHandler::SourceVmtFormatHandler(util::IAssetManager &assetManager)
	: util::IImportAssetFormatHandler{assetManager}
{}
bool msys::SourceVmtFormatHandler::Import(const std::string &outputPath,std::string &outFilePath)
{
	auto size = m_file->GetSize();
	if(size == 0)
		return false;
	std::vector<uint8_t> data;
	data.resize(size);
	size = m_file->Read(data.data(),size);
	if(size == 0)
		return false;
	data.resize(size);

	VTFLib::CVMTFile vmt {};
	if(vmt.Load(data.data(),static_cast<vlUInt>(size)) != vlTrue)
	{
		m_error = "VMT Parsing error in material: " +std::string{vlGetLastError()};
		return false;
	}
	return LoadVMT(vmt,outputPath,outFilePath);
}

// Find highest dx node version and merge its values with the specified node
static void merge_dx_node_values(VTFLib::Nodes::CVMTGroupNode &node)
{
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
	auto numNodes = node.GetNodeCount();
	VTFLib::Nodes::CVMTNode *dxNode = nullptr;
	auto bestDxVersion = DXVersion::Undefined;
	auto bestOperator = Operator::None;
	for(auto i=decltype(numNodes){0u};i<numNodes;++i)
	{
		auto *pNode = node.GetNode(i);
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
}

static std::array<float,3> get_vmt_matrix(VTFLib::Nodes::CVMTStringNode &node)
{
	std::string value = node.GetValue();
	if(value.front() == '[')
		value = value.substr(1);
	if(value.back() == ']')
		value = value.substr(0,value.length() -1);
	std::vector<std::string> substrings {};
	ustring::explode_whitespace(value,substrings);

	std::array<float,3> data {0.f,0.f,0.f};
	for(uint8_t i=0;i<umath::min(substrings.size(),static_cast<size_t>(3));++i)
	{
		auto val = util::to_float(substrings.at(i));
		for(auto j=i;j<3;++j)
			data[j] = val;
	}
	return data;
}

bool msys::SourceVmtFormatHandler::LoadVMT(VTFLib::CVMTFile &vmt,const std::string &outputPath,std::string &outFilePath)
{
	auto *vmtRoot = vmt.GetRoot();
	std::string shader = vmtRoot->GetName();
	ustring::to_lower(shader);

	merge_dx_node_values(*vmtRoot);
	auto phongOverride = std::numeric_limits<float>::quiet_NaN();
	auto bWater = false;
	std::string shaderName;
	VTFLib::Nodes::CVMTNode *node = nullptr;
	auto dataSettings = ds::create_data_settings({});
	auto root = std::make_shared<ds::Block>(*dataSettings);
	if(shader == "worldvertextransition")
		shaderName = "pbr";
	else if(shader == "sprite")
		shaderName = "particle";
	else if(shader == "water")
	{
		bWater = true;
		shaderName = "water";
		auto hasDudv = false;
		if((node = vmtRoot->GetNode("$bumpmap")) != nullptr)
		{
			if(node->GetType() == VMTNodeType::NODE_TYPE_STRING)
			{
				auto *bumpMapNode = static_cast<VTFLib::Nodes::CVMTStringNode*>(node);
				root->AddData(Material::DUDV_MAP_IDENTIFIER,std::make_shared<ds::Texture>(*dataSettings,bumpMapNode->GetValue()));
				hasDudv = true;
			}
		}
		if(!hasDudv)
		{
			std::string defaultDudvMap = "nature/water_coast01_dudv"; // Should be shipped with SFM or HL2
			root->AddData(Material::DUDV_MAP_IDENTIFIER,std::make_shared<ds::Texture>(*dataSettings,defaultDudvMap));
		}
		if((node = vmtRoot->GetNode("$normalmap")) != nullptr)
		{
			if(node->GetType() == VMTNodeType::NODE_TYPE_STRING)
			{
				auto *normalMapNode = static_cast<VTFLib::Nodes::CVMTStringNode*>(node);
				root->AddData(Material::NORMAL_MAP_IDENTIFIER,std::make_shared<ds::Texture>(*dataSettings,normalMapNode->GetValue()));
			}
		}

		auto fog = root->AddBlock("fog");
		if((node = vmtRoot->GetNode("$fogenable")) != nullptr)
		{
			auto fogEnabled = true;
			vmt_parameter_to_numeric_type<bool>(node,fogEnabled);
			fog->AddValue("bool","enabled",fogEnabled ? "1" : "0");
		}
		
		auto fInchesToMeters = [](float inches) {
			return inches *0.0254f;
		};
		if((node = vmtRoot->GetNode("$fogstart")) != nullptr)
		{
			auto fogStart = 0.f;
			vmt_parameter_to_numeric_type<float>(node,fogStart);
			fog->AddValue("float","start",std::to_string(fInchesToMeters(fogStart)));
		}
			
		if((node = vmtRoot->GetNode("$fogend")) != nullptr)
		{
			auto fogEnd = 0.f;
			vmt_parameter_to_numeric_type<float>(node,fogEnd);
			fog->AddValue("float","end",std::to_string(fInchesToMeters(fogEnd)));
		}
			
		if((node = vmtRoot->GetNode("$fogcolor")) != nullptr)
		{
			auto vColor = vmt_parameter_to_color(*node);
			if(vColor.has_value())
			{
				Color color {*vColor};
				fog->AddValue("color","color",std::to_string(color.r) +' ' +std::to_string(color.g) +' ' +std::to_string(color.b));
			}
		}
	}
	else if(shader == "teeth")
	{
		shaderName = "pbr";
		phongOverride = 1.f; // Hack
	}
	else if(shader == "unlitgeneric")
		shaderName = "unlit";
	else //if(shader == "LightmappedGeneric")
		shaderName = "pbr";

	auto isParticleShader = (shaderName == "particle");
	if(isParticleShader == false)
	{
		// If $vertexalpha parameter is set, it's probably a particle material?
		if((node = vmtRoot->GetNode("$vertexalpha")) != nullptr)
			vmt_parameter_to_numeric_type<bool>(node,isParticleShader);
	}

	if((node = vmtRoot->GetNode("$additive")) != nullptr)
	{
		auto additive = false;
		if(vmt_parameter_to_numeric_type<bool>(node,additive) && additive)
			root->AddValue("bool","additive","1");
		if(additive && isParticleShader)
		{
			// Additive needs lower color values to match Source more closely
			// (Example: "Explosion_Flashup" particle system with "effects/softglow" material)
			root->AddValue("vector4","color_factor","0.1 0.1 0.1 1.0");
			root->AddValue("vector4","bloom_color_factor","0 0 0 1");
		}
		else
		{
			// Additive isn't supported right now, so we'll just hide
			// the material for the time being
			root->AddValue("float","alpha_factor","0");
		}
	}

	auto bHasGlowMap = false;
	auto bHasGlow = false;
	if((node = vmtRoot->GetNode("$selfillummask")) != nullptr)
	{
		if(node->GetType() == VMTNodeType::NODE_TYPE_STRING)
		{
			auto *selfIllumMaskNode = static_cast<VTFLib::Nodes::CVMTStringNode*>(node);
			root->AddData(Material::GLOW_MAP_IDENTIFIER,std::make_shared<ds::Texture>(*dataSettings,selfIllumMaskNode->GetValue()));
			bHasGlowMap = true;
			bHasGlow = true;
		}
	}
	auto hasDiffuseMap = false;
	// Prefer HDR textures over LDR
	if((node = vmtRoot->GetNode("$hdrcompressedTexture")) != nullptr)
	{
		if(node->GetType() == VMTNodeType::NODE_TYPE_STRING)
		{
			hasDiffuseMap = true;
			auto *baseTextureStringNode = static_cast<VTFLib::Nodes::CVMTStringNode*>(node);
			root->AddData(Material::ALBEDO_MAP_IDENTIFIER,std::make_shared<ds::Texture>(*dataSettings,baseTextureStringNode->GetValue()));
		}
	}
	if(hasDiffuseMap == false && (node = vmtRoot->GetNode("$hdrbasetexture")) != nullptr)
	{
		if(node->GetType() == VMTNodeType::NODE_TYPE_STRING)
		{
			hasDiffuseMap = true;
			auto *baseTextureStringNode = static_cast<VTFLib::Nodes::CVMTStringNode*>(node);
			root->AddData(Material::ALBEDO_MAP_IDENTIFIER,std::make_shared<ds::Texture>(*dataSettings,baseTextureStringNode->GetValue()));
		}
	}
	if(hasDiffuseMap == false && (node = vmtRoot->GetNode("$basetexture")) != nullptr)
	{
		if(node->GetType() == VMTNodeType::NODE_TYPE_STRING)
		{
			auto *baseTextureStringNode = static_cast<VTFLib::Nodes::CVMTStringNode*>(node);
			root->AddData(Material::ALBEDO_MAP_IDENTIFIER,std::make_shared<ds::Texture>(*dataSettings,baseTextureStringNode->GetValue()));

			if(bHasGlowMap == false && (node = vmtRoot->GetNode("$selfillum")) != nullptr)
			{
				root->AddData(Material::GLOW_MAP_IDENTIFIER,std::make_shared<ds::Texture>(*dataSettings,baseTextureStringNode->GetValue()));
				root->AddValue("int","glow_blend_diffuse_mode","3");
				root->AddValue("float","glow_blend_diffuse_scale","1");
				root->AddValue("bool","glow_alpha_only","1");
				bHasGlow = true;
			}
		}
	}
	
	if((node = vmtRoot->GetNode("$detail")) != nullptr)
	{
		if(node->GetType() == VMTNodeType::NODE_TYPE_STRING)
		{
			auto *texNode = static_cast<VTFLib::Nodes::CVMTStringNode*>(node);
			auto detailBlendMode = msys::DetailMode::Invalid;
			static_assert(std::is_same_v<std::underlying_type_t<decltype(detailBlendMode)>,uint8_t>);
			node = vmtRoot->GetNode("$detailblendmode");
			if(node)
				vmt_parameter_to_numeric_type<uint8_t>(node,reinterpret_cast<uint8_t&>(detailBlendMode));
			if(umath::to_integral(detailBlendMode) >= 0 && umath::to_integral(detailBlendMode) < umath::to_integral(msys::DetailMode::Count))
			{
				root->AddValue("string","detail_blend_mode",msys::to_string(detailBlendMode));
				root->AddData("detail_map",std::make_shared<ds::Texture>(*dataSettings,texNode->GetValue()));

				node = vmtRoot->GetNode("$detailscale");
				if(node)
				{
					Vector2 uvScale {4.f,4.f};
					if(node->GetType() == VMTNodeType::NODE_TYPE_STRING)
					{
						auto values = get_vmt_matrix(*static_cast<VTFLib::Nodes::CVMTStringNode*>(node));
						if(values.size() > 0)
						{
							uvScale[0] = values.at(0);
							uvScale[1] = values.at(0);
						}
						if(values.size() > 1)
							uvScale[1] = values.at(1);
					}
					else
					{
						vmt_parameter_to_numeric_type<float>(node,uvScale.x);
						uvScale.y = uvScale.x;
					}
					root->AddValue("vector2","detail_uv_scale",std::to_string(uvScale[0]) +' ' +std::to_string(uvScale[1]));
				}

				node = vmtRoot->GetNode("$detailblendfactor");
				if(node)
				{
					auto detailBlendFactor = 1.f;
					if(node->GetType() == VMTNodeType::NODE_TYPE_STRING)
					{
						auto values = get_vmt_matrix(*static_cast<VTFLib::Nodes::CVMTStringNode*>(node));
						for(uint8_t i=0;i<umath::min<uint32_t>(static_cast<uint32_t>(values.size()),static_cast<uint32_t>(1u));++i)
							detailBlendFactor = values.at(i);
					}
					else
						vmt_parameter_to_numeric_type<float>(node,detailBlendFactor);
					root->AddValue("float","detail_factor",std::to_string(detailBlendFactor));
				}
				
				Vector3 detailColorFactor {1.f,1.f,1.f};
				node = vmtRoot->GetNode("$detailtint");
				if(node)
				{
					auto color = vmt_parameter_to_color(*node);
					if(color.has_value())
						detailColorFactor = *color;
				}
				root->AddValue("vector","detail_color_factor",std::to_string(detailColorFactor[0]) +' ' +std::to_string(detailColorFactor[1]) +' ' +std::to_string(detailColorFactor[2]));
			}
		}
	}

	// These are custom parameters; Used to make it easier to import PBR assets into Pragma
	if((node = vmtRoot->GetNode("$rmatexture")) != nullptr && node->GetType() == VMTNodeType::NODE_TYPE_STRING)
	{
		auto *metalnessNode = static_cast<VTFLib::Nodes::CVMTStringNode*>(node);
		root->AddData(Material::RMA_MAP_IDENTIFIER,std::make_shared<ds::Texture>(*dataSettings,metalnessNode->GetValue()));
	}

	if((node = vmtRoot->GetNode("$emissiontexture")) != nullptr && node->GetType() == VMTNodeType::NODE_TYPE_STRING)
	{
		auto *emissionNode = static_cast<VTFLib::Nodes::CVMTStringNode*>(node);
		root->AddData(Material::EMISSION_MAP_IDENTIFIER,std::make_shared<ds::Texture>(*dataSettings,emissionNode->GetValue()));
	}

	if((node = vmtRoot->GetNode("$metalnessfactor")) != nullptr)
	{
		float factor = 0.f;
		if(vmt_parameter_to_numeric_type<float>(node,factor))
			root->AddData("metalness_factor",std::make_shared<ds::Float>(*dataSettings,std::to_string(factor)));
	}
	if((node = vmtRoot->GetNode("$roughnessfactor")) != nullptr)
	{
		float factor = 0.f;
		if(vmt_parameter_to_numeric_type<float>(node,factor))
			root->AddData("roughness_factor",std::make_shared<ds::Float>(*dataSettings,std::to_string(factor)));
	}
	if((node = vmtRoot->GetNode("$specularfactor")) != nullptr)
	{
		float factor = 0.f;
		if(vmt_parameter_to_numeric_type<float>(node,factor))
			root->AddData("specular_factor",std::make_shared<ds::Float>(*dataSettings,std::to_string(factor)));
	}
	if((node = vmtRoot->GetNode("$emissionfactor")) != nullptr)
	{
		float factor = 0.f;
		if(vmt_parameter_to_numeric_type<float>(node,factor))
			root->AddData("emission_factor",std::make_shared<ds::Float>(*dataSettings,std::to_string(factor)));
	}

	if(bHasGlow)
	{
		auto *nodeFresnel = vmtRoot->GetNode("$selfillumfresnelminmaxexp");
		if(nodeFresnel && nodeFresnel->GetType() == VMTNodeType::NODE_TYPE_STRING)
		{
			auto values = get_vmt_matrix(*static_cast<VTFLib::Nodes::CVMTStringNode*>(nodeFresnel));
			if(values.at(2) == 0.f) // TODO: Not entirely sure this is sensible
			{
				root->AddValue("int","glow_blend_diffuse_mode","4");
				root->AddValue("float","glow_blend_diffuse_scale","1");
			}
		}
	}
	if(shader == "worldvertextransition" && (node = vmtRoot->GetNode("$basetexture2")) != nullptr)
	{
		shaderName = "pbr_blend";
		if(node->GetType() == VMTNodeType::NODE_TYPE_STRING)
		{
			auto *baseTexture2StringNode = static_cast<VTFLib::Nodes::CVMTStringNode*>(node);
			root->AddData(Material::ALBEDO_MAP2_IDENTIFIER,std::make_shared<ds::Texture>(*dataSettings,baseTexture2StringNode->GetValue()));
		}
	}
	if(bWater == false && (node = vmtRoot->GetNode("$bumpmap")) != nullptr)
	{
		if(node->GetType() == VMTNodeType::NODE_TYPE_STRING)
		{
			auto *bumpMapNode = static_cast<VTFLib::Nodes::CVMTStringNode*>(node);
			root->AddData(Material::NORMAL_MAP_IDENTIFIER,std::make_shared<ds::Texture>(*dataSettings,bumpMapNode->GetValue()));
		}
	}
	if((node = vmtRoot->GetNode("$envmapmask")) != nullptr)
	{
		if(node->GetType() == VMTNodeType::NODE_TYPE_STRING)
		{
			auto *specularMapNode = static_cast<VTFLib::Nodes::CVMTStringNode*>(node);
			root->AddData("specular_map",std::make_shared<ds::Texture>(*dataSettings,specularMapNode->GetValue()));
		}
	}
	if((node = vmtRoot->GetNode("$additive")) != nullptr)
	{
		root->AddValue("int","alpha_mode",std::to_string(umath::to_integral(AlphaMode::Blend)));
		get_vmt_data<ds::Bool,int32_t>(root,*dataSettings,"black_to_alpha",node);
	}
	if((node = vmtRoot->GetNode("$phong")) != nullptr)
		get_vmt_data<ds::Bool,int32_t>(root,*dataSettings,"phong_normal_alpha",node);
	if((node = vmtRoot->GetNode("$phongexponent")) != nullptr)
	{
		if(std::isnan(phongOverride))
		{
			get_vmt_data<ds::Float,float>(root,*dataSettings,"phong_intensity",node,[](float v) -> float {
				return sqrtf(v);
				});
		}
		else
			root->AddData("phong_intensity",std::make_shared<ds::Float>(*dataSettings,std::to_string(phongOverride)));
	}
	if((node = vmtRoot->GetNode("$phongboost")) != nullptr)
	{
		get_vmt_data<ds::Float,float>(root,*dataSettings,"phong_shininess",node,[](float v) -> float {
			return v *2.f;
			});
	}
	if((node = vmtRoot->GetNode("$parallaxmap")) != nullptr)
	{
		if(node->GetType() == VMTNodeType::NODE_TYPE_STRING)
		{
			auto *parallaxMapNode = static_cast<VTFLib::Nodes::CVMTStringNode*>(node);
			root->AddData(Material::PARALLAX_MAP_IDENTIFIER,std::make_shared<ds::Texture>(*dataSettings,parallaxMapNode->GetValue()));
		}
	}
	if((node = vmtRoot->GetNode("$parallaxmapscale")) != nullptr)
	{
		get_vmt_data<ds::Float,float>(root,*dataSettings,"parallax_height_scale",node,[](float v) -> float {
			return v *0.025f;
			});
	}

	auto translucent = false;
	if(((node = vmtRoot->GetNode("$translucent")) && vmt_parameter_to_numeric_type<bool>(node,translucent)))
		root->AddValue("int","alpha_mode",std::to_string(umath::to_integral(AlphaMode::Blend)));

	float alphaFactor = 1.f;
	if(((node = vmtRoot->GetNode("$alpha")) && vmt_parameter_to_numeric_type<float>(node,alphaFactor)))
		root->AddValue("float","alpha_factor",std::to_string(alphaFactor));

	auto alphaTest = false;
	if(((node = vmtRoot->GetNode("$alphatest")) && vmt_parameter_to_numeric_type<bool>(node,alphaTest)))
	{
		root->AddValue("int","alpha_mode",std::to_string(umath::to_integral(AlphaMode::Mask)));
		auto alphaCutoff = 0.5f; // TODO: Confirm that the default for Source is 0.5
		if(node = vmtRoot->GetNode("$alphatestreference"))
			vmt_parameter_to_numeric_type<float>(node,alphaCutoff);
		root->AddValue("float","alpha_cutoff",std::to_string(alphaCutoff));
	}

	if((node = vmtRoot->GetNode("$surfaceprop")) != nullptr)
	{
		if(node->GetType() == VMTNodeType::NODE_TYPE_STRING)
		{
			auto *surfacePropNode = static_cast<VTFLib::Nodes::CVMTStringNode*>(node);
			std::string surfaceProp = surfacePropNode->GetValue();
			ustring::to_lower(surfaceProp);
			std::string surfaceMaterial = "concrete";
			std::unordered_map<std::string,std::string> translateMaterial = {
#include "impl_surfacematerials.h"
			};
			auto it = translateMaterial.find(surfaceProp);
			if(it != translateMaterial.end())
				surfaceMaterial = it->second;
			root->AddData("surfacematerial",std::make_shared<ds::String>(*dataSettings,surfaceMaterial));
		}
	}
	if((node = vmtRoot->GetNode("$compress")) != nullptr)
	{
		if(node->GetType() == VMTNodeType::NODE_TYPE_STRING)
		{
			auto *compressMapNode = static_cast<VTFLib::Nodes::CVMTStringNode*>(node);
			root->AddData(Material::WRINKLE_COMPRESS_MAP_IDENTIFIER,std::make_shared<ds::Texture>(*dataSettings,compressMapNode->GetValue()));
		}
	}
	if((node = vmtRoot->GetNode("$stretch")) != nullptr)
	{
		if(node->GetType() == VMTNodeType::NODE_TYPE_STRING)
		{
			auto *stretchMapNode = static_cast<VTFLib::Nodes::CVMTStringNode*>(node);
			root->AddData(Material::WRINKLE_STRETCH_MAP_IDENTIFIER,std::make_shared<ds::Texture>(*dataSettings,stretchMapNode->GetValue()));
		}
	}
	if((node = vmtRoot->GetNode("$phongexponenttexture")) != nullptr)
	{
		if(node->GetType() == VMTNodeType::NODE_TYPE_STRING)
		{
			auto *phongExponentTexture = static_cast<VTFLib::Nodes::CVMTStringNode*>(node);
			root->AddData(Material::EXPONENT_MAP_IDENTIFIER,std::make_shared<ds::Texture>(*dataSettings,phongExponentTexture->GetValue()));
		}
	}

	if((node = vmtRoot->GetNode("$fakepbr_layer")) != nullptr)
	{
		int32_t fakePbrLayer = 0;
		if(vmt_parameter_to_numeric_type<int32_t>(node,fakePbrLayer))
			shaderName = "nodraw"; // Fake PBR uses multiple mesh layers; We'll just hide the additional ones
	}

	if((node = vmtRoot->GetNode("$no_draw")) != nullptr)
	{
		int32_t noDraw = 0;
		if(vmt_parameter_to_numeric_type<int32_t>(node,noDraw))
			shaderName = "nodraw";
	}

	if((node = vmtRoot->GetNode("$ambientoccltexture")) != nullptr)
	{
		if(node->GetType() == VMTNodeType::NODE_TYPE_STRING)
		{
			auto *aoTex = static_cast<VTFLib::Nodes::CVMTStringNode*>(node);
			root->AddData("ao_map",std::make_shared<ds::Texture>(*dataSettings,aoTex->GetValue()));
		}
	}
	
	if(shaderName == "pbr" || shaderName == "pbr_blend")
	{
		if(root->HasValue(Material::RMA_MAP_IDENTIFIER) == false)
		{
			auto rmaInfo = root->AddBlock("rma_info");
			rmaInfo->AddValue("bool","requires_metalness_update","1");
			rmaInfo->AddValue("bool","requires_roughness_update","1");
			if(root->HasValue("ao_map") == false)
				rmaInfo->AddValue("bool","requires_ao_update","1");
		}
	}

	if(isParticleShader == false)
	{
		if(((node = vmtRoot->GetNode("$color")) || (node = vmtRoot->GetNode("$color2"))) && node->GetType() == VMTNodeType::NODE_TYPE_STRING)
		{
			auto color = vmt_parameter_to_color(*node);
			if(color.has_value())
			{
				root->AddValue("vector4","color_factor",std::to_string(color->r) +' ' +std::to_string(color->g) +' ' +std::to_string(color->b) +" 1.0");
				if(root->HasValue(Material::ALBEDO_MAP_IDENTIFIER) == false) // $color / $color2 attributes work without a diffuse texture
					root->AddData(Material::ALBEDO_MAP_IDENTIFIER,std::make_shared<ds::Texture>(*dataSettings,"white"));
			}
		}
	}
	
	if(!LoadVMTData(vmt,shader,*root,shaderName))
		return false;
	auto mat = Material::Create(static_cast<msys::MaterialManager&>(GetAssetManager()),shaderName,root);
	if(!mat)
		return false;
	std::string err;
	outFilePath = outputPath +".pmat";
	if(!mat->Save(outFilePath,err,true))
	{
		m_error = std::move(err);
		return false;
	}
	return true;
}
bool msys::SourceVmtFormatHandler::LoadVMTData(VTFLib::CVMTFile &vmt,const std::string &vmtShader,ds::Block &rootData,std::string &matShader) {return true;}
#endif
