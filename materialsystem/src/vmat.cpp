#pragma optimize("",off)
#include "materialmanager.h"
#include <sharedutils/util_path.hpp>
#include <sharedutils/util_file.h>
#ifndef DISABLE_VMAT_SUPPORT
#include <util_source2.hpp>
#include <source2/resource.hpp>
#include <source2/resource_data.hpp>
#endif

#ifndef DISABLE_VMT_SUPPORT
bool MaterialManager::InitializeVMatData(source2::resource::Resource &resource,source2::resource::Material &vmat,LoadInfo &info,ds::Block &rootData,ds::Settings &settings,const std::string &shader) {return true;}

bool MaterialManager::LoadVMat(source2::resource::Resource &resource,LoadInfo &info)
{
	auto *s2Mat = dynamic_cast<source2::resource::Material*>(resource.FindBlock(source2::BlockType::DATA));
	if(s2Mat == nullptr)
		return false;
	auto dataSettings = CreateDataSettings();
	auto root = std::make_shared<ds::Block>(*dataSettings);

	std::string shaderName = "pbr";

	auto fAssignTexture = [this](const std::string &strPath) -> std::string {
		::util::Path path{strPath};
		path.RemoveFileExtension();
		path += ".vtex_c";

		std::string inputPath = path.GetString();
		auto front = path.GetFront();
		path.PopFront();

		auto outputPath = path;
		if(ustring::compare(front,"materials",false) == false)
			outputPath = "models/" +outputPath;

		auto &texLoader = GetTextureImporter();
		if(texLoader)
		{
			auto outputPathNoExt = outputPath.GetString();
			ufile::remove_extension_from_filename(outputPathNoExt);
			texLoader(inputPath,"materials/" +outputPathNoExt);
		}

		return outputPath.GetString();
	};

	auto *albedoMap = s2Mat->FindTextureParam("g_tColor");
	if(albedoMap == nullptr)
		albedoMap = s2Mat->FindTextureParam("g_tColor1");
	if(albedoMap == nullptr)
		albedoMap = s2Mat->FindTextureParam("g_tColor2");
	if(albedoMap)
		root->AddData(Material::ALBEDO_MAP_IDENTIFIER,std::make_shared<ds::Texture>(*dataSettings,fAssignTexture(*albedoMap)));

	auto *normalMap = s2Mat->FindTextureParam("g_tNormal");
	if(normalMap)
		root->AddData(Material::NORMAL_MAP_IDENTIFIER,std::make_shared<ds::Texture>(*dataSettings,fAssignTexture(*normalMap)));

	auto *metalnessMap = s2Mat->FindTextureParam("g_tMetalnessReflectance");
	if(metalnessMap)
		root->AddData(Material::METALNESS_MAP_IDENTIFIER,std::make_shared<ds::Texture>(*dataSettings,fAssignTexture(*metalnessMap)));

	auto *emissionMap = s2Mat->FindTextureParam("g_tSelfIllumMask");
	if(emissionMap)
		root->AddData(Material::EMISSION_MAP_IDENTIFIER,std::make_shared<ds::Texture>(*dataSettings,fAssignTexture(*emissionMap)));

	auto *aoMap = s2Mat->FindTextureParam("g_tAmbientOcclusion");
	if(aoMap == nullptr)
		aoMap = s2Mat->FindTextureParam("g_tOcclusion");
	if(aoMap)
		root->AddData(Material::AO_MAP_IDENTIFIER,std::make_shared<ds::Texture>(*dataSettings,fAssignTexture(*aoMap)));
	
	info.shader = shaderName;
	info.root = root;
	return InitializeVMatData(resource,*s2Mat,info,*root,*dataSettings,shaderName);
}
#endif
#pragma optimize("",on)
