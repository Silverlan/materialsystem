/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __MATERIALMANAGER_H__
#define __MATERIALMANAGER_H__

#include "matsysdefinitions.h"
#include "material.h"

#pragma warning(push)
#pragma warning(disable : 4251)
enum class TextureType : uint32_t;

//#ifdef ENABLE_VMT_SUPPORT
namespace VTFLib
{
	class CVMTFile;
};
//#endif

class DLLMATSYS MaterialManager
{
protected:
	std::unordered_map<std::string,MaterialHandle> m_materials;
	uint32_t m_unnamedIdx = 0;
	std::string PathToIdentifier(const std::string &path,std::string *ext,bool &hadExtension) const;
	std::string PathToIdentifier(const std::string &path,std::string *ext) const;
	std::string PathToIdentifier(const std::string &path) const;
	MaterialHandle m_error;

	struct DLLMATSYS LoadInfo
	{
		LoadInfo();
		Material *material;
		std::string identifier;
		std::string shader;
		std::shared_ptr<ds::Block> root;
	};
	bool Load(const std::string &path,LoadInfo &info,bool bReload=false); // Returns true if no error has occurred. 'mat' will only be set if a material with the given path already exists.
	virtual Material *CreateMaterial(const std::string *identifier,const std::string &shader,std::shared_ptr<ds::Block> root=nullptr);
	template<class TMaterial,typename... TARGS>
		TMaterial *CreateMaterial(TARGS...);

	std::shared_ptr<ds::Settings> CreateDataSettings() const;
	std::string ToMaterialIdentifier(const std::string &id) const;
//#ifdef ENABLE_VMT_SUPPORT
	bool LoadVMT(VTFLib::CVMTFile &vmt,LoadInfo &info);
	virtual bool InitializeVMTData(VTFLib::CVMTFile &vmt,LoadInfo &info,ds::Block &rootData,ds::Settings &settings,const std::string &shader);
//#endif
public:
	struct DLLMATSYS ImageFormat
	{
		ImageFormat(TextureType _type,std::string _extension)
			: type(_type),extension(_extension)
		{}
		TextureType type;
		std::string extension;
	};

	MaterialManager();
	MaterialManager &operator=(const MaterialManager&)=delete;
	MaterialManager(const MaterialManager&)=delete;
	virtual ~MaterialManager();

	Material *CreateMaterial(const std::string &identifier,const std::string &shader,const std::shared_ptr<ds::Block> &root=nullptr);
	Material *CreateMaterial(const std::string &shader,const std::shared_ptr<ds::Block> &root=nullptr);
	Material *FindMaterial(const std::string &identifier,std::string &internalMatId) const;
	Material *FindMaterial(const std::string &identifier) const;
	virtual Material *Load(const std::string &path,bool bReload=false,bool *bFirstTimeError=nullptr);
	virtual void SetErrorMaterial(Material *mat);
	Material *GetErrorMaterial() const;
	const std::unordered_map<std::string,MaterialHandle> &GetMaterials() const;
	void Clear(); // Clears all materials (+Textures?)
	void ClearUnused();

	static const std::vector<ImageFormat> &get_supported_image_formats();
	static void SetRootMaterialLocation(const std::string &location);
	static const std::string &GetRootMaterialLocation();
};
#pragma warning(pop)

template<class TMaterial,typename... TARGS>
	TMaterial *MaterialManager::CreateMaterial(TARGS ...args) {return new TMaterial{*this,std::forward<TARGS>(args)...};}

#endif
