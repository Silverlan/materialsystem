// SPDX-FileCopyrightText: (c) 2021 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.materialsystem;

import :format_handlers;
import :material_manager2;

msys::MaterialFormatHandler::MaterialFormatHandler(util::IAssetManager &assetManager) : util::IAssetFormatHandler {assetManager} {}

bool msys::udm_to_data_block(udm::LinkedPropertyWrapper &udmDataRoot, ds::Block &root)
{
	std::function<void(const std::string &key, udm::LinkedPropertyWrapper &prop, ds::Block &block, bool texture)> udmToDataSys = nullptr;
	udmToDataSys = [&udmToDataSys](const std::string &key, udm::LinkedPropertyWrapper &prop, ds::Block &block, bool texture) {
		prop.InitializeProperty();
		if(prop.prop) {
			switch(prop.prop->type) {
			case udm::Type::String:
				block.AddValue(texture ? "texture" : "string", key, prop.prop->ToValue<std::string>(""));
				break;
			case udm::Type::Int8:
			case udm::Type::UInt8:
			case udm::Type::Int16:
			case udm::Type::UInt16:
			case udm::Type::Int32:
			case udm::Type::UInt32:
			case udm::Type::Int64:
			case udm::Type::UInt64:
				block.AddValue("int", key, std::to_string(prop.prop->ToValue<int32_t>(0)));
				break;
			case udm::Type::Float:
			case udm::Type::Double:
				block.AddValue("float", key, std::to_string(prop.prop->ToValue<float>(0.f)));
				break;
			case udm::Type::Boolean:
				block.AddValue("bool", key, std::to_string(prop.prop->ToValue<bool>(false)));
				break;
			case udm::Type::Vector2:
				{
					auto v = prop.prop->ToValue<Vector2>(Vector2 {});
					block.AddValue("vector2", key, std::to_string(v.x) + ' ' + std::to_string(v.y));
					break;
				}
			case udm::Type::Vector3:
				{
					auto v = prop.prop->ToValue<Vector3>(Vector3 {});
					block.AddValue("vector", key, std::to_string(v.x) + ' ' + std::to_string(v.y) + ' ' + std::to_string(v.z));
					break;
				}
			case udm::Type::Vector4:
				{
					auto v = prop.prop->ToValue<Vector4>(Vector4 {});
					block.AddValue("vector4", key, std::to_string(v.x) + ' ' + std::to_string(v.y) + ' ' + std::to_string(v.z) + ' ' + std::to_string(v.w));
					break;
				}
			case udm::Type::Element:
				{
					if(texture) {
						auto texPath = prop["texture"]->ToValue<std::string>("");
						auto dsVal = block.AddValue("texture", key, texPath);
						auto *dsTex = dynamic_cast<ds::Texture *>(dsVal.get());
						if(dsTex)
							dsTex->GetValue().userData = prop->Copy(true);
					}
					else {
						auto childBlock = block.AddBlock(key);
						for(auto udmChild : prop.ElIt())
							udmToDataSys(std::string {udmChild.key}, udmChild.property, *childBlock, texture);
					}
					break;
				}
			}
			static_assert(umath::to_integral(udm::Type::Count) == 36u);
		}
	};
	auto udmTextures = udmDataRoot["textures"];
	for(auto udmTex : udmTextures.ElIt())
		udmToDataSys(std::string {udmTex.key}, udmTex.property, root, true);

	auto udmProps = udmDataRoot["properties"];
	for(auto udmProp : udmProps.ElIt())
		udmToDataSys(std::string {udmProp.key}, udmProp.property, root, false);
	return true;
}
bool msys::PmatFormatHandler::LoadData(MaterialProcessor &processor, MaterialLoadInfo &info)
{
	std::shared_ptr<udm::Data> udmData = nullptr;
	try {
		udmData = udm::Data::Load(std::move(m_file));
	}
	catch(const udm::Exception &e) {
		return false;
	}
	if(udmData == nullptr)
		return false;
	auto udmDataRoot = udmData->GetAssetData().GetData();

	auto dataSettings = static_cast<MaterialManager &>(GetAssetManager()).CreateDataSettings();
	auto root = util::make_shared<ds::Block>(*dataSettings);
	auto it = udmDataRoot.begin_el();
	if(it == udmDataRoot.end_el())
		return false;
	auto &firstEl = *it;
	shader = firstEl.key;
	if(!udm_to_data_block(firstEl.property, *root))
		return false;
	firstEl.property["base_material"](baseMaterial);
	data = root;
	return true;
}
msys::PmatFormatHandler::PmatFormatHandler(util::IAssetManager &assetManager) : MaterialFormatHandler {assetManager} {}

///////////

