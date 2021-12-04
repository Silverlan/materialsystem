/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <prosper_context.hpp>
#include "texturemanager/texture_manager2.hpp"
#include "texturemanager/load/texture_loader.hpp"
#include "texturemanager/load/texture_format_handler.hpp"
#include "texturemanager/load/handlers/format_handler_gli.hpp"
#include "texturemanager/load/handlers/format_handler_uimg.hpp"
#include "texturemanager/load/handlers/format_handler_vtex.hpp"
#include "texturemanager/load/handlers/format_handler_vtf.hpp"
#include "texturemanager/load/texture_processor.hpp"
#include "texturemanager/texture.h"
#include <sharedutils/util.h>
#include <sharedutils/util_file.h>
#include <fsys/filesystem.h>
#include <fsys/ifile.hpp>

bool msys::TextureAsset::IsInUse() const {return texture.use_count() > 1;}

/////////////

msys::TextureManager::TextureManager(prosper::IPrContext &context)
	: m_context{context}
{
	m_loader = std::make_unique<msys::TextureLoader>(context);
	auto gliHandler = []() -> std::unique_ptr<msys::ITextureFormatHandler> {
		return std::make_unique<msys::TextureFormatHandlerGli>();
	};

	// Note: Registration order also represents order of preference/priority
	RegisterFormatHandler("dds",gliHandler);
	RegisterFormatHandler("ktx",gliHandler);
	
	RegisterFormatHandler("vtf",[]() -> std::unique_ptr<msys::ITextureFormatHandler> {
		return std::make_unique<msys::TextureFormatHandlerVtf>();
	});
	RegisterFormatHandler("vtex_c",[]() -> std::unique_ptr<msys::ITextureFormatHandler> {
		return std::make_unique<msys::TextureFormatHandlerVtex>();
	});

	auto uimgHandler = []() -> std::unique_ptr<msys::ITextureFormatHandler> {
		return std::make_unique<msys::TextureFormatHandlerUimg>();
	};
	RegisterFormatHandler("png",uimgHandler);
	RegisterFormatHandler("tga",uimgHandler);
	RegisterFormatHandler("jpg",uimgHandler);
	RegisterFormatHandler("bmp",uimgHandler);
	RegisterFormatHandler("psd",uimgHandler);
	RegisterFormatHandler("gif",uimgHandler);
	RegisterFormatHandler("hdr",uimgHandler);
	RegisterFormatHandler("pic",uimgHandler);

	m_loader->SetAllowMultiThreadedGpuResourceAllocation(context.SupportsMultiThreadedResourceAllocation());
}

msys::TextureManager::~TextureManager() {}

void msys::TextureManager::RegisterFormatHandler(const std::string &ext,const std::function<std::unique_ptr<ITextureFormatHandler>()> &factory)
{
	m_loader->RegisterFormatHandler(ext,factory);
	RegisterFileExtension(ext);
}

void msys::TextureManager::SetRootDirectory(const std::string &dir) {m_rootDir = util::Path::CreatePath(dir);}
const util::Path &msys::TextureManager::GetRootDirectory() const {return m_rootDir;}

