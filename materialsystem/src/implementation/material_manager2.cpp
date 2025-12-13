// SPDX-FileCopyrightText: (c) 2021 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.materialsystem;

import :format_handlers;
import :material_manager2;

pragma::material::MaterialFormatHandler::MaterialFormatHandler(pragma::util::IAssetManager &assetManager) : IAssetFormatHandler {assetManager} {}

bool pragma::material::udm_to_data_block(udm::LinkedPropertyWrapper &udmDataRoot, datasystem::Block &root)
{
	std::function<void(const std::string &key, udm::LinkedPropertyWrapper &prop, datasystem::Block &block, bool texture)> udmToDataSys = nullptr;
	udmToDataSys = [&udmToDataSys](const std::string &key, udm::LinkedPropertyWrapper &prop, datasystem::Block &block, bool texture) {
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
						auto *dsTex = dynamic_cast<datasystem::Texture *>(dsVal.get());
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
			static_assert(pragma::math::to_integral(udm::Type::Count) == 36u);
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
bool pragma::material::PmatFormatHandler::LoadData(MaterialProcessor &processor, MaterialLoadInfo &info)
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
	auto root = pragma::util::make_shared<datasystem::Block>(*dataSettings);
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
pragma::material::PmatFormatHandler::PmatFormatHandler(pragma::util::IAssetManager &assetManager) : MaterialFormatHandler {assetManager} {}

///////////

pragma::material::WmiFormatHandler::WmiFormatHandler(pragma::util::IAssetManager &assetManager) : MaterialFormatHandler {assetManager} {}
bool pragma::material::WmiFormatHandler::LoadData(MaterialProcessor &processor, MaterialLoadInfo &info)
{
	auto root = datasystem::System::ReadData(*m_file, {});
	if(root == nullptr)
		return false;
	auto *data = root->GetData();
	std::string shader;
	std::shared_ptr<datasystem::Block> matData = nullptr;
	for(auto it = data->begin(); it != data->end(); it++) {
		auto &val = it->second;
		if(val->IsBlock()) // Shader has to be first block
		{
			matData = std::static_pointer_cast<datasystem::Block>(val);
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

std::unique_ptr<pragma::util::IAssetProcessor> pragma::material::MaterialLoader::CreateAssetProcessor(const std::string &identifier, const std::string &ext, std::unique_ptr<pragma::util::IAssetFormatHandler> &&formatHandler)
{
	auto processor = TAssetFormatLoader<MaterialProcessor>::CreateAssetProcessor(identifier, ext, std::move(formatHandler));
	auto &mdlProcessor = static_cast<MaterialProcessor &>(*processor);
	mdlProcessor.identifier = identifier;
	mdlProcessor.formatExtension = ext;
	return processor;
}

///////////

pragma::material::MaterialLoadInfo::MaterialLoadInfo(pragma::util::AssetLoadFlags flags) : AssetLoadInfo {flags} {}
pragma::material::MaterialProcessor::MaterialProcessor(pragma::util::AssetFormatLoader &loader, std::unique_ptr<pragma::util::IAssetFormatHandler> &&handler) : FileAssetProcessor {loader, std::move(handler)} {}
bool pragma::material::MaterialProcessor::Load()
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
bool pragma::material::MaterialProcessor::Finalize() { return true; }

std::shared_ptr<pragma::material::MaterialManager> pragma::material::MaterialManager::Create()
{
	auto matManager = std::shared_ptr<MaterialManager> {new MaterialManager {}};
	matManager->Initialize();
	return matManager;
}
pragma::material::MaterialManager::MaterialManager()
{
	auto fileHandler = std::make_unique<pragma::util::AssetFileHandler>();
	fileHandler->open = [](const std::string &path, pragma::util::AssetFormatType formatType) -> std::unique_ptr<ufile::IFile> {
		auto openMode = fs::FileMode::Read;
		openMode |= fs::FileMode::Binary;
		auto f = fs::open_file(path, openMode);
		if(!f)
			return nullptr;
		return std::make_unique<fs::File>(f);
	};
	fileHandler->exists = [](const std::string &path) -> bool { return fs::exists(path); };
	SetFileHandler(std::move(fileHandler));
	SetRootDirectory("materials");
	m_loader = std::make_unique<MaterialLoader>(*this);

	// TODO: New extensions might be added after the model manager has been created
	//for(auto &ext : get_model_extensions())
	//	RegisterFileExtension(ext);
}
void pragma::material::MaterialManager::Initialize()
{
	RegisterFormatHandler<PmatFormatHandler>("pmat_b");
	RegisterFormatHandler<PmatFormatHandler>("pmat", pragma::util::AssetFormatType::Text);
	RegisterFormatHandler<WmiFormatHandler>("wmi", pragma::util::AssetFormatType::Text);
	InitializeImportHandlers();
}
void pragma::material::MaterialManager::Reset()
{
	m_error = nullptr;
	TFileAssetManager<Material, MaterialLoadInfo>::Reset();
}
std::shared_ptr<pragma::material::Material> pragma::material::MaterialManager::ReloadAsset(const std::string &path, std::unique_ptr<MaterialLoadInfo> &&loadInfo, PreloadResult *optOutResult)
{
	auto *asset = FindCachedAsset(path);
	if(!asset) {
		ClearCachedResult(GetIdentifierHash(path));
		return LoadAsset(path, std::move(loadInfo), optOutResult);
	}
	auto matNew = LoadAsset(path, pragma::util::AssetLoadFlags::IgnoreCache | pragma::util::AssetLoadFlags::DontCache, optOutResult);
	if(!matNew)
		return nullptr;
	auto matOld = GetAssetObject(*asset);
	matOld->Assign(*matNew);
	OnAssetReloaded(path);
	return matOld;
}
pragma::util::AssetObject pragma::material::MaterialManager::ReloadAsset(const std::string &path, std::unique_ptr<pragma::util::AssetLoadInfo> &&loadInfo, PreloadResult *optOutResult)
{
	return ReloadAsset(path, pragma::util::static_unique_pointer_cast<pragma::util::AssetLoadInfo, MaterialLoadInfo>(std::move(loadInfo)), optOutResult);
}

static bool g_shouldUseVkvVmtParser = false;
void pragma::material::set_use_vkv_vmt_parser(bool useVkvParser) { g_shouldUseVkvVmtParser = useVkvParser; }
bool pragma::material::should_use_vkv_vmt_parser() { return g_shouldUseVkvVmtParser; }

void pragma::material::MaterialManager::InitializeImportHandlers()
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
void pragma::material::MaterialManager::SetErrorMaterial(Material *mat)
{
	if(mat == nullptr)
		m_error = nullptr;
	else {
		mat->SetErrorFlag(true);
		m_error = mat->GetHandle();
	}
}
pragma::material::Material *pragma::material::MaterialManager::GetErrorMaterial() const { return const_cast<Material *>(m_error.get()); }
void pragma::material::MaterialManager::InitializeProcessor(pragma::util::IAssetProcessor &processor) {}
pragma::util::AssetObject pragma::material::MaterialManager::InitializeAsset(const pragma::util::Asset &asset, const pragma::util::AssetLoadJob &job)
{
	auto &matProcessor = *static_cast<MaterialProcessor *>(job.processor.get());
	matProcessor.material->SetIndex(asset.index);
	return matProcessor.material;
}
std::shared_ptr<pragma::datasystem::Settings> pragma::material::MaterialManager::CreateDataSettings() const { return datasystem::create_data_settings({}); }
std::shared_ptr<pragma::material::Material> pragma::material::MaterialManager::CreateMaterialObject(const std::string &shader, const std::shared_ptr<datasystem::Block> &data) { return Material::Create(*this, shader, data); }
std::shared_ptr<pragma::material::Material> pragma::material::MaterialManager::CreateMaterial(const udm::AssetData &assetData, std::string &outErr)
{
	auto dataSettings = CreateDataSettings();
	auto root = pragma::util::make_shared<datasystem::Block>(*dataSettings);
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
	auto asset = std::make_shared<pragma::util::Asset>();
	asset->assetObject = mat;
	auto index = AddToIndex(asset);
	mat->SetIndex(index);
	mat->SetLoaded(true);
	if(!baseMaterial.empty())
		mat->SetBaseMaterial(baseMaterial);
	return mat;
}
std::shared_ptr<pragma::material::Material> pragma::material::MaterialManager::CreateMaterial(const std::string &shader, const std::shared_ptr<datasystem::Block> &data)
{
	auto mat = CreateMaterialObject(shader, data);
	auto asset = std::make_shared<pragma::util::Asset>();
	asset->assetObject = mat;
	auto index = AddToIndex(asset);
	mat->SetIndex(index);
	mat->SetLoaded(true);
	return mat;
}
std::shared_ptr<pragma::material::Material> pragma::material::MaterialManager::CreateMaterial(const std::string &identifier, const std::string &shader, const std::shared_ptr<datasystem::Block> &data)
{
	auto tmpMat = CreateMaterialObject(shader, data);
	std::shared_ptr<Material> mat = nullptr;
	auto *asset = FindCachedAsset(identifier);
	if(asset) {
		mat = GetAssetObject(*asset);
		mat->Assign(*tmpMat);
	}
	else {
		auto asset = std::make_shared<pragma::util::Asset>();
		asset->assetObject = tmpMat;
		auto idx = AddToCache(identifier, asset);
		mat = tmpMat;
		mat->SetIndex(idx);
		mat->SetLoaded(true);
		mat->SetName(ToCacheIdentifier(identifier));
	}
	return mat;
}