msys::WmiFormatHandler::WmiFormatHandler(util::IAssetManager &assetManager) : MaterialFormatHandler {assetManager} {}
bool msys::WmiFormatHandler::LoadData(MaterialProcessor &processor, MaterialLoadInfo &info)
{
	auto root = ds::System::ReadData(*m_file, {});
	if(root == nullptr)
		return false;
	auto *data = root->GetData();
	std::string shader;
	std::shared_ptr<ds::Block> matData = nullptr;
	for(auto it = data->begin(); it != data->end(); it++) {
		auto &val = it->second;
		if(val->IsBlock()) // Shader has to be first block
		{
			matData = std::static_pointer_cast<ds::Block>(val);
			shader = it->first;
			break;
		}
	}
	if(matData == nullptr)
		return false;
	root->DetachData(*matData);

	this->data = matData;
	this->shader = shader;
	return true;
}

///////////

std::unique_ptr<util::IAssetProcessor> msys::MaterialLoader::CreateAssetProcessor(const std::string &identifier, const std::string &ext, std::unique_ptr<util::IAssetFormatHandler> &&formatHandler)
{
	auto processor = util::TAssetFormatLoader<MaterialProcessor>::CreateAssetProcessor(identifier, ext, std::move(formatHandler));
	auto &mdlProcessor = static_cast<MaterialProcessor &>(*processor);
	mdlProcessor.identifier = identifier;
	mdlProcessor.formatExtension = ext;
	return processor;
}

///////////

msys::MaterialLoadInfo::MaterialLoadInfo(util::AssetLoadFlags flags) : util::AssetLoadInfo {flags} {}
msys::MaterialProcessor::MaterialProcessor(util::AssetFormatLoader &loader, std::unique_ptr<util::IAssetFormatHandler> &&handler) : util::FileAssetProcessor {loader, std::move(handler)} {}
bool msys::MaterialProcessor::Load()
{
	auto &matHandler = static_cast<MaterialFormatHandler &>(*handler);
	auto r = matHandler.LoadData(*this, static_cast<MaterialLoadInfo &>(*loadInfo));
	if(!r)
		return false;
	auto mat = static_cast<MaterialManager &>(matHandler.GetAssetManager()).CreateMaterialObject(matHandler.shader, matHandler.data);
	mat->SetLoaded(true);
	mat->SetName(identifier);
	if(!matHandler.baseMaterial.empty())
		mat->SetBaseMaterial(matHandler.baseMaterial);
	material = mat;
	return true;
}
bool msys::MaterialProcessor::Finalize() { return true; }

std::shared_ptr<msys::MaterialManager> msys::MaterialManager::Create()
{
	auto matManager = std::shared_ptr<MaterialManager> {new MaterialManager {}};
	matManager->Initialize();
	return matManager;
}
msys::MaterialManager::MaterialManager()
{
	auto fileHandler = std::make_unique<util::AssetFileHandler>();
	fileHandler->open = [](const std::string &path, util::AssetFormatType formatType) -> std::unique_ptr<ufile::IFile> {
		auto openMode = filemanager::FileMode::Read;
		openMode |= filemanager::FileMode::Binary;
		auto f = filemanager::open_file(path, openMode);
		if(!f)
			return nullptr;
		return std::make_unique<fsys::File>(f);
	};
	fileHandler->exists = [](const std::string &path) -> bool { return filemanager::exists(path); };
	SetFileHandler(std::move(fileHandler));
	SetRootDirectory("materials");
	m_loader = std::make_unique<MaterialLoader>(*this);

	// TODO: New extensions might be added after the model manager has been created
	//for(auto &ext : get_model_extensions())
	//	RegisterFileExtension(ext);
}
void msys::MaterialManager::Initialize()
{
	RegisterFormatHandler<PmatFormatHandler>("pmat_b");
	RegisterFormatHandler<PmatFormatHandler>("pmat", util::AssetFormatType::Text);
	RegisterFormatHandler<WmiFormatHandler>("wmi", util::AssetFormatType::Text);
	InitializeImportHandlers();
}
void msys::MaterialManager::Reset()
{
	m_error = nullptr;
	util::TFileAssetManager<Material, MaterialLoadInfo>::Reset();
}
std::shared_ptr<msys::Material> msys::MaterialManager::ReloadAsset(const std::string &path, std::unique_ptr<MaterialLoadInfo> &&loadInfo, PreloadResult *optOutResult)
{
	auto *asset = FindCachedAsset(path);
	if(!asset) {
		ClearCachedResult(GetIdentifierHash(path));
		return LoadAsset(path, std::move(loadInfo), optOutResult);
	}
	auto matNew = LoadAsset(path, util::AssetLoadFlags::IgnoreCache | util::AssetLoadFlags::DontCache, optOutResult);
	if(!matNew)
		return nullptr;
	auto matOld = GetAssetObject(*asset);
	matOld->Assign(*matNew);
	OnAssetReloaded(path);
	return matOld;
}
util::AssetObject msys::MaterialManager::ReloadAsset(const std::string &path, std::unique_ptr<util::AssetLoadInfo> &&loadInfo, PreloadResult *optOutResult)
{
	return ReloadAsset(path, util::static_unique_pointer_cast<util::AssetLoadInfo, MaterialLoadInfo>(std::move(loadInfo)), optOutResult);
}

