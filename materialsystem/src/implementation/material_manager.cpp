// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <array>
#ifndef DISABLE_VMT_SUPPORT
#include <VMTFile.h>
#include <VTFLib.h>
#endif
#include <string>
#include <memory>
#include <iostream>

module pragma.materialsystem;

import :material_manager;

#undef CreateFile

static const std::unordered_map<std::string, std::string> ENUM_VARS = {
  // These have to correspond with their respective vulkan enum values!
  {"SAMPLER_ADDRESS_MODE_REPEAT", "0"},
  {"SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT", "1"},
  {"SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE", "2"},
  {"SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER", "3"},
  {"SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE", "4"},

  {"MIPMAP_MODE_IGNORE", "-1"},
  {"MIPMAP_MODE_LOAD_OR_GENERATE", "0"},
  {"MIPMAP_MODE_LOAD", "1"},
  {"MIPMAP_MODE_GENERATE", "2"},

  {"BORDER_COLOR_FLOAT_TRANSPARENT_BLACK", "0"},
  {"BORDER_COLOR_INT_TRANSPARENT_BLACK", "1"},
  {"BORDER_COLOR_FLOAT_OPAQUE_BLACK", "2"},
  {"BORDER_COLOR_INT_OPAQUE_BLACK", "3"},
  {"BORDER_COLOR_FLOAT_OPAQUE_WHITE", "4"},
  {"BORDER_COLOR_INT_OPAQUE_WHITE", "5"},

  {"ALPHA_MODE_OPAQUE", std::to_string(umath::to_integral(AlphaMode::Opaque))},
  {"ALPHA_MODE_MASK", std::to_string(umath::to_integral(AlphaMode::Mask))},
  {"ALPHA_MODE_BLEND", std::to_string(umath::to_integral(AlphaMode::Blend))},
};

MaterialManager::LoadInfo::LoadInfo() : material(nullptr) {}

static std::string g_materialLocation = "materials";
std::optional<std::string> MaterialManager::FindMaterialPath(const std::string &material)
{
	std::string ext;
	auto path = PathToIdentifier(material, &ext);
	//path += '.' +ext;
	if(FileManager::Exists(g_materialLocation + '/' + path) == false)
		return {};
	return path;
}

MaterialManager::MaterialManager() : m_error {} {}
MaterialManager::~MaterialManager() { Clear(); }

void MaterialManager::SetRootMaterialLocation(const std::string &location) { g_materialLocation = location; }
const std::string &MaterialManager::GetRootMaterialLocation() { return g_materialLocation; }

msys::Material *MaterialManager::CreateMaterial(const std::string &shader, const std::shared_ptr<ds::Block> &root) { return CreateMaterial(nullptr, shader, root); }
msys::Material *MaterialManager::CreateMaterial(const std::string &identifier, const std::string &shader, const std::shared_ptr<ds::Block> &root) { return CreateMaterial(&identifier, shader, root); }
msys::Material *MaterialManager::CreateMaterial(const std::string *identifier, const std::string &shader, std::shared_ptr<ds::Block> root)
{
	std::string matId;
	if(identifier != nullptr) {
		matId = *identifier;
		ustring::to_lower(matId);
		auto *mat = FindMaterial(matId);
		if(mat != nullptr)
			return mat;
	}
	else
		matId = "__anonymous" + std::to_string(m_unnamedIdx++);
	if(root == nullptr) {
		auto dataSettings = ds::create_data_settings(ENUM_VARS);
		root = std::make_shared<ds::Block>(*dataSettings);
	}
	msys::Material *mat; //auto *mat = CreateMaterial<Material>(shader,root); // Claims ownership of 'root' and frees the memory at destruction
	mat->SetName(matId);
	AddMaterial(matId, *mat);
	return mat;
}

void MaterialManager::AddMaterial(const std::string &identifier, msys::Material &mat)
{
	auto nidentifier = ToMaterialIdentifier(identifier);
	auto it = m_nameToMaterialIndex.find(nidentifier);
	if(it != m_nameToMaterialIndex.end()) {
		///m_materials[it->second] = mat.GetHandle();
		//mat.SetIndex(it->second);
		return;
	}
	//mat.SetIndex(m_materials.size());
	if(m_materials.size() == m_materials.capacity())
		m_materials.reserve(m_materials.size() * 1.1 + 100);
	//m_materials.push_back(mat.GetHandle());
	m_nameToMaterialIndex[nidentifier] = mat.GetIndex();
}

