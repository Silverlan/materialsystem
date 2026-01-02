// SPDX-FileCopyrightText: (c) 2021 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module pragma.cmaterialsystem;

import :texture_manager;

// #define ENABLE_VERBOSE_OUTPUT

pragma::material::TextureLoadInfo::TextureLoadInfo(pragma::util::AssetLoadFlags flags) : AssetLoadInfo {flags}, mipmapMode {TextureMipmapMode::LoadOrGenerate} {}

/////////////

pragma::material::TextureManager::TextureManager(prosper::IPrContext &context) : m_context {context}
{
	auto fileHandler = std::make_unique<pragma::util::AssetFileHandler>();
	fileHandler->open = [](const std::string &path, pragma::util::AssetFormatType formatType) -> std::unique_ptr<ufile::IFile> {
		auto openMode = fs::FileMode::Read;
		if(formatType == pragma::util::AssetFormatType::Binary)
			openMode |= fs::FileMode::Binary;
		auto fp = fs::open_file(path, openMode);
		if(!fp)
			return nullptr;
		return std::make_unique<fs::File>(fp);
	};
	fileHandler->exists = [](const std::string &path) -> bool { return fs::exists(path); };
	SetFileHandler(std::move(fileHandler));

	m_loader = std::make_unique<TextureLoader>(*this, context);
	auto gliHandler = [](IAssetManager &assetManager) -> std::unique_ptr<ITextureFormatHandler> { return std::make_unique<TextureFormatHandlerGli>(assetManager); };

	// Note: Registration order also represents order of preference/priority
	RegisterFormatHandler("dds", gliHandler);
	RegisterFormatHandler("ktx", gliHandler);

	RegisterFormatHandler("vtf", [](IAssetManager &assetManager) -> std::unique_ptr<ITextureFormatHandler> { return std::make_unique<TextureFormatHandlerVtf>(assetManager); });
	RegisterFormatHandler("vtex_c", [](IAssetManager &assetManager) -> std::unique_ptr<ITextureFormatHandler> { return std::make_unique<TextureFormatHandlerVtex>(assetManager); });

	auto uimgHandler = [](IAssetManager &assetManager) -> std::unique_ptr<ITextureFormatHandler> { return std::make_unique<TextureFormatHandlerUimg>(assetManager); };
	RegisterFormatHandler("png", uimgHandler);
	RegisterFormatHandler("tga", uimgHandler);
	RegisterFormatHandler("jpg", uimgHandler);
	RegisterFormatHandler("bmp", uimgHandler);
	RegisterFormatHandler("psd", uimgHandler);
	RegisterFormatHandler("gif", uimgHandler);
	RegisterFormatHandler("hdr", uimgHandler);
	RegisterFormatHandler("pic", uimgHandler);

	RegisterFormatHandler("svg", [](IAssetManager &assetManager) -> std::unique_ptr<ITextureFormatHandler> { return std::make_unique<TextureFormatHandlerSvg>(assetManager); });

	static_cast<TextureLoader &>(GetLoader()).SetAllowMultiThreadedGpuResourceAllocation(context.SupportsMultiThreadedResourceAllocation());
}

pragma::material::TextureManager::~TextureManager() { m_error = nullptr; }

void pragma::material::TextureManager::InitializeProcessor(pragma::util::IAssetProcessor &processor)
{
	auto &txProcessor = static_cast<TextureProcessor &>(processor);
	txProcessor.mipmapMode = static_cast<TextureLoadInfo &>(*txProcessor.loadInfo).mipmapMode;
	txProcessor.SetTextureData(static_cast<TextureLoadInfo &>(*txProcessor.loadInfo).textureData);
}

pragma::util::AssetObject pragma::material::TextureManager::InitializeAsset(const pragma::util::Asset &asset, const pragma::util::AssetLoadJob &job)
{
	auto &texProcessor = *static_cast<TextureProcessor *>(job.processor.get());
	auto texture = texProcessor.texture;

	auto texWrapper = std::make_shared<Texture>(m_context, texture);

	auto &img = texture->GetImage();
	auto flags = texWrapper->GetFlags();
	pragma::math::set_flag(flags, Texture::Flags::SRGB, img.IsSrgb());
	flags |= Texture::Flags::Indexed | Texture::Flags::Loaded;
	flags &= ~Texture::Flags::Error;
	texWrapper->SetFlags(flags);
	texWrapper->SetName(job.identifier);

	return texWrapper;
}

std::shared_ptr<pragma::material::Texture> pragma::material::TextureManager::GetErrorTexture() { return m_error; }

void pragma::material::TextureManager::SetErrorTexture(const std::shared_ptr<Texture> &tex)
{
	if(m_error) {
		auto flags = m_error->GetFlags();
		pragma::math::set_flag(flags, Texture::Flags::Error, false);
		m_error->SetFlags(flags);
	}
	m_error = tex;
	if(tex)
		tex->AddFlags(Texture::Flags::Error);
}

void pragma::material::TextureManager::Test()
{
	std::string basePath = pragma::util::get_program_path() + '/';
	auto addTexture = [&basePath, this](const std::string &texPath, const std::string &identifier) {
		auto fp = fs::open_system_file(basePath + "/" + texPath, fs::FileMode::Binary | fs::FileMode::Read);
		auto f = std::make_unique<fs::File>(fp);
		std::string ext;
		ufile::get_extension(texPath, &ext);
		GetLoader().AddJob(identifier, ext, std::move(f));
	};
	addTexture("materials/error.dds", "error");

	uint32_t numComplete = 0;
	auto t = std::chrono::high_resolution_clock::now();
	for(;;) {
		GetLoader().Poll([&numComplete, &t](const pragma::util::AssetLoadJob &job, pragma::util::AssetLoadResult result, std::optional<std::string> errMsg) {
			switch(result) {
			case pragma::util::AssetLoadResult::Succeeded:
				{
					auto dtQueue = job.completionTime - job.queueStartTime;
					auto dtTask = job.completionTime - job.taskStartTime;
					std::cout << job.identifier << " has been loaded!" << std::endl;
					std::cout << "Time since job has been queued to completion: " << (dtQueue.count() / 1'000'000'000.0) << std::endl;
					std::cout << "Time since task has been started to completion: " << (dtTask.count() / 1'000'000'000.0) << std::endl;
					if(++numComplete == 7) {
						auto dt = std::chrono::high_resolution_clock::now() - t;
						std::cout << "Total completion time: " << (dt.count() / 1'000'000'000.0) << std::endl;
					}
					break;
				}
			default:
				{
					std::string msg = job.identifier + " has failed: ";
					msg += errMsg ? *errMsg : "Unknown error";
					std::cout << msg << std::endl;
					break;
				}
			}
		});
	}
}
