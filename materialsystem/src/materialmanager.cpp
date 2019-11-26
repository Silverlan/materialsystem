/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "materialmanager.h"
#include "textureinfo.h"
#include <fsys/filesystem.h>
#include <sharedutils/util_string.h>
#include <sharedutils/util_file.h>
#include <sharedutils/util.h>
#include <array>
#ifdef ENABLE_VMT_SUPPORT
#include <VMTFile.h>
#endif

#pragma optimize("",off)
static const std::unordered_map<std::string,std::string> ENUM_VARS = { // These have to correspond with their respective vulkan enum values!
	{"SAMPLER_ADDRESS_MODE_REPEAT","0"},
	{"SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT","1"},
	{"SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE","2"},
	{"SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER","3"},
	{"SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE","4"},

	{"MIPMAP_MODE_IGNORE","-1"},
	{"MIPMAP_MODE_LOAD_OR_GENERATE","0"},
	{"MIPMAP_MODE_LOAD","1"},
	{"MIPMAP_MODE_GENERATE","2"},

	{"BORDER_COLOR_FLOAT_TRANSPARENT_BLACK","0"},
	{"BORDER_COLOR_INT_TRANSPARENT_BLACK","1"},
	{"BORDER_COLOR_FLOAT_OPAQUE_BLACK","2"},
	{"BORDER_COLOR_INT_OPAQUE_BLACK","3"},
	{"BORDER_COLOR_FLOAT_OPAQUE_WHITE","4"},
	{"BORDER_COLOR_INT_OPAQUE_WHITE","5"}
};