extern const std::array<std::string, 5> g_knownMaterialFormats = {msys::Material::FORMAT_MATERIAL_BINARY, msys::Material::FORMAT_MATERIAL_ASCII, "wmi", "vmat_c", "vmt"};
std::string MaterialManager::PathToIdentifier(const std::string &path, std::string *ext, bool &hadExtension) const
{
	auto matPath = FileManager::GetNormalizedPath(path);
	std::string fext;
	auto hasExt = ufile::get_extension(matPath, &fext);
	if(hasExt) {
		auto lext = fext;
		ustring::to_lower(lext);
		auto it = std::find(g_knownMaterialFormats.begin(), g_knownMaterialFormats.end(), lext);
		if(it == g_knownMaterialFormats.end())
			hasExt = false; // Assume that it's part of the filename
	}
	if(hasExt == false) {
		hadExtension = false;
		for(auto &exts : g_knownMaterialFormats) {
			*ext = exts;
			if(FileManager::Exists(g_materialLocation + "\\" + matPath + '.' + *ext))
				break;
		}
		matPath += '.' + *ext;
	}
	else
		hadExtension = true;
	return matPath;
}

std::string MaterialManager::PathToIdentifier(const std::string &path, std::string *ext) const
{
	auto hadExtension = false;
	return PathToIdentifier(path, ext, hadExtension);
}
std::string MaterialManager::PathToIdentifier(const std::string &path) const
{
	std::string ext;
	return PathToIdentifier(path, &ext);
}

msys::Material *MaterialManager::FindMaterial(const std::string &identifier, std::string &internalMatId) const
{
	internalMatId = PathToIdentifier(identifier);
	auto it = m_nameToMaterialIndex.find(ToMaterialIdentifier(internalMatId));
	if(it == m_nameToMaterialIndex.end())
		return nullptr;
	return const_cast<msys::Material*>(m_materials.at(it->second).get());
}
msys::Material *MaterialManager::FindMaterial(const std::string &identifier) const
{
	std::string internalMatId;
	return FindMaterial(identifier, internalMatId);
}
msys::Material *MaterialManager::GetMaterial(msys::MaterialIndex index) { return (index < m_materials.size()) ? m_materials.at(index).get() : nullptr; }
const msys::Material *MaterialManager::GetMaterial(msys::MaterialIndex index) const { return const_cast<MaterialManager *>(this)->GetMaterial(index); }
std::shared_ptr<ds::Settings> MaterialManager::CreateDataSettings() const { return ds::create_data_settings(ENUM_VARS); }

std::string MaterialManager::ToMaterialIdentifier(const std::string &id) const
{
	auto path = util::Path::CreateFile(id);
	path.RemoveFileExtension(g_knownMaterialFormats);
	auto identifier = path.GetString();
	ustring::to_lower(identifier);
	return identifier;
}

