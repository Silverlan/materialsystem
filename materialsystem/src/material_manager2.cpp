/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2021 Silverlan
 */

#include "impl_texture_formats.h"
#include "material_manager2.hpp"
#include <fsys/filesystem.h>
#include <fsys/ifile.hpp>

#pragma optimize("",off)
#include <udm.hpp>
bool msys::MaterialFormatHandler::LoadData(MaterialProcessor &processor,MaterialLoadInfo &info)
{
	std::shared_ptr<udm::Data> udmData = nullptr;
	try
	{
		udmData = udm::Data::Load(std::move(m_file));
	}
	catch(const udm::Exception &e)
	{
		return false;
	}
	if(udmData == nullptr)
		return false;
	auto udmDataRoot = udmData->GetAssetData().GetData();

	auto dataSettings = static_cast<MaterialManager&>(GetAssetManager()).CreateDataSettings();
	auto root = std::make_shared<ds::Block>(*dataSettings);

	std::function<void(const std::string &key,udm::LinkedPropertyWrapper &prop,ds::Block &block,bool texture)> udmToDataSys = nullptr;
	udmToDataSys = [&udmToDataSys,&dataSettings](const std::string &key,udm::LinkedPropertyWrapper &prop,ds::Block &block,bool texture) {
		prop.InitializeProperty();
		if(prop.prop)
		{
			switch(prop.prop->type)
			{
			case udm::Type::String:
				block.AddValue(texture ? "texture" : "string",key,prop.prop->ToValue<std::string>(""));
				break;
			case udm::Type::Int8:
			case udm::Type::UInt8:
			case udm::Type::Int16:
			case udm::Type::UInt16:
			case udm::Type::Int32:
			case udm::Type::UInt32:
			case udm::Type::Int64:
			case udm::Type::UInt64:
				block.AddValue("int",key,std::to_string(prop.prop->ToValue<int32_t>(0)));
				break;
			case udm::Type::Float:
			case udm::Type::Double:
				block.AddValue("float",key,std::to_string(prop.prop->ToValue<float>(0.f)));
				break;
			case udm::Type::Boolean:
				block.AddValue("bool",key,std::to_string(prop.prop->ToValue<bool>(false)));
				break;
			case udm::Type::Vector2:
			{
				auto v = prop.prop->ToValue<Vector2>(Vector2{});
				block.AddValue("vector2",key,std::to_string(v.x) +' ' +std::to_string(v.y));
				break;
			}
			case udm::Type::Vector3:
			{
				auto v = prop.prop->ToValue<Vector3>(Vector3{});
				block.AddValue("vector",key,std::to_string(v.x) +' ' +std::to_string(v.y) +' ' +std::to_string(v.z));
				break;
			}
			case udm::Type::Vector4:
			{
				auto v = prop.prop->ToValue<Vector4>(Vector4{});
				block.AddValue("vector4",key,std::to_string(v.x) +' ' +std::to_string(v.y) +' ' +std::to_string(v.z) +' ' +std::to_string(v.w));
				break;
			}
			case udm::Type::Element:
			{
				auto childBlock = block.AddBlock(key);
				for(auto udmChild : prop.ElIt())
					udmToDataSys(std::string{udmChild.key},udmChild.property,*childBlock,texture);
				break;
			}
			}
			static_assert(umath::to_integral(udm::Type::Count) == 36u);
		}
		prop.GetValuePtr<float>();
	};
	auto it = udmDataRoot.begin_el();
	if(it == udmDataRoot.end_el())
		return false;
	auto &firstEl = *it;

	auto udmTextures = firstEl.property["textures"];
	for(auto udmTex : udmTextures.ElIt())
		udmToDataSys(std::string{udmTex.key},udmTex.property,*root,true);
	
	auto udmProps = firstEl.property["properties"];
	for(auto udmProp : udmProps.ElIt())
		udmToDataSys(std::string{udmProp.key},udmProp.property,*root,false);

	data = root;
	shader = firstEl.key;
	return true;
}
msys::MaterialFormatHandler::MaterialFormatHandler(util::IAssetManager &assetManager)
	: util::IAssetFormatHandler{assetManager}
{}
		
///////////

