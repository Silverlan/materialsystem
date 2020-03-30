#include "materialmanager.h"
#include <sharedutils/util_string.h>
#ifndef DISABLE_VMT_SUPPORT
#include <VMTFile.h>
#include "util_vmt.hpp"
#endif

#ifndef DISABLE_VMT_SUPPORT
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

	std::array<float,3> data {
		(substrings.size() > 0) ? util::to_float(substrings.at(0)) : 0.f,
		(substrings.size() > 1) ? util::to_float(substrings.at(1)) : 0.f,
		(substrings.size() > 2) ? util::to_float(substrings.at(2)) : 0.f,
	};
	return data;
}

bool MaterialManager::InitializeVMTData(VTFLib::CVMTFile &vmt,LoadInfo &info,ds::Block &rootData,ds::Settings &settings,const std::string &shader) {return true;}

bool MaterialManager::LoadVMT(VTFLib::CVMTFile &vmt,LoadInfo &info)
{
	auto *vmtRoot = vmt.GetRoot();
	std::string shader = vmtRoot->GetName();
	ustring::to_lower(shader);

	merge_dx_node_values(*vmtRoot);
	auto phongOverride = std::numeric_limits<float>::quiet_NaN();
	auto bWater = false;
	std::string shaderName;
	VTFLib::Nodes::CVMTNode *node = nullptr;
	auto dataSettings = CreateDataSettings();
	auto root = std::make_shared<ds::Block>(*dataSettings);
	if(shader == "worldvertextransition")
		shaderName = "textured";
	else if(shader == "sprite")
	{
		shaderName = "particle";
		root->AddValue("bool","black_to_alpha","1");
	}
	else if(shader == "water")
	{
		bWater = true;
		shaderName = "water";
		if((node = vmtRoot->GetNode("$bumpmap")) != nullptr)
		{
			if(node->GetType() == VMTNodeType::NODE_TYPE_STRING)
			{
				auto *bumpMapNode = static_cast<VTFLib::Nodes::CVMTStringNode*>(node);
				root->AddData(Material::DUDV_MAP_IDENTIFIER,std::make_shared<ds::Texture>(*dataSettings,bumpMapNode->GetValue()));
			}
		}
		if((node = vmtRoot->GetNode("$normalmap")) != nullptr)
		{
			if(node->GetType() == VMTNodeType::NODE_TYPE_STRING)
			{
				auto *normalMapNode = static_cast<VTFLib::Nodes::CVMTStringNode*>(node);
				root->AddData(Material::NORMAL_MAP_IDENTIFIER,std::make_shared<ds::Texture>(*dataSettings,normalMapNode->GetValue()));
			}
		}
	}
	else if(shader == "teeth")
	{
		shaderName = "textured";
		phongOverride = 1.f; // Hack
	}
	else if(shader == "unlitgeneric")
		shaderName = "unlit";
	else //if(shader == "LightmappedGeneric")
		shaderName = "textured";

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

	// These are custom parameters; Used to make it easier to import PBR assets into Pragma
	if((node = vmtRoot->GetNode("$metalnesstexture")) != nullptr && node->GetType() == VMTNodeType::NODE_TYPE_STRING)
	{
		auto *metalnessNode = static_cast<VTFLib::Nodes::CVMTStringNode*>(node);
		root->AddData(Material::METALNESS_MAP_IDENTIFIER,std::make_shared<ds::Texture>(*dataSettings,metalnessNode->GetValue()));
	}
	auto hasPbrRoughness = false;
	if((node = vmtRoot->GetNode("$roughnesstexture")) != nullptr && node->GetType() == VMTNodeType::NODE_TYPE_STRING)
	{
		auto *roughnessNode = static_cast<VTFLib::Nodes::CVMTStringNode*>(node);
		root->AddData(Material::ROUGHNESS_MAP_IDENTIFIER,std::make_shared<ds::Texture>(*dataSettings,roughnessNode->GetValue()));
		hasPbrRoughness = true;
	}
	auto hasPbrGlossiness = false;
	if(hasPbrRoughness == false && (node = vmtRoot->GetNode("$glossinesstexture")) != nullptr && node->GetType() == VMTNodeType::NODE_TYPE_STRING)
	{
		auto *glossinessNode = static_cast<VTFLib::Nodes::CVMTStringNode*>(node);
		root->AddData(Material::SPECULAR_MAP_IDENTIFIER,std::make_shared<ds::Texture>(*dataSettings,glossinessNode->GetValue()));
		hasPbrGlossiness = true;
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
	if((node = vmtRoot->GetNode("$envmap")) != nullptr && hasPbrRoughness == false && hasPbrGlossiness == false)
	{
		if(node->GetType() == VMTNodeType::NODE_TYPE_STRING)
		{
			auto *envMapNode = static_cast<VTFLib::Nodes::CVMTStringNode*>(node);
			std::string val = envMapNode->GetValue();
			auto lval = val;
			ustring::to_lower(lval);
			if(lval != "env_cubemap")
				root->AddData(Material::SPECULAR_MAP_IDENTIFIER,std::make_shared<ds::Texture>(*dataSettings,val));
		}
	}
	if((node = vmtRoot->GetNode("$envmapmask")) != nullptr && hasPbrRoughness == false && hasPbrGlossiness == false)
	{
		if(node->GetType() == VMTNodeType::NODE_TYPE_STRING)
		{
			auto *specularMapNode = static_cast<VTFLib::Nodes::CVMTStringNode*>(node);
			root->AddData(Material::SPECULAR_MAP_IDENTIFIER,std::make_shared<ds::Texture>(*dataSettings,specularMapNode->GetValue()));
		}
	}
	if((node = vmtRoot->GetNode("$additive")) != nullptr)
		get_vmt_data<ds::Bool,int32_t>(root,*dataSettings,"black_to_alpha",node);
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
	if((node = vmtRoot->GetNode("$translucent")) != nullptr)
		get_vmt_data<ds::Bool,int32_t>(root,*dataSettings,"translucent",node);
	if((node = vmtRoot->GetNode("$alphatest")) != nullptr)
		get_vmt_data<ds::Bool,int32_t>(root,*dataSettings,"translucent",node);
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

	info.shader = shaderName;
	info.root = root;
	return InitializeVMTData(vmt,info,*root,*dataSettings,shader);
}
#endif
