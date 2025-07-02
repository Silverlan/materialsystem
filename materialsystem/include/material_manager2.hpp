// SPDX-FileCopyrightText: (c) 2021 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __MSYS_MATERIAL_MANAGER_HPP__
#define __MSYS_MATERIAL_MANAGER_HPP__

#include "matsysdefinitions.h"
#include "material.h"
#include <sharedutils/asset_loader/file_asset_processor.hpp>
#include <sharedutils/asset_loader/asset_format_loader.hpp>
#include <sharedutils/asset_loader/file_asset_manager.hpp>

namespace udm {
	struct LinkedPropertyWrapper;
};
namespace msys {
	DLLMATSYS void set_use_vkv_vmt_parser(bool useVkvParser);
	DLLMATSYS bool should_use_vkv_vmt_parser();
	DLLMATSYS bool udm_to_data_block(udm::LinkedPropertyWrapper &udmDataRoot, ds::Block &root);
	class DLLMATSYS MaterialProcessor : public util::FileAssetProcessor {
	  public:
		MaterialProcessor(util::AssetFormatLoader &loader, std::unique_ptr<util::IAssetFormatHandler> &&handler);
		virtual bool Load() override;
		virtual bool Finalize() override;

		std::shared_ptr<Material> material = nullptr;
		std::string identifier;
		std::string formatExtension;
	};
	class DLLMATSYS MaterialLoader : public util::TAssetFormatLoader<MaterialProcessor> {
	  public:
		MaterialLoader(util::IAssetManager &assetManager) : util::TAssetFormatLoader<MaterialProcessor> {assetManager, "material"} {}
	  protected:
		virtual std::unique_ptr<util::IAssetProcessor> CreateAssetProcessor(const std::string &identifier, const std::string &ext, std::unique_ptr<util::IAssetFormatHandler> &&formatHandler) override;
	};
	struct DLLMATSYS MaterialLoadInfo : public util::AssetLoadInfo {
		MaterialLoadInfo(util::AssetLoadFlags flags = util::AssetLoadFlags::None);
	};
	class DLLMATSYS MaterialFormatHandler : public util::IAssetFormatHandler {
	  public:
		MaterialFormatHandler(util::IAssetManager &assetManager);
		virtual bool LoadData(MaterialProcessor &processor, MaterialLoadInfo &info) = 0;

		std::shared_ptr<ds::Block> data;
		std::string shader;
		std::string baseMaterial;
	};
	class DLLMATSYS PmatFormatHandler : public MaterialFormatHandler {
	  public:
		PmatFormatHandler(util::IAssetManager &assetManager);
		virtual bool LoadData(MaterialProcessor &processor, MaterialLoadInfo &info) override;
	};
	class DLLMATSYS WmiFormatHandler : public MaterialFormatHandler {
	  public:
		WmiFormatHandler(util::IAssetManager &assetManager);
		virtual bool LoadData(MaterialProcessor &processor, MaterialLoadInfo &info) override;
	};
	class DLLMATSYS MaterialManager : public util::TFileAssetManager<Material, MaterialLoadInfo> {
	  public:
		static std::shared_ptr<MaterialManager> Create();
		virtual ~MaterialManager() = default;

		void SetErrorMaterial(Material *mat);
		Material *GetErrorMaterial() const;

		std::shared_ptr<Material> ReloadAsset(const std::string &path, std::unique_ptr<MaterialLoadInfo> &&loadInfo = nullptr, PreloadResult *optOutResult = nullptr);

		std::shared_ptr<ds::Settings> CreateDataSettings() const;
		virtual std::shared_ptr<Material> CreateMaterial(const std::string &shader, const std::shared_ptr<ds::Block> &data);
		virtual std::shared_ptr<Material> CreateMaterial(const udm::AssetData &data, std::string &outErr);
		std::shared_ptr<Material> CreateMaterial(const std::string &identifier, const std::string &shader, const std::shared_ptr<ds::Block> &data);
	  protected:
		friend MaterialProcessor;
		MaterialManager();
		virtual void Reset() override;
		virtual void Initialize();
		virtual void InitializeImportHandlers();
		virtual void InitializeProcessor(util::IAssetProcessor &processor) override;
		virtual std::shared_ptr<Material> CreateMaterialObject(const std::string &shader, const std::shared_ptr<ds::Block> &data);
		virtual util::AssetObject InitializeAsset(const util::Asset &asset, const util::AssetLoadJob &job) override;
		virtual util::AssetObject ReloadAsset(const std::string &path, std::unique_ptr<util::AssetLoadInfo> &&loadInfo, PreloadResult *optOutResult = nullptr) override;
		msys::MaterialHandle m_error;
	};
};

#endif