bool MaterialManager::Load(const std::string &path, LoadInfo &info, bool bReload)
{
	std::string ext;
	auto &matId = info.identifier;
	auto bHadExtension = false;
	matId = FileManager::GetNormalizedPath(path);
	auto it = m_nameToMaterialIndex.find(ToMaterialIdentifier(matId));
	if(it != m_nameToMaterialIndex.end()) {
		auto &hMat = m_materials.at(it->second);
		if(hMat) {
			info.material = hMat.get();
			info.shader = info.material->GetShaderIdentifier();
			if(bReload == false)
				return true;
		}
	}
	matId = PathToIdentifier(matId, &ext, bHadExtension);
	std::string absPath = g_materialLocation + "\\";
	absPath += path;
	if(bHadExtension == false)
		absPath += '.' + ext;
	auto sub = matId;
	std::string openMode = "r";
#ifndef DISABLE_VMAT_SUPPORT
	if(ustring::compare<std::string>(ext, "vmat_c", false))
		openMode = "rb";
#endif
	auto fp = FileManager::OpenFile(absPath.c_str(), openMode.c_str());
	if(fp == nullptr)
		return false;
	auto handleLoadError = [this](const std::string &matId, msys::Material **mat) -> bool {
		//auto matErr = m_materials.find("error");
		//if(matErr == m_materials.end())
		//	return false;
		//auto *nmat = CreateMaterial<Material>();
		//AddMaterial(matId,*nmat);
		//*mat = nmat;
		*mat = GetErrorMaterial();
		return true;
	};
#ifndef DISABLE_VMT_SUPPORT
	if(ext == "vmt") // Load from vmt-file
	{
		auto sz = fp->GetSize();
		std::vector<uint8_t> data(sz);
		fp->Read(data.data(), sz);
		fp = nullptr;
		while(sz > 0 && data[sz - 1] == '\0')
			--sz;
		VTFLib::CVMTFile vmt {};
		if(vmt.Load(data.data(), static_cast<vlUInt>(sz)) == vlTrue)
			return LoadVMT(vmt, info);
		auto *err = vlGetLastError();
		// TODO: Don't print this out directly, add an error handler!
		std::cout << "VMT Parsing error in material '" << path << "': " << err << std::endl;
		return false;
	}
#endif
#ifndef DISABLE_VMAT_SUPPORT
	if(ustring::compare<std::string>(ext, "vmat_c", false)) {
		fsys::File f {fp};
		auto resource = source2::load_resource(f);
		return resource ? LoadVMat(*resource, info) : false;
	}
#endif

	if(ext.empty() || ustring::compare<std::string>(ext, msys::Material::FORMAT_MATERIAL_ASCII, false) || ustring::compare<std::string>(ext, msys::Material::FORMAT_MATERIAL_BINARY, false))
		return LoadUdm(fp, info);

	fsys::File f {fp};
	auto root = ds::System::ReadData(f, ENUM_VARS);
	fp.reset();
	if(root == nullptr)
		return handleLoadError(matId, &info.material);
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
		return handleLoadError(matId, &info.material);
	root->DetachData(*matData);

	info.shader = shader;
	info.root = matData;
	return true;
}
msys::Material *MaterialManager::Load(const std::string &path, bool bReload, bool loadInstantly, bool *bFirstTimeError)
{
	if(bFirstTimeError != nullptr)
		*bFirstTimeError = false;
	LoadInfo info {};
	if(Load(path, info, bReload) == false) {
		if(info.material != nullptr) // 'bReload' was set true, material already existed and reloading failed
			return info.material;
		info.material = GetErrorMaterial();
		if(info.material == nullptr)
			return nullptr;
		if(bFirstTimeError != nullptr)
			*bFirstTimeError = true;
		//info.material = info.material->Copy(); // Copy error material
	}
	else if(info.material != nullptr) {
		info.material->SetLoaded(true);
		if(bReload)
			info.material->Initialize(info.shader, info.root);
		return info.material;
	}
	else {
		//info.material = new Material{*this,info.shader,info.root};
		info.material->SetLoaded(true);
	}
	info.material->SetName(info.identifier);
	AddMaterial(info.identifier, *info.material);
	if(info.saveOnDisk) {
		std::string err;
		info.material->Save("addons/converted/materials/" + info.material->GetName(), err, true);
	}
	return info.material;
}
bool MaterialManager::LoadUdm(std::shared_ptr<VFilePtrInternal> &f, LoadInfo &loadInfo)
{
	std::shared_ptr<udm::Data> udmData = nullptr;
	try {
		udmData = udm::Data::Load(f);
	}
	catch(const udm::Exception &e) {
		return false;
	}
	if(udmData == nullptr)
		return false;
	auto data = udmData->GetAssetData().GetData();

	auto dataSettings = CreateDataSettings();
	auto root = std::make_shared<ds::Block>(*dataSettings);

	std::function<void(const std::string &key, udm::LinkedPropertyWrapper &prop, ds::Block &block, bool texture)> udmToDataSys = nullptr;
	udmToDataSys = [&udmToDataSys, &dataSettings](const std::string &key, udm::LinkedPropertyWrapper &prop, ds::Block &block, bool texture) {
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
					auto childBlock = block.AddBlock(key);
					for(auto udmChild : prop.ElIt())
						udmToDataSys(std::string {udmChild.key}, udmChild.property, *childBlock, texture);
					break;
				}
			}
			static_assert(umath::to_integral(udm::Type::Count) == 36u);
		}
		prop.GetValuePtr<float>();
	};
	auto it = data.begin_el();
	if(it == data.end_el())
		return false;
	auto &firstEl = *it;

	auto udmTextures = firstEl.property["textures"];
	for(auto udmTex : udmTextures.ElIt())
		udmToDataSys(std::string {udmTex.key}, udmTex.property, *root, true);

	auto udmProps = firstEl.property["properties"];
	for(auto udmProp : udmProps.ElIt())
		udmToDataSys(std::string {udmProp.key}, udmProp.property, *root, false);

	loadInfo.shader = firstEl.key;
	loadInfo.root = root;
	return true;
}
void MaterialManager::SetErrorMaterial(msys::Material *mat)
{
	if(mat == nullptr)
		m_error = msys::MaterialHandle {};
	else {
		mat->SetErrorFlag(true);
		//m_error = mat->GetHandle();
	}
}
const std::vector<MaterialManager::ImageFormat> &MaterialManager::get_supported_image_formats()
{
	static std::vector<ImageFormat> s_supportedImageFormats = {
		// Order of preference
		{msys::TextureType::KTX, "ktx"},
		{msys::TextureType::DDS, "dds"},
		{msys::TextureType::PNG, "png"},
		{msys::TextureType::TGA, "tga"},
		{msys::TextureType::JPG, "jpg"},
		{msys::TextureType::BMP, "bmp"},
		{msys::TextureType::PSD, "psd"},
		{msys::TextureType::GIF, "gif"},
		{msys::TextureType::HDR, "hdr"},
		{msys::TextureType::PIC, "pic"},
  #ifndef DISABLE_VTF_SUPPORT
		{msys::TextureType::VTF, "vtf"},
  #endif
  #ifndef DISABLE_VTEX_SUPPORT
		{msys::TextureType::VTex, "vtex_c"},
  #endif
	  };
	static_assert(umath::to_integral(msys::TextureType::Count) == 13, "Update this implementation when new texture types have been added!");
	return s_supportedImageFormats;
}
msys::Material *MaterialManager::GetErrorMaterial() const { return const_cast<msys::Material*>(m_error.get()); }
const std::vector<msys::MaterialHandle> &MaterialManager::GetMaterials() const { return m_materials; }
uint32_t MaterialManager::Clear()
{
	for(auto &hMaterial : m_materials) {
		//if(hMaterial.IsValid())
		//	hMaterial->Remove();
	}
	auto n = m_materials.size();
	m_materials.clear();
	m_nameToMaterialIndex.clear();
	return n;
}
void MaterialManager::SetTextureImporter(const std::function<VFilePtr(const std::string &, const std::string &)> &fileHandler) { m_textureImporter = fileHandler; }
const std::function<std::shared_ptr<VFilePtrInternal>(const std::string &, const std::string &)> &MaterialManager::GetTextureImporter() const { return m_textureImporter; }
uint32_t MaterialManager::ClearUnused()
{
	uint32_t n = 0;
	for(auto it = m_materials.begin(); it != m_materials.end();) {
		auto &hMaterial = *it;
		// Note: If a material has a use count of 2 (i.e. 2 handles), it means it's not actually being used, since the material itself
		// has one handle, and the material manager has one as well.
		if(hMaterial && hMaterial.use_count() <= 2 && hMaterial.get() != m_error.get()) {
			auto matIdx = hMaterial->GetIndex();
			/*for(auto it=m_nameToMaterialIndex.begin();it!=m_nameToMaterialIndex.end();++it)
			{
				if(it->second != matIdx)
					continue;
				m_nameToMaterialIndex.erase(it);
				break;
			}*/
			//	hMaterial->Remove();
			//it = m_materials.erase(it);
			*it = {};
			++it;
			++n;
		}
		else
			++it;
	}
	return n;
}
