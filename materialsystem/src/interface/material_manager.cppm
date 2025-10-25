// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "matsysdefinitions.hpp"
#include <optional>
#include <string>
#include <memory>
#include <functional>
#include <vector>
#ifndef DISABLE_VMT_SUPPORT
#include <VMTFile.h>
#endif

export module pragma.materialsystem:material_manager;

export import :enums;
export import :material;

#ifndef DISABLE_VMAT_SUPPORT
import source2;
#endif

#pragma warning(push)
#pragma warning(disable : 4251)

export {
	class DLLMATSYS MaterialManager {
	public:
		std::optional<std::string> FindMaterialPath(const std::string &material);
		struct DLLMATSYS ImageFormat {
			ImageFormat(msys::TextureType _type, std::string _extension) : type(_type), extension(_extension) {}
			msys::TextureType type;
			std::string extension;
		};

		MaterialManager();
		MaterialManager &operator=(const MaterialManager &) = delete;
		MaterialManager(const MaterialManager &) = delete;
		virtual ~MaterialManager();

		msys::Material *CreateMaterial(const std::string &identifier, const std::string &shader, const std::shared_ptr<ds::Block> &root = nullptr);
		msys::Material *CreateMaterial(const std::string &shader, const std::shared_ptr<ds::Block> &root = nullptr);
		msys::Material *FindMaterial(const std::string &identifier, std::string &internalMatId) const;
		msys::Material *FindMaterial(const std::string &identifier) const;
		msys::Material *GetMaterial(msys::MaterialIndex index);
		const msys::Material *GetMaterial(msys::MaterialIndex index) const;
		virtual msys::Material *Load(const std::string &path, bool bReload = false, bool loadInstantly = true, bool *bFirstTimeError = nullptr);
		virtual void SetErrorMaterial(msys::Material *mat);
		msys::Material *GetErrorMaterial() const;
		const std::vector<msys::MaterialHandle> &GetMaterials() const;
		uint32_t Clear(); // Clears all materials (+Textures?)
		uint32_t ClearUnused();
		void SetTextureImporter(const std::function<std::shared_ptr<VFilePtrInternal>(const std::string &, const std::string &)> &fileHandler);
		const std::function<std::shared_ptr<VFilePtrInternal>(const std::string &, const std::string &)> &GetTextureImporter() const;

		static const std::vector<ImageFormat> &get_supported_image_formats();
		static void SetRootMaterialLocation(const std::string &location);
		static const std::string &GetRootMaterialLocation();
	protected:
		std::vector<msys::MaterialHandle> m_materials;
		std::unordered_map<std::string, msys::MaterialIndex> m_nameToMaterialIndex;
		uint32_t m_unnamedIdx = 0;
		std::string PathToIdentifier(const std::string &path, std::string *ext, bool &hadExtension) const;
		std::string PathToIdentifier(const std::string &path, std::string *ext) const;
		std::string PathToIdentifier(const std::string &path) const;
		msys::MaterialHandle m_error;
		std::function<std::shared_ptr<VFilePtrInternal>(const std::string &, const std::string &)> m_textureImporter = nullptr;

		struct DLLMATSYS LoadInfo {
			LoadInfo();
			msys::Material *material;
			std::string identifier;
			std::string shader;
			std::shared_ptr<ds::Block> root;
			bool saveOnDisk = false;
		};
		bool Load(const std::string &path, LoadInfo &info, bool bReload = false); // Returns true if no error has occurred. 'mat' will only be set if a material with the given path already exists.
		virtual msys::Material *CreateMaterial(const std::string *identifier, const std::string &shader, std::shared_ptr<ds::Block> root = nullptr);
		template<class TMaterial, typename... TARGS>
		TMaterial *CreateMaterial(TARGS...);

		std::shared_ptr<ds::Settings> CreateDataSettings() const;
		std::string ToMaterialIdentifier(const std::string &id) const;
		void AddMaterial(const std::string &identifier, msys::Material &mat);
	#ifndef DISABLE_VMT_SUPPORT
		bool LoadVMT(VTFLib::CVMTFile &vmt, LoadInfo &info);
		virtual bool InitializeVMTData(VTFLib::CVMTFile &vmt, LoadInfo &info, ds::Block &rootData, ds::Settings &settings, const std::string &shader);
	#endif
	#ifndef DISABLE_VMAT_SUPPORT
		enum class VMatOrigin : uint8_t { Source2 = 0, SteamVR, Dota2 };
		bool LoadVMat(source2::resource::Resource &vmat, LoadInfo &info);
		virtual bool InitializeVMatData(source2::resource::Resource &resource, source2::resource::Material &vmat, LoadInfo &info, ds::Block &rootData, ds::Settings &settings, const std::string &shader, VMatOrigin origin);
	#endif
		bool LoadUdm(std::shared_ptr<VFilePtrInternal> &f, LoadInfo &loadInfo);
	};
	#pragma warning(pop)

	//template<class TMaterial,typename... TARGS>
	//	TMaterial *MaterialManager::CreateMaterial(TARGS ...args) {return TMaterial::Create(*this,std::forward<TARGS>(args)...);}
}
