/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "materialmanager.h"
#include "textureinfo.h"
#include <sharedutils/alpha_mode.hpp>
#include <fsys/filesystem.h>
#include <sharedutils/util_string.h>
#include <sharedutils/util_file.h>
#include <sharedutils/util.h>
#include <array>
#ifndef DISABLE_VMT_SUPPORT
#include <VMTFile.h>
#include <VTFLib.h>
#include "util_vmt.hpp"
#endif
#ifndef DISABLE_VMAT_SUPPORT
#include <util_source2.hpp>
#include <source2/resource.hpp>
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
	{"BORDER_COLOR_INT_OPAQUE_WHITE","5"},

	{"ALPHA_MODE_OPAQUE",std::to_string(umath::to_integral(AlphaMode::Opaque))},
	{"ALPHA_MODE_MASK",std::to_string(umath::to_integral(AlphaMode::Mask))},
	{"ALPHA_MODE_BLEND",std::to_string(umath::to_integral(AlphaMode::Blend))}
};

MaterialManager::LoadInfo::LoadInfo()
	: material(nullptr)
{}

static std::string g_materialLocation = "materials";
std::optional<std::string> MaterialManager::FindMaterialPath(const std::string &material)
{
	std::string ext;
	auto path = PathToIdentifier(material,&ext);
	//path += '.' +ext;
	if(FileManager::Exists(g_materialLocation +'/' +path) == false)
		return {};
	return path;
}

MaterialManager::MaterialManager()
	: m_error{}
{}
MaterialManager::~MaterialManager()
{
	Clear();
}

void MaterialManager::SetRootMaterialLocation(const std::string &location)
{
	g_materialLocation = location;
}
const std::string &MaterialManager::GetRootMaterialLocation() {return g_materialLocation;}

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
	AddMaterial(matId,*mat);
	return mat;
}

void MaterialManager::AddMaterial(const std::string &identifier,Material &mat)
{
	mat.SetIndex(m_materials.size());
	if(m_materials.size() == m_materials.capacity())
		m_materials.reserve(m_materials.size() *1.1 +100);
	m_materials.push_back(mat.GetHandle());
	m_nameToMaterialIndex[ToMaterialIdentifier(identifier)] = mat.GetIndex();
}

