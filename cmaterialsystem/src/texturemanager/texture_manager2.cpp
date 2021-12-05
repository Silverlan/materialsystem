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
#pragma optimize("",off)
// #define ENABLE_VERBOSE_OUTPUT

bool msys::TextureAsset::IsInUse() const {return texture.use_count() > 1;}

/////////////

msys::TextureLoadInfo::TextureLoadInfo(TextureLoadFlags flags)
	: flags{flags},mipmapMode{TextureMipmapMode::LoadOrGenerate}
{}
msys::TextureManager::TextureManager(prosper::IPrContext &context)
	: m_context{context}
{
	SetFileHandler([](const std::string &fileName) -> std::shared_ptr<ufile::IFile> {
		auto fp = filemanager::open_file(fileName,filemanager::FileMode::Binary | filemanager::FileMode::Read);
		if(!fp)
			return nullptr;
		return std::make_shared<fsys::File>(fp);
	});

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

void msys::TextureManager::SetFileHandler(const std::function<std::shared_ptr<ufile::IFile>(const std::string&)> &fileHandler) {m_fileHandler = fileHandler;}

void msys::TextureManager::RemoveFromCache(const std::string &path)
{
	IAssetManager::RemoveFromCache(path);
	m_loader->InvalidateLoadJob(path);
}
msys::TextureManager::PreloadResult msys::TextureManager::PreloadTexture(const std::string &strPath,const TextureLoadInfo &loadInfo) {return PreloadTexture(strPath,DEFAULT_PRIORITY,loadInfo);}
msys::TextureManager::PreloadResult msys::TextureManager::PreloadTexture(
	const std::string &cacheName,const std::shared_ptr<ufile::IFile> &file,const std::string &ext,util::AssetLoadJobPriority priority,
	const TextureLoadInfo &loadInfo
)
{
	auto jobId = m_loader->AddJob(ToCacheIdentifier(cacheName),ext,file,priority,[&loadInfo](TextureProcessor &processor) {
		processor.userData = std::make_shared<TextureLoadInfo>(loadInfo);
		processor.mipmapMode = loadInfo.mipmapMode;
		processor.onLoaded = [&processor](msys::TextureAsset *asset) {
			auto &loadInfo = *static_cast<TextureLoadInfo*>(processor.userData.get());
			if(asset)
			{
				if(loadInfo.onLoaded)
					loadInfo.onLoaded(*asset);
			}
			else if(loadInfo.onFailure)
				loadInfo.onFailure();
		};
	});
	if(!jobId.has_value())
		return PreloadResult{{},false};
	return PreloadResult{jobId,true};
}
msys::TextureManager::PreloadResult msys::TextureManager::PreloadTexture(const std::string &strPath,util::AssetLoadJobPriority priority,const TextureLoadInfo &loadInfo)
{
	auto *asset = FindCachedAsset(strPath);
	if(asset)
		return PreloadResult{{},true};
	auto path = util::Path::CreateFile(strPath);
	if(!umath::is_flag_set(loadInfo.flags,TextureLoadFlags::AbsolutePath))
		path = m_rootDir +path;
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
	auto f = m_fileHandler(path.GetString());
	if(!f)
		return PreloadResult{{},false};
	return PreloadTexture(strPath,f,*ext,priority,loadInfo);
}
std::shared_ptr<Texture> msys::TextureManager::LoadTexture(const std::string &path,const PreloadResult &r,const TextureLoadInfo &loadInfo)
{
	if(r.success == false)
	{
		if(loadInfo.onFailure)
			loadInfo.onFailure();
		return nullptr;
	}
	if(!r.jobId.has_value())
	{
		// Already fully loaded
		auto *asset = static_cast<TextureAsset*>(FindCachedAsset(path));
		if(loadInfo.onLoaded)
			loadInfo.onLoaded(*asset);
		return asset->texture;
	}
	auto identifier = ToCacheIdentifier(path);
	std::shared_ptr<prosper::Texture> texture = nullptr;
	auto jobId = *r.jobId;

	auto *asset = Poll(jobId);
	if(!asset)
		return nullptr;
	return asset->texture;
}

void msys::TextureManager::Poll() {Poll({});}

msys::TextureAsset *msys::TextureManager::Poll(std::optional<util::AssetLoadJobId> untilJob)
{
	TextureAsset *targetAsset = nullptr;
	do
	{
		m_loader->Poll([this,&untilJob,&targetAsset](const util::AssetLoadJob &job) {
#ifdef ENABLE_VERBOSE_OUTPUT
			auto dtQueue = job.completionTime -job.queueStartTime;
			auto dtTask = job.completionTime -job.taskStartTime;
			std::cout<<job.identifier<<" has been loaded!"<<std::endl;
			std::cout<<"Time since job has been queued to completion: "<<(dtQueue.count() /1'000'000'000.0)<<std::endl;
			std::cout<<"Time since task has been started to completion: "<<(dtTask.count() /1'000'000'000.0)<<std::endl;
#endif
			auto &texProcessor = *static_cast<TextureProcessor*>(job.processor.get());
			auto texture = texProcessor.texture;

			auto texWrapper = std::make_shared<Texture>(m_context,texture);
	
			auto &img = texture->GetImage();
			auto flags = texWrapper->GetFlags();
			umath::set_flag(flags,Texture::Flags::SRGB,img.IsSrgb());
			flags |= Texture::Flags::Indexed | Texture::Flags::Loaded;
			flags &= ~Texture::Flags::Error;
			texWrapper->SetFlags(flags);
			texWrapper->SetName(job.identifier);

			auto asset = std::make_shared<TextureAsset>(texWrapper);
			auto &loadInfo = *static_cast<TextureLoadInfo*>(texProcessor.userData.get());
			if(!umath::is_flag_set(loadInfo.flags,TextureLoadFlags::DontCache))
				AddToCache(job.identifier,asset);
			if(untilJob.has_value() && job.jobId == *untilJob)
			{
				targetAsset = asset.get();
				untilJob = {};
			}
			if(texProcessor.onLoaded)
				texProcessor.onLoaded(asset.get());
		},[&untilJob,&targetAsset](const util::AssetLoadJob &job) {
#ifdef ENABLE_VERBOSE_OUTPUT
			std::cout<<job.identifier<<" has failed!"<<std::endl;
#endif

			auto &texProcessor = *static_cast<TextureProcessor*>(job.processor.get());
			if(untilJob.has_value() && job.jobId == *untilJob)
			{
				targetAsset = nullptr;
				untilJob = {};
			}
			if(texProcessor.onLoaded)
				texProcessor.onLoaded(nullptr);
		});
	}
	while(untilJob.has_value());
	return targetAsset;
}

std::shared_ptr<Texture> msys::TextureManager::LoadTexture(
	const std::string &cacheName,const std::shared_ptr<ufile::IFile> &file,const std::string &ext,const TextureLoadInfo &loadInfo
)
{
	auto r = PreloadTexture(cacheName,file,ext,IMMEDIATE_PRIORITY,loadInfo);
	if(r.success == false)
		return nullptr;
	return LoadTexture(cacheName,r,loadInfo);
}
std::shared_ptr<Texture> msys::TextureManager::LoadTexture(const std::string &path,const TextureLoadInfo &loadInfo)
{
	return LoadTexture(path,PreloadTexture(path,IMMEDIATE_PRIORITY,loadInfo),loadInfo);
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
#pragma optimize("",on)
