/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __MSYS_MATERIAL_MANAGER_HPP__
#define __MSYS_MATERIAL_MANAGER_HPP__

#include "matsysdefinitions.h"
#include "material.h"
#include <sharedutils/asset_loader/file_asset_processor.hpp>
#include <sharedutils/asset_loader/asset_format_loader.hpp>
#include <sharedutils/asset_loader/file_asset_manager.hpp>

namespace msys
{
	class DLLMATSYS MaterialProcessor
		: public util::FileAssetProcessor
	{
	public:
		MaterialProcessor(util::AssetFormatLoader &loader,std::unique_ptr<util::IAssetFormatHandler> &&handler);
		virtual bool Load() override;
		virtual bool Finalize() override;

		std::shared_ptr<Material> material = nullptr;
		std::string identifier;
		std::string formatExtension;
	};
	class DLLMATSYS MaterialLoader
		: public util::TAssetFormatLoader<MaterialProcessor>
	{
	public:
		MaterialLoader(util::IAssetManager &assetManager)
			: util::TAssetFormatLoader<MaterialProcessor>{assetManager}
		{}
	protected:
		virtual std::unique_ptr<util::IAssetProcessor> CreateAssetProcessor(
			const std::string &identifier,const std::string &ext,std::unique_ptr<util::IAssetFormatHandler> &&formatHandler
		) override;
	};
	struct DLLMATSYS MaterialLoadInfo
		: public util::AssetLoadInfo
	{
		MaterialLoadInfo(util::AssetLoadFlags flags=util::AssetLoadFlags::None);
	};
	class DLLMATSYS MaterialFormatHandler
		: public util::IAssetFormatHandler
	{
	public:
		MaterialFormatHandler(util::IAssetManager &assetManager);
		bool LoadData(MaterialProcessor &processor,MaterialLoadInfo &info);

		std::shared_ptr<ds::Block> data;
		std::string shader;
	};
	class DLLMATSYS MaterialManager
		: public util::TFileAssetManager<Material,MaterialLoadInfo>
	{
	public:
		MaterialManager();
		virtual ~MaterialManager()=default;

		void SetErrorMaterial(Material *mat);
		Material *GetErrorMaterial() const;
		
		std::shared_ptr<ds::Settings> CreateDataSettings() const;
		virtual std::shared_ptr<Material> CreateMaterial(const std::string &shader,const std::shared_ptr<ds::Block> &data);
		std::shared_ptr<Material> CreateMaterial(const std::string &identifier,const std::string &shader,const std::shared_ptr<ds::Block> &data);
	protected:
		virtual void InitializeProcessor(util::IAssetProcessor &processor) override;
		virtual util::AssetObject InitializeAsset(const util::Asset &asset,const util::AssetLoadJob &job) override;
		msys::MaterialHandle m_error;
	};
};

#endif
