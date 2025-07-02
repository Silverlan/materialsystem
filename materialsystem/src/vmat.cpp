// SPDX-FileCopyrightText: (c) 2020 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#include "materialmanager.h"
#include <sharedutils/util_path.hpp>
#include <sharedutils/util_file.h>
#ifndef DISABLE_VMAT_SUPPORT
#include "util_vmat.hpp"

import source2;
#endif

#ifndef DISABLE_VMT_SUPPORT
bool MaterialManager::InitializeVMatData(source2::resource::Resource &resource, source2::resource::Material &vmat, LoadInfo &info, ds::Block &rootData, ds::Settings &settings, const std::string &shader, VMatOrigin origin) { return true; }

bool MaterialManager::LoadVMat(source2::resource::Resource &resource, LoadInfo &info)
{
	auto *s2Mat = dynamic_cast<source2::resource::Material *>(resource.FindBlock(source2::BlockType::DATA));
	if(s2Mat == nullptr)
		return false;
	auto dataSettings = CreateDataSettings();
	auto root = std::make_shared<ds::Block>(*dataSettings);

	std::string shaderName = "pbr";

	auto fLoadTexture = [this](const std::string &strPath) -> std::string {
		std::string inputPath;
		auto outputPath = vmat::get_vmat_texture_path(strPath, &inputPath);

		auto &texLoader = GetTextureImporter();
		if(texLoader) {
			auto outputPathNoExt = outputPath.GetString();
			ufile::remove_extension_from_filename(outputPathNoExt);
			texLoader(inputPath, "materials/" + outputPathNoExt);
		}

		return outputPath.GetString();
	};

	// Shared properties
	auto *surfaceProp = s2Mat->FindTextureAttr("PhysicsSurfaceProperties");
	if(surfaceProp)
		root->AddValue("string", "surfacematerial", *surfaceProp);

	auto *aoMap = s2Mat->FindTextureParam("g_tAmbientOcclusion");
	if(aoMap)
		fLoadTexture(*aoMap);

	auto origin = VMatOrigin::Source2;
	if(s2Mat->GetShaderName() == "vr_simple_2way_blend.vfx") {
		shaderName = "pbr_blend";
		auto *albedoMapA = s2Mat->FindTextureParam("g_tColorA");
		auto *albedoMapB = s2Mat->FindTextureParam("g_tColorB");

		if(albedoMapA)
			root->AddData(Material::ALBEDO_MAP_IDENTIFIER, std::make_shared<ds::Texture>(*dataSettings, fLoadTexture(*albedoMapA)));
		if(albedoMapB)
			root->AddData(Material::ALBEDO_MAP2_IDENTIFIER, std::make_shared<ds::Texture>(*dataSettings, fLoadTexture(*albedoMapB)));

		auto *normalMapA = s2Mat->FindTextureParam("g_tNormalA");
		auto *normalMapB = s2Mat->FindTextureParam("g_tNormalB");

		if(normalMapA)
			root->AddData(Material::NORMAL_MAP_IDENTIFIER, std::make_shared<ds::Texture>(*dataSettings, fLoadTexture(*normalMapA)));

		auto *metalnessA = s2Mat->FindFloatParam("g_flMetalnessA");
		auto *metalnessB = s2Mat->FindFloatParam("g_flMetalnessB");

		if(metalnessA)
			root->AddValue("float", "metalness_factor", std::to_string(*metalnessA));

		auto *roughnessA = s2Mat->FindVectorParam("TextureRoughnessA");
		auto *roughnessB = s2Mat->FindVectorParam("TextureRoughnessB");

		// TODO: Vector roughness?
		//if(roughnessA)
		//	root->AddValue("float","roughness_factor",std::to_string(*roughnessA));

		// TODO: Add support for secondary normal map / metalness / roughness
	}
	else if(s2Mat->GetShaderName() == "vr_cilia.vfx") {
		// TODO: Can these have a ColorB/NormalB/etc??
		auto *albedoMap = s2Mat->FindTextureParam("g_tColorA");
		if(albedoMap)
			root->AddData(Material::ALBEDO_MAP_IDENTIFIER, std::make_shared<ds::Texture>(*dataSettings, fLoadTexture(*albedoMap)));

		auto *normalMap = s2Mat->FindTextureParam("g_tNormalA");
		if(normalMap)
			root->AddData(Material::NORMAL_MAP_IDENTIFIER, std::make_shared<ds::Texture>(*dataSettings, fLoadTexture(*normalMap)));

		auto *metalness = s2Mat->FindFloatParam("g_flMetalnessA");
		if(metalness)
			root->AddValue("float", "metalness_factor", std::to_string(*metalness));

		auto *textureColor = s2Mat->FindVectorParam("TextureColorA");
		// TODO
	}
	else if(s2Mat->GetShaderName() == "hero.vfx") {
		origin = VMatOrigin::Dota2;
		// Dota 2
		auto *albedoMap = s2Mat->FindTextureParam("g_tColor");
		if(albedoMap)
			root->AddData(Material::ALBEDO_MAP_IDENTIFIER, std::make_shared<ds::Texture>(*dataSettings, fLoadTexture(*albedoMap)));

		auto *normalMap = s2Mat->FindTextureParam("g_tNormal");
		if(normalMap)
			root->AddData(Material::NORMAL_MAP_IDENTIFIER, std::make_shared<ds::Texture>(*dataSettings, fLoadTexture(*normalMap)));
#if 0
		// TODO
		+		["g_tMasks1"]	("g_tMasks1", "materials/models/courier/drodo/drodo_detailmask_tga_bcdebe17.vtex")	std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,std::basic_string<char,std::char_traits<char>,std::allocator<char> > >
		+		["g_tMasks2"]	("g_tMasks2", "materials/models/courier/drodo/drodo_specmask_tga_cff5ad3a.vtex")	std::pair<std::basic_string<char,std::char_traits<char>,std::allocator<char> > const ,std::basic_string<char,std::char_traits<char>,std::allocator<char> > >
#endif
	}
	else {
		auto *albedoMap = s2Mat->FindTextureParam("g_tColor");
		if(albedoMap == nullptr)
			albedoMap = s2Mat->FindTextureParam("g_tColor1");
		if(albedoMap == nullptr)
			albedoMap = s2Mat->FindTextureParam("g_tColor2");
		if(albedoMap)
			root->AddData(Material::ALBEDO_MAP_IDENTIFIER, std::make_shared<ds::Texture>(*dataSettings, fLoadTexture(*albedoMap)));

		auto *normalMap = s2Mat->FindTextureParam("g_tNormal");
		if(normalMap)
			root->AddData(Material::NORMAL_MAP_IDENTIFIER, std::make_shared<ds::Texture>(*dataSettings, fLoadTexture(*normalMap)));

		auto *emissionMap = s2Mat->FindTextureParam("g_tSelfIllumMask");
		if(emissionMap)
			root->AddData(Material::EMISSION_MAP_IDENTIFIER, std::make_shared<ds::Texture>(*dataSettings, fLoadTexture(*emissionMap)));

		auto *metalness = s2Mat->FindFloatParam("g_flMetalness");
		if(metalness)
			root->AddValue("float", "metalness_factor", std::to_string(*metalness));
		// TODO: Is there an attribute like this for roughness?
	}

	info.shader = shaderName;
	info.root = root;
	return InitializeVMatData(resource, *s2Mat, info, *root, *dataSettings, shaderName, origin);
}
#endif