#ifdef ENABLE_VMT_SUPPORT
template<class TData,typename TInternal>
	static void get_vmt_data(const std::shared_ptr<ds::Block> &root,ds::Settings &dataSettings,const std::string &key,VTFLib::Nodes::CVMTNode *node,const std::function<TInternal(TInternal)> &translate=nullptr)
{
	auto type = node->GetType();
	if(type == VMTNodeType::NODE_TYPE_SINGLE)
	{
		auto *singleNode = static_cast<VTFLib::Nodes::CVMTSingleNode*>(node);
		auto v = singleNode->GetValue();
		if(translate != nullptr)
			v = translate(v);
		root->AddData(key,std::make_shared<TData>(dataSettings,std::to_string(v)));
	}
	else if(type == VMTNodeType::NODE_TYPE_INTEGER)
	{
		auto *integerNode = static_cast<VTFLib::Nodes::CVMTIntegerNode*>(node);
		auto v = static_cast<float>(integerNode->GetValue());
		if(translate != nullptr)
			v = translate(v);
		root->AddData(key,std::make_shared<TData>(dataSettings,std::to_string(v)));
	}
	else if(type == VMTNodeType::NODE_TYPE_STRING)
	{
		auto *stringNode = static_cast<VTFLib::Nodes::CVMTStringNode*>(node);
		auto v = util::to_float(stringNode->GetValue());
		if(translate != nullptr)
			v = translate(v);
		root->AddData(key,std::make_shared<TData>(dataSettings,std::to_string(v)));
	}
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
#endif

MaterialManager::LoadInfo::LoadInfo()
	: material(nullptr)
{}

MaterialManager::MaterialManager()
	: m_error{}
{}
MaterialManager::~MaterialManager()
{
	Clear();
}

Material *MaterialManager::CreateMaterial(const std::string &shader,const std::shared_ptr<ds::Block> &root) {return CreateMaterial(nullptr,shader,root);}
Material *MaterialManager::CreateMaterial(const std::string &identifier,const std::string &shader,const std::shared_ptr<ds::Block> &root) {return CreateMaterial(&identifier,shader,root);}
Material *MaterialManager::CreateMaterial(const std::string *identifier,const std::string &shader,std::shared_ptr<ds::Block> root)
{
	std::string matId;
	if(identifier != nullptr)
	{
		matId = *identifier;
		ustring::to_lower(matId);
		auto *mat = FindMaterial(matId);
		if(mat != nullptr)
			return mat;
	}
	else
		matId = "__anonymous" +(m_unnamedIdx++);
	if(root == nullptr)
	{
		auto dataSettings = ds::create_data_settings(ENUM_VARS);
		root = std::make_shared<ds::Block>(*dataSettings);
	}
	auto *mat = CreateMaterial<Material>(shader,root); // Claims ownership of 'root' and frees the memory at destruction
	mat->SetName(matId);
	m_materials.insert(decltype(m_materials)::value_type{ToMaterialIdentifier(matId),mat->GetHandle()});
	return mat;
}

std::string MaterialManager::PathToIdentifier(const std::string &path,std::string *ext,bool &hadExtension) const
{
	auto matPath = FileManager::GetNormalizedPath(path);
	if(ufile::get_extension(matPath,ext) == false)
	{
		hadExtension = false;
		*ext = "wmi";
#ifdef ENABLE_VMT_SUPPORT
		if(!FileManager::Exists("materials\\" +matPath +'.' +*ext))
			*ext = "vmt";
#endif
		matPath += '.' +*ext;
	}
	else
		hadExtension = true;
	return matPath;
}

std::string MaterialManager::PathToIdentifier(const std::string &path,std::string *ext) const
{
	auto hadExtension = false;
	return PathToIdentifier(path,ext,hadExtension);
}
std::string MaterialManager::PathToIdentifier(const std::string &path) const
{
	std::string ext;
	return PathToIdentifier(path,&ext);
}

Material *MaterialManager::FindMaterial(const std::string &identifier,std::string &internalMatId) const
{
	internalMatId = PathToIdentifier(identifier);
	auto it = m_materials.find(ToMaterialIdentifier(internalMatId));
	if(it == m_materials.end())
		return nullptr;
	return it->second.get();
}
Material *MaterialManager::FindMaterial(const std::string &identifier) const
{
	std::string internalMatId;
	return FindMaterial(identifier,internalMatId);
}
std::shared_ptr<ds::Settings> MaterialManager::CreateDataSettings() const {return ds::create_data_settings(ENUM_VARS);}

std::string MaterialManager::ToMaterialIdentifier(const std::string &id) const
{
	auto identifier = id;
	ufile::remove_extension_from_filename(identifier);
	ustring::to_lower(identifier);
	return identifier;
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
#include <iostream>
bool MaterialManager::Load(const std::string &path,LoadInfo &info,bool bReload)
{
	std::string ext;
	auto &matId = info.identifier;
	auto bHadExtension = false;
	matId = PathToIdentifier(path,&ext,bHadExtension);
	matId = FileManager::GetCanonicalizedPath(matId);

	auto it = m_materials.find(ToMaterialIdentifier(matId));
	if(it != m_materials.end())
	{
		if(!it->second.IsValid())
			m_materials.erase(it);
		else
		{
			info.material = it->second.get();
			info.shader = info.material->GetShaderIdentifier();
			if(bReload == false)
				return true;
		}
	}
	std::string absPath = "materials\\";
	absPath += path;
	if(bHadExtension == false)
		absPath += '.' +ext;
	auto sub = matId;
	auto f = FileManager::OpenFile(absPath.c_str(),"r");
	if(f == nullptr)
		return false;
	auto handleLoadError = [this](const std::string &matId,Material **mat) -> bool {
		//auto matErr = m_materials.find("error");
		//if(matErr == m_materials.end())
		//	return false;
		auto *nmat = CreateMaterial<Material>();
		m_materials.insert(decltype(m_materials)::value_type{ToMaterialIdentifier(matId),nmat->GetHandle()});
		*mat = nmat;
		return true;
	};
#ifdef ENABLE_VMT_SUPPORT
	if(ext == "vmt") // Load from vmt-file
	{
		auto sz = f->GetSize();
		std::vector<uint8_t> data(sz);
		f->Read(data.data(),sz);
		f = nullptr;
		while(sz > 0 && data[sz -1] == '\0')
			--sz;
		VTFLib::CVMTFile vmt {};
		if(vmt.Load(data.data(),static_cast<vlUInt>(sz)) == vlTrue)
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
				shaderName = "texturedalphatransition";
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
					root->AddData(Material::DIFFUSE_MAP_IDENTIFIER,std::make_shared<ds::Texture>(*dataSettings,baseTextureStringNode->GetValue()));
				}
			}
			if(hasDiffuseMap == false && (node = vmtRoot->GetNode("$hdrbasetexture")) != nullptr)
			{
				if(node->GetType() == VMTNodeType::NODE_TYPE_STRING)
				{
					hasDiffuseMap = true;
					auto *baseTextureStringNode = static_cast<VTFLib::Nodes::CVMTStringNode*>(node);
					root->AddData(Material::DIFFUSE_MAP_IDENTIFIER,std::make_shared<ds::Texture>(*dataSettings,baseTextureStringNode->GetValue()));
				}
			}
			if(hasDiffuseMap == false && (node = vmtRoot->GetNode("$basetexture")) != nullptr)
			{
				if(node->GetType() == VMTNodeType::NODE_TYPE_STRING)
				{
					auto *baseTextureStringNode = static_cast<VTFLib::Nodes::CVMTStringNode*>(node);
					root->AddData(Material::DIFFUSE_MAP_IDENTIFIER,std::make_shared<ds::Texture>(*dataSettings,baseTextureStringNode->GetValue()));

					if(bHasGlowMap == false && (node = vmtRoot->GetNode("$selfillum")) != nullptr)
					{
						root->AddData(Material::GLOW_MAP_IDENTIFIER,std::make_shared<ds::Texture>(*dataSettings,baseTextureStringNode->GetValue()));
						root->AddValue("int","glow_blend_diffuse_mode","1");
						root->AddValue("float","glow_blend_diffuse_scale","6");
						root->AddValue("bool","glow_alpha_only","1");
						bHasGlow = true;
					}
				}
			}
			if(bHasGlow)
			{
				auto *nodeFresnel = vmtRoot->GetNode("$selfillumfresnelminmaxexp");
				if(nodeFresnel->GetType() == VMTNodeType::NODE_TYPE_STRING)
				{
					auto values = get_vmt_matrix(*static_cast<VTFLib::Nodes::CVMTStringNode*>(nodeFresnel));
					if(values.at(2) == 0.f) // TODO: Not entirely sure this is sensible
					{
						root->AddValue("int","glow_blend_diffuse_mode","4");
						root->AddValue("float","glow_blend_diffuse_scale","1");
					}
				}
			}
			if((node = vmtRoot->GetNode("$basetexture2")) != nullptr)
			{
				if(shaderName == "textured")
					shaderName = "texturedalphatransition";
				if(node->GetType() == VMTNodeType::NODE_TYPE_STRING)
				{
					auto *baseTexture2StringNode = static_cast<VTFLib::Nodes::CVMTStringNode*>(node);
					root->AddData(Material::DIFFUSE_MAP2_IDENTIFIER,std::make_shared<ds::Texture>(*dataSettings,baseTexture2StringNode->GetValue()));
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
			if((node = vmtRoot->GetNode("$envmap")) != nullptr)
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
			if((node = vmtRoot->GetNode("$envmapmask")) != nullptr)
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
			info.shader = shaderName;
			info.root = root;
			return true;
		}
		return false;
	}
#endif

	auto root = ds::System::ReadData(f,ENUM_VARS);
	f.reset();
	if(root == nullptr)
		return handleLoadError(matId,&info.material);
	auto *data = root->GetData();
	std::string shader;
	std::shared_ptr<ds::Block> matData = nullptr;
	for(auto it=data->begin();it!=data->end();it++)
	{
		auto &val = it->second;
		if(val->IsBlock()) // Shader has to be first block
		{
			matData = std::static_pointer_cast<ds::Block>(val);
			shader = it->first;
			break;
		}
	}
	if(matData == nullptr)
		return handleLoadError(matId,&info.material);
	root->DetachData(*matData);

	info.shader = shader;
	info.root = matData;
	return true;
}
Material *MaterialManager::Load(const std::string &path,bool bReload,bool *bFirstTimeError)
{
	if(bFirstTimeError != nullptr)
		*bFirstTimeError = false;
	LoadInfo info{};
	if(Load(path,info,bReload) == false)
	{
		if(info.material != nullptr) // 'bReload' was set true, material already existed and reloading failed
			return info.material;
		info.material = GetErrorMaterial();
		if(info.material == nullptr)
			return nullptr;
		if(bFirstTimeError != nullptr)
			*bFirstTimeError = true;
		info.material = info.material->Copy(); // Copy error material
	}
	else if(info.material != nullptr)
	{
		info.material->SetLoaded(true);
		if(bReload)
			info.material->Initialize(info.shader,info.root);
		return info.material;
	}
	else
	{
		info.material = new Material{*this,info.shader,info.root};
		info.material->SetLoaded(true);
	}
	info.material->SetName(info.identifier);
	m_materials.insert(decltype(m_materials)::value_type{ToMaterialIdentifier(info.identifier),info.material->GetHandle()});
	return info.material;
}
void MaterialManager::SetErrorMaterial(Material *mat)
{
	if(mat == nullptr)
		m_error = MaterialHandle{};
	else
		m_error = mat->GetHandle();
}
Material *MaterialManager::GetErrorMaterial() const {return m_error.get();}
const std::unordered_map<std::string,MaterialHandle> &MaterialManager::GetMaterials() const {return m_materials;}
void MaterialManager::Clear()
{
	for(auto &it : m_materials)
	{
		auto &hMaterial = it.second;
		if(hMaterial.IsValid())
			hMaterial->Remove();
	}
	m_materials.clear();
}
void MaterialManager::ClearUnused()
{
	for(auto it=m_materials.begin();it!=m_materials.end();)
	{
		auto &hMaterial = it->second;
		if(!hMaterial.IsValid() || (hMaterial.use_count() == 1 && hMaterial.get() != m_error.get())) // Use count = 1 => Only in use by us!
		{
			hMaterial->Remove();
			it = m_materials.erase(it);
		}
		else
			++it;
	}
}
#pragma optimize("",on)
