// SPDX-FileCopyrightText: (c) 2021 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#include <prosper_context.hpp>
#include "texturemanager/texture_manager2.hpp"
#include "texturemanager/load/texture_loader.hpp"
#include "texturemanager/load/texture_format_handler.hpp"
#include "texturemanager/load/handlers/format_handler_gli.hpp"
#include "texturemanager/load/handlers/format_handler_uimg.hpp"
#include "texturemanager/load/handlers/format_handler_vtex.hpp"
#include "texturemanager/load/handlers/format_handler_vtf.hpp"
#include "texturemanager/load/handlers/format_handler_svg.hpp"
#include "texturemanager/load/texture_processor.hpp"
#include "texturemanager/texture.h"
#include <sharedutils/util.h>
#include <sharedutils/util_file.h>
#include <fsys/filesystem.h>
#include <fsys/ifile.hpp>

// #define ENABLE_VERBOSE_OUTPUT

msys::TextureLoadInfo::TextureLoadInfo(util::AssetLoadFlags flags) : util::AssetLoadInfo {flags}, mipmapMode {TextureMipmapMode::LoadOrGenerate} {}

/////////////

msys::TextureManager::TextureManager(prosper::IPrContext &context) : m_context {context}
{
	auto fileHandler = std::make_unique<util::AssetFileHandler>();
	fileHandler->open = [](const std::string &path, util::AssetFormatType formatType) -> std::unique_ptr<ufile::IFile> {
		auto openMode = filemanager::FileMode::Read;
		if(formatType == util::AssetFormatType::Binary)
			openMode |= filemanager::FileMode::Binary;
		auto fp = filemanager::open_file(path, openMode);
		if(!fp)
			return nullptr;
		return std::make_unique<fsys::File>(fp);
	};
	fileHandler->exists = [](const std::string &path) -> bool { return filemanager::exists(path); };
	SetFileHandler(std::move(fileHandler));

	m_loader = std::make_unique<msys::TextureLoader>(*this, context);
	auto gliHandler = [](util::IAssetManager &assetManager) -> std::unique_ptr<msys::ITextureFormatHandler> { return std::make_unique<msys::TextureFormatHandlerGli>(assetManager); };

	// Note: Registration order also represents order of preference/priority
	RegisterFormatHandler("dds", gliHandler);
	RegisterFormatHandler("ktx", gliHandler);

	RegisterFormatHandler("vtf", [](util::IAssetManager &assetManager) -> std::unique_ptr<msys::ITextureFormatHandler> { return std::make_unique<msys::TextureFormatHandlerVtf>(assetManager); });
	RegisterFormatHandler("vtex_c", [](util::IAssetManager &assetManager) -> std::unique_ptr<msys::ITextureFormatHandler> { return std::make_unique<msys::TextureFormatHandlerVtex>(assetManager); });

	auto uimgHandler = [](util::IAssetManager &assetManager) -> std::unique_ptr<msys::ITextureFormatHandler> { return std::make_unique<msys::TextureFormatHandlerUimg>(assetManager); };
	RegisterFormatHandler("png", uimgHandler);
	RegisterFormatHandler("tga", uimgHandler);
	RegisterFormatHandler("jpg", uimgHandler);
	RegisterFormatHandler("bmp", uimgHandler);
	RegisterFormatHandler("psd", uimgHandler);
	RegisterFormatHandler("gif", uimgHandler);
	RegisterFormatHandler("hdr", uimgHandler);
	RegisterFormatHandler("pic", uimgHandler);

	RegisterFormatHandler("svg", [](util::IAssetManager &assetManager) -> std::unique_ptr<msys::ITextureFormatHandler> { return std::make_unique<msys::TextureFormatHandlerSvg>(assetManager); });

	static_cast<TextureLoader &>(GetLoader()).SetAllowMultiThreadedGpuResourceAllocation(context.SupportsMultiThreadedResourceAllocation());
}

msys::TextureManager::~TextureManager() { m_error = nullptr; }

void msys::TextureManager::InitializeProcessor(util::IAssetProcessor &processor)
{
	auto &txProcessor = static_cast<TextureProcessor &>(processor);
	txProcessor.mipmapMode = static_cast<TextureLoadInfo &>(*txProcessor.loadInfo).mipmapMode;
	txProcessor.SetTextureData(static_cast<TextureLoadInfo &>(*txProcessor.loadInfo).textureData);
}

util::AssetObject msys::TextureManager::InitializeAsset(const util::Asset &asset, const util::AssetLoadJob &job)
{
	auto &texProcessor = *static_cast<TextureProcessor *>(job.processor.get());
	auto texture = texProcessor.texture;

	auto texWrapper = std::make_shared<Texture>(m_context, texture);

	auto &img = texture->GetImage();
	auto flags = texWrapper->GetFlags();
	umath::set_flag(flags, Texture::Flags::SRGB, img.IsSrgb());
	flags |= Texture::Flags::Indexed | Texture::Flags::Loaded;
	flags &= ~Texture::Flags::Error;
	texWrapper->SetFlags(flags);
	texWrapper->SetName(job.identifier);

	return texWrapper;
}

std::shared_ptr<Texture> msys::TextureManager::GetErrorTexture() { return m_error; }

void msys::TextureManager::SetErrorTexture(const std::shared_ptr<Texture> &tex)
{
	if(m_error) {
		auto flags = m_error->GetFlags();
		umath::set_flag(flags, Texture::Flags::Error, false);
		m_error->SetFlags(flags);
	}
	m_error = tex;
	if(tex)
		tex->AddFlags(Texture::Flags::Error);
}

void msys::TextureManager::Test()
{
	std::string basePath = util::get_program_path() + '/';
	auto addTexture = [&basePath, this](const std::string &texPath, const std::string &identifier) {
		auto fp = filemanager::open_system_file(basePath + "/" + texPath, filemanager::FileMode::Binary | filemanager::FileMode::Read);
		auto f = std::make_unique<fsys::File>(fp);
		std::string ext;
		ufile::get_extension(texPath, &ext);
		GetLoader().AddJob(identifier, ext, std::move(f));
	};
	addTexture("materials/error.dds", "error");

	uint32_t numComplete = 0;
	auto t = std::chrono::high_resolution_clock::now();
	for(;;) {
		GetLoader().Poll([&numComplete, &t](const util::AssetLoadJob &job, util::AssetLoadResult result, std::optional<std::string> errMsg) {
			switch(result) {
			case util::AssetLoadResult::Succeeded:
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
