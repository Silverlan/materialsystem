// SPDX-FileCopyrightText: (c) 2021 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.materialsystem:material_manager2;

export import :material;

export namespace pragma::material {
	DLLMATSYS void set_use_vkv_vmt_parser(bool useVkvParser);
	DLLMATSYS bool should_use_vkv_vmt_parser();
	DLLMATSYS bool udm_to_data_block(udm::LinkedPropertyWrapper &udmDataRoot, datasystem::Block &root);
	class DLLMATSYS MaterialProcessor : public pragma::util::FileAssetProcessor {
	  public:
		MaterialProcessor(pragma::util::AssetFormatLoader &loader, std::unique_ptr<pragma::util::IAssetFormatHandler> &&handler);
		virtual bool Load() override;
		virtual bool Finalize() override;

		std::shared_ptr<Material> material = nullptr;
		std::string identifier;
		std::string formatExtension;
	};
	class DLLMATSYS MaterialLoader : public pragma::util::TAssetFormatLoader<MaterialProcessor> {
	  public:
		MaterialLoader(pragma::util::IAssetManager &assetManager) : TAssetFormatLoader<MaterialProcessor> {assetManager, "material"} {}
	  protected:
		virtual std::unique_ptr<pragma::util::IAssetProcessor> CreateAssetProcessor(const std::string &identifier, const std::string &ext, std::unique_ptr<pragma::util::IAssetFormatHandler> &&formatHandler) override;
	};
	struct DLLMATSYS MaterialLoadInfo : public pragma::util::AssetLoadInfo {
		MaterialLoadInfo(pragma::util::AssetLoadFlags flags = pragma::util::AssetLoadFlags::None);
	};
	class DLLMATSYS MaterialFormatHandler : public pragma::util::IAssetFormatHandler {
	  public:
		MaterialFormatHandler(pragma::util::IAssetManager &assetManager);
		virtual bool LoadData(MaterialProcessor &processor, MaterialLoadInfo &info) = 0;

		std::shared_ptr<datasystem::Block> data;
		std::string shader;
		std::string baseMaterial;
	};
	class DLLMATSYS PmatFormatHandler : public MaterialFormatHandler {
	  public:
		PmatFormatHandler(pragma::util::IAssetManager &assetManager);
		virtual bool LoadData(MaterialProcessor &processor, MaterialLoadInfo &info) override;
	};
	class DLLMATSYS WmiFormatHandler : public MaterialFormatHandler {
	  public:
		WmiFormatHandler(pragma::util::IAssetManager &assetManager);
		virtual bool LoadData(MaterialProcessor &processor, MaterialLoadInfo &info) override;
	};
	class DLLMATSYS MaterialManager : public pragma::util::TFileAssetManager<Material, MaterialLoadInfo> {
	  public:
		static std::shared_ptr<MaterialManager> Create();
		virtual ~MaterialManager() = default;

		void SetErrorMaterial(Material *mat);
		Material *GetErrorMaterial() const;

		std::shared_ptr<Material> ReloadAsset(const std::string &path, std::unique_ptr<MaterialLoadInfo> &&loadInfo = nullptr, PreloadResult *optOutResult = nullptr);

		std::shared_ptr<datasystem::Settings> CreateDataSettings() const;
		virtual std::shared_ptr<Material> CreateMaterial(const std::string &shader, const std::shared_ptr<datasystem::Block> &data);
		virtual std::shared_ptr<Material> CreateMaterial(const udm::AssetData &data, std::string &outErr);
		std::shared_ptr<Material> CreateMaterial(const std::string &identifier, const std::string &shader, const std::shared_ptr<datasystem::Block> &data);
	  protected:
		friend MaterialProcessor;
		MaterialManager();
		virtual void Reset() override;
		virtual void Initialize();
		virtual void InitializeImportHandlers();
		virtual void InitializeProcessor(pragma::util::IAssetProcessor &processor) override;
		virtual std::shared_ptr<Material> CreateMaterialObject(const std::string &shader, const std::shared_ptr<datasystem::Block> &data);
		virtual pragma::util::AssetObject InitializeAsset(const pragma::util::Asset &asset, const pragma::util::AssetLoadJob &job) override;
		virtual pragma::util::AssetObject ReloadAsset(const std::string &path, std::unique_ptr<pragma::util::AssetLoadInfo> &&loadInfo, PreloadResult *optOutResult = nullptr) override;
		MaterialHandle m_error;
	};
};