static bool g_shouldUseVkvVmtParser = false;
void msys::set_use_vkv_vmt_parser(bool useVkvParser) { g_shouldUseVkvVmtParser = useVkvParser; }
bool msys::should_use_vkv_vmt_parser() { return g_shouldUseVkvVmtParser; }

void msys::MaterialManager::InitializeImportHandlers()
{
#ifndef DISABLE_VMT_SUPPORT
	if(should_use_vkv_vmt_parser())
		RegisterImportHandler<SourceVmtFormatHandler2>("vmt");
	else
		RegisterImportHandler<SourceVmtFormatHandler>("vmt");
#endif
#ifndef DISABLE_VMAT_SUPPORT
	RegisterImportHandler<Source2VmatFormatHandler>("vmat_c");
#endif
}
void msys::MaterialManager::SetErrorMaterial(Material *mat)
{
	if(mat == nullptr)
		m_error = nullptr;
	else {
		mat->SetErrorFlag(true);
		m_error = mat->GetHandle();
	}
}
msys::Material *msys::MaterialManager::GetErrorMaterial() const { return const_cast<Material *>(m_error.get()); }
void msys::MaterialManager::InitializeProcessor(util::IAssetProcessor &processor) {}
util::AssetObject msys::MaterialManager::InitializeAsset(const util::Asset &asset, const util::AssetLoadJob &job)
{
	auto &matProcessor = *static_cast<MaterialProcessor *>(job.processor.get());
	matProcessor.material->SetIndex(asset.index);
	return matProcessor.material;
}
std::shared_ptr<ds::Settings> msys::MaterialManager::CreateDataSettings() const { return ds::create_data_settings({}); }
std::shared_ptr<msys::Material> msys::MaterialManager::CreateMaterialObject(const std::string &shader, const std::shared_ptr<ds::Block> &data) { return Material::Create(*this, shader, data); }
std::shared_ptr<msys::Material> msys::MaterialManager::CreateMaterial(const udm::AssetData &assetData, std::string &outErr)
{
	auto dataSettings = CreateDataSettings();
	auto root = util::make_shared<ds::Block>(*dataSettings);
	auto data = assetData.GetData();
	auto it = data.begin_el();
	if(it == data.end_el()) {
		outErr = "Empty material data";
		return nullptr;
	}
	auto &firstEl = *it;
	auto shader = firstEl.key;
	if(!udm_to_data_block(firstEl.property, *root)) {
		outErr = "Failed to convert material data";
		return nullptr;
	}
	std::string baseMaterial;
	firstEl.property["base_material"](baseMaterial);
	auto mat = CreateMaterialObject(std::string {shader}, root);
	auto asset = std::make_shared<util::Asset>();
	asset->assetObject = mat;
	auto index = AddToIndex(asset);
	mat->SetIndex(index);
	mat->SetLoaded(true);
	if(!baseMaterial.empty())
		mat->SetBaseMaterial(baseMaterial);
	return mat;
}
std::shared_ptr<msys::Material> msys::MaterialManager::CreateMaterial(const std::string &shader, const std::shared_ptr<ds::Block> &data)
{
	auto mat = CreateMaterialObject(shader, data);
	auto asset = std::make_shared<util::Asset>();
	asset->assetObject = mat;
	auto index = AddToIndex(asset);
	mat->SetIndex(index);
	mat->SetLoaded(true);
	return mat;
}
std::shared_ptr<msys::Material> msys::MaterialManager::CreateMaterial(const std::string &identifier, const std::string &shader, const std::shared_ptr<ds::Block> &data)
{
	auto tmpMat = CreateMaterialObject(shader, data);
	std::shared_ptr<Material> mat = nullptr;
	auto *asset = FindCachedAsset(identifier);
	if(asset) {
		mat = GetAssetObject(*asset);
		mat->Assign(*tmpMat);
	}
	else {
		auto asset = std::make_shared<util::Asset>();
		asset->assetObject = tmpMat;
		auto idx = AddToCache(identifier, asset);
		mat = tmpMat;
		mat->SetIndex(idx);
		mat->SetLoaded(true);
		mat->SetName(ToCacheIdentifier(identifier));
	}
	return mat;
}