std::unique_ptr<util::IAssetProcessor> msys::MaterialLoader::CreateAssetProcessor(
	const std::string &identifier,const std::string &ext,std::unique_ptr<util::IAssetFormatHandler> &&formatHandler
)
{
	auto processor = util::TAssetFormatLoader<MaterialProcessor>::CreateAssetProcessor(identifier,ext,std::move(formatHandler));
	auto &mdlProcessor = static_cast<MaterialProcessor&>(*processor);
	mdlProcessor.identifier = identifier;
	mdlProcessor.formatExtension = ext;
	return processor;
}

///////////

msys::MaterialLoadInfo::MaterialLoadInfo(util::AssetLoadFlags flags)
	: util::AssetLoadInfo{flags}
{}
msys::MaterialProcessor::MaterialProcessor(util::AssetFormatLoader &loader,std::unique_ptr<util::IAssetFormatHandler> &&handler)
	: util::FileAssetProcessor{loader,std::move(handler)}
{}
bool msys::MaterialProcessor::Load()
{
	auto &mdlHandler = static_cast<MaterialFormatHandler&>(*handler);
	auto r = mdlHandler.LoadData(*this,static_cast<MaterialLoadInfo&>(*loadInfo));
	if(!r)
		return false;
	auto mat = static_cast<MaterialManager&>(mdlHandler.GetAssetManager()).CreateMaterial(mdlHandler.shader,mdlHandler.data);
	mat->SetLoaded(true);
	material = mat;
	return true;
}
bool msys::MaterialProcessor::Finalize() {return true;}

msys::MaterialManager::MaterialManager()
{
	auto fileHandler = std::make_unique<util::AssetFileHandler>();
	fileHandler->open = [](const std::string &path,util::AssetFormatType formatType) -> std::unique_ptr<ufile::IFile> {
		auto openMode = filemanager::FileMode::Read;
		if(formatType == util::AssetFormatType::Binary)
			openMode |= filemanager::FileMode::Binary;
		auto f = filemanager::open_file(path,openMode);
		if(!f)
			return nullptr;
		return std::make_unique<fsys::File>(f);
	};
	fileHandler->exists = [](const std::string &path) -> bool {
		return filemanager::exists(path);
	};
	SetFileHandler(std::move(fileHandler));
	SetRootDirectory("materials");
	m_loader = std::make_unique<MaterialLoader>(*this);

	// TODO: New extensions might be added after the model manager has been created
	//for(auto &ext : get_model_extensions())
	//	RegisterFileExtension(ext);

	RegisterFormatHandler("pmat_b",[](util::IAssetManager &assetManager) -> std::unique_ptr<util::IAssetFormatHandler> {
		return std::make_unique<MaterialFormatHandler>(assetManager);
	});
	RegisterFormatHandler("pmat",[](util::IAssetManager &assetManager) -> std::unique_ptr<util::IAssetFormatHandler> {
		return std::make_unique<MaterialFormatHandler>(assetManager);
	},util::AssetFormatType::Text);
}
void msys::MaterialManager::SetErrorMaterial(Material *mat)
{
	if(mat == nullptr)
		m_error = nullptr;
	else
	{
		mat->SetErrorFlag(true);
		m_error = mat->GetHandle();
	}
}
Material *msys::MaterialManager::GetErrorMaterial() const {return m_error.get();}
void msys::MaterialManager::InitializeProcessor(util::IAssetProcessor &processor) {}
util::AssetObject msys::MaterialManager::InitializeAsset(const util::AssetLoadJob &job)
{
	auto &matProcessor = *static_cast<MaterialProcessor*>(job.processor.get());
	return matProcessor.material;
}
std::shared_ptr<ds::Settings> msys::MaterialManager::CreateDataSettings() const {return ds::create_data_settings({});}
std::shared_ptr<Material> msys::MaterialManager::CreateMaterial(const std::string &shader,const std::shared_ptr<ds::Block> &data)
{
	return Material::Create(*this,shader,data);
}
std::shared_ptr<Material> msys::MaterialManager::CreateMaterial(const std::string &identifier,const std::string &shader,const std::shared_ptr<ds::Block> &data)
{
	auto mat = CreateMaterial(shader,data);
	auto asset = std::make_shared<util::Asset>();
	asset->assetObject = mat;
	AddToCache(identifier,asset);
	return mat;
}
#pragma optimize("",on)