void msys::TextureManager::RemoveFromCache(const std::string &path) {IAssetManager::RemoveFromCache(path);}
msys::TextureManager::PreloadResult msys::TextureManager::PreloadTexture(const std::string &strPath) {return PreloadTexture(strPath,0);}
msys::TextureManager::PreloadResult msys::TextureManager::PreloadTexture(const std::string &strPath,util::AssetLoadJobPriority priority)
{
	auto *asset = FindCachedAsset(strPath);
	if(asset)
		return PreloadResult{{},true};
	auto path = m_rootDir +util::Path::CreateFile(strPath);
	auto ext = path.GetFileExtension();
	if(ext.has_value())
	{
		// Confirm that this is a valid texture format extension
		auto it = std::find_if(m_extensions.begin(),m_extensions.end(),[&ext](const std::pair<std::string,size_t> &pair) {
			return *ext == pair.first;
		});
		if(it == m_extensions.end())
			ext = {};
	}
	if(!ext.has_value())
	{
		// No valid extension found, search for all known extensions
		for(auto &pair : m_extensions)
		{
			auto extPath = path +("." +pair.first);
			if(filemanager::exists(extPath.GetString()))
			{
				path = std::move(extPath);
				ext = pair.first;
				break;
			}
		}
	}
	if(!ext.has_value())
		return PreloadResult{{},false};
	auto fp = filemanager::open_file(path.GetString(),filemanager::FileMode::Binary | filemanager::FileMode::Read);
	if(!fp)
		return PreloadResult{{},false};
	auto f = std::make_shared<fsys::File>(fp);
	auto jobId = m_loader->AddJob(ToCacheIdentifier(strPath),*ext,f,priority);
	if(!jobId.has_value())
		return PreloadResult{{},false};
	return PreloadResult{jobId,true};
}
Texture *msys::TextureManager::LoadTexture(const std::string &path)
{
	auto r = PreloadTexture(path,10);
	if(r.success == false)
		return nullptr;
	if(!r.jobId.has_value())
	{
		// Already fully loaded
		return static_cast<TextureAsset*>(FindCachedAsset(path))->texture.get();
	}
	auto identifier = ToCacheIdentifier(path);
	std::optional<bool> loadSuccess {};
	std::shared_ptr<prosper::Texture> texture = nullptr;
	auto jobId = *r.jobId;
	while(!loadSuccess.has_value())
	{
		// Wait until texture has been loaded
		m_loader->Poll([&loadSuccess,jobId,&texture](const util::AssetLoadJob &job) {
			if(job.jobId == jobId)
			{
				loadSuccess = true;
				auto &texProcessor = *static_cast<TextureProcessor*>(job.processor.get());
				texture = texProcessor.texture;
			}
		},[&loadSuccess,jobId](const util::AssetLoadJob &job) {
			if(job.jobId == jobId)
				loadSuccess = false;
		},true);
	}
	if(!*loadSuccess)
		return nullptr;

	auto texWrapper = std::make_shared<Texture>(m_context,texture);
	
	auto &img = texture->GetImage();
	auto flags = texWrapper->GetFlags();
	umath::set_flag(flags,Texture::Flags::SRGB,img.IsSrgb());
	flags |= Texture::Flags::Indexed | Texture::Flags::Loaded;
	texWrapper->SetFlags(flags);

	auto asset = std::make_shared<TextureAsset>(texWrapper);
	AddToCache(identifier,asset);
	return asset->texture.get();
}

std::shared_ptr<Texture> msys::TextureManager::GetErrorTexture() {return m_error;}

void msys::TextureManager::SetErrorTexture(const std::shared_ptr<Texture> &tex)
{
	if(m_error)
	{
		auto flags = m_error->GetFlags();
		umath::set_flag(flags,Texture::Flags::Error,false);
		m_error->SetFlags(flags);
	}
	m_error = tex;
	if(tex)
		tex->AddFlags(Texture::Flags::Error);
}

void msys::TextureManager::Poll()
{
	m_loader->Poll([](const util::AssetLoadJob &job) {
		// Succeeded
	},[](const util::AssetLoadJob &job) {
		// Failed
	});
}

void msys::TextureManager::Test()
{
	std::string basePath = util::get_program_path() +'/';
	auto addTexture = [&basePath,this](const std::string &texPath,const std::string &identifier) {
		auto fp = filemanager::open_system_file(basePath +"/" +texPath,filemanager::FileMode::Binary | filemanager::FileMode::Read);
		auto f = std::make_shared<fsys::File>(fp);
		std::string ext;
		ufile::get_extension(texPath,&ext);
		m_loader->AddJob(identifier,ext,f);
	};
	addTexture("materials/error.dds","error");

	uint32_t numComplete = 0;
	auto t = std::chrono::high_resolution_clock::now();
	for(;;)
	{
		m_loader->Poll([&numComplete,&t](const util::AssetLoadJob &job) {
			auto dtQueue = job.completionTime -job.queueStartTime;
			auto dtTask = job.completionTime -job.taskStartTime;
			std::cout<<job.identifier<<" has been loaded!"<<std::endl;
			std::cout<<"Time since job has been queued to completion: "<<(dtQueue.count() /1'000'000'000.0)<<std::endl;
			std::cout<<"Time since task has been started to completion: "<<(dtTask.count() /1'000'000'000.0)<<std::endl;
			if(++numComplete == 7)
			{
				auto dt = std::chrono::high_resolution_clock::now() -t;
				std::cout<<"Total completion time: "<<(dt.count() /1'000'000'000.0)<<std::endl;
			}
		},[](const util::AssetLoadJob &job) {
			std::cout<<job.identifier<<" has failed!"<<std::endl;
		});
	}
}