extern const std::array<std::string,3> g_knownMaterialFormats = {"wmi","vmat_c","vmt"};
std::string MaterialManager::PathToIdentifier(const std::string &path,std::string *ext,bool &hadExtension) const
{
	auto matPath = FileManager::GetNormalizedPath(path);
	std::string fext;
	auto hasExt = ufile::get_extension(matPath,&fext);
	if(hasExt)
	{
		auto lext = fext;
		ustring::to_lower(lext);
		auto it = std::find(g_knownMaterialFormats.begin(),g_knownMaterialFormats.end(),lext);
		if(it == g_knownMaterialFormats.end())
			hasExt = false; // Assume that it's part of the filename
	}
	if(hasExt == false)
	{
		hadExtension = false;
		*ext = "wmi";
#ifndef DISABLE_VAT_SUPPORT
		if(!FileManager::Exists(g_materialLocation +"\\" +matPath +'.' +*ext))
			*ext = "vmat_c";
#endif
#ifndef DISABLE_VMT_SUPPORT
		if(!FileManager::Exists(g_materialLocation +"\\" +matPath +'.' +*ext))
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
	auto it = m_nameToMaterialIndex.find(ToMaterialIdentifier(internalMatId));
	if(it == m_nameToMaterialIndex.end())
		return nullptr;
	return m_materials.at(it->second).get();
}
Material *MaterialManager::FindMaterial(const std::string &identifier) const
{
	std::string internalMatId;
	return FindMaterial(identifier,internalMatId);
}
Material *MaterialManager::GetMaterial(MaterialIndex index)
{
	return (index < m_materials.size()) ? m_materials.at(index).get() : nullptr;
}
const Material *MaterialManager::GetMaterial(MaterialIndex index) const {return const_cast<MaterialManager*>(this)->GetMaterial(index);}
std::shared_ptr<ds::Settings> MaterialManager::CreateDataSettings() const {return ds::create_data_settings(ENUM_VARS);}

std::string MaterialManager::ToMaterialIdentifier(const std::string &id) const
{
	auto identifier = id;
	ufile::remove_extension_from_filename(identifier,g_knownMaterialFormats);
	ustring::to_lower(identifier);
	return identifier;
}

bool MaterialManager::Load(const std::string &path,LoadInfo &info,bool bReload)
{
	std::string ext;
	auto &matId = info.identifier;
	auto bHadExtension = false;
	matId = PathToIdentifier(path,&ext,bHadExtension);
	matId = FileManager::GetCanonicalizedPath(matId);

	auto it = m_nameToMaterialIndex.find(ToMaterialIdentifier(matId));
	if(it != m_nameToMaterialIndex.end())
	{
		auto &hMat = m_materials.at(it->second);
		if(hMat.IsValid())
		{
			info.material = hMat.get();
			info.shader = info.material->GetShaderIdentifier();
			if(bReload == false)
				return true;
		}
	}
	std::string absPath = g_materialLocation +"\\";
	absPath += path;
	if(bHadExtension == false)
		absPath += '.' +ext;
	auto sub = matId;
	std::string openMode = "r";
#ifndef DISABLE_VMAT_SUPPORT
	if(ustring::compare(ext,"vmat_c",false))
		openMode = "rb";
#endif
	auto f = FileManager::OpenFile(absPath.c_str(),openMode.c_str());
	if(f == nullptr)
		return false;
	auto handleLoadError = [this](const std::string &matId,Material **mat) -> bool {
		//auto matErr = m_materials.find("error");
		//if(matErr == m_materials.end())
		//	return false;
		auto *nmat = CreateMaterial<Material>();
		AddMaterial(matId,*nmat);
		*mat = nmat;
		return true;
	};
#ifndef DISABLE_VMT_SUPPORT
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
			return LoadVMT(vmt,info);
		auto *err = vlGetLastError();
		// TODO: Don't print this out directly, add an error handler!
		std::cout<<"VMT Parsing error in material '"<<path<<"': "<<err<<std::endl;
		return false;
	}
#endif
#ifndef DISABLE_VMAT_SUPPORT
	if(ustring::compare(ext,"vmat_c",false))
	{
		auto resource = source2::load_resource(f);
		return resource ? LoadVMat(*resource,info) : false;
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
	AddMaterial(info.identifier,*info.material);
	if(info.saveOnDisk)
		info.material->Save(info.material->GetName(),"addons/converted/");
	return info.material;
}
void MaterialManager::SetErrorMaterial(Material *mat)
{
	if(mat == nullptr)
		m_error = MaterialHandle{};
	else
	{
		mat->SetErrorFlag(true);
		m_error = mat->GetHandle();
	}
}
Material *MaterialManager::GetErrorMaterial() const {return m_error.get();}
const std::vector<MaterialHandle> &MaterialManager::GetMaterials() const {return m_materials;}
uint32_t MaterialManager::Clear()
{
	for(auto &hMaterial : m_materials)
	{
		if(hMaterial.IsValid())
			hMaterial->Remove();
	}
	auto n = m_materials.size();
	m_materials.clear();
	m_nameToMaterialIndex.clear();
	return n;
}
void MaterialManager::SetTextureImporter(const std::function<VFilePtr(const std::string&,const std::string&)> &fileHandler) {m_textureImporter = fileHandler;}
const std::function<std::shared_ptr<VFilePtrInternal>(const std::string&,const std::string&)> &MaterialManager::GetTextureImporter() const {return m_textureImporter;}
uint32_t MaterialManager::ClearUnused()
{
	uint32_t n = 0;
	for(auto it=m_materials.begin();it!=m_materials.end();)
	{
		auto &hMaterial = *it;
		if(!hMaterial.IsValid() || (hMaterial.use_count() == 1 && hMaterial.get() != m_error.get())) // Use count = 1 => Only in use by us!
		{
			auto matIdx = hMaterial->GetIndex();
			for(auto it=m_nameToMaterialIndex.begin();it!=m_nameToMaterialIndex.end();++it)
			{
				if(it->second != matIdx)
					continue;
				m_nameToMaterialIndex.erase(it);
				break;
			}
			hMaterial->Remove();
			it = m_materials.erase(it);
			++n;
		}
		else
			++it;
	}
	return n;
}
#pragma optimize("",on)
