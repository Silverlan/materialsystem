// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <memory>
#include <functional>
#include <string>

module pragma.cmaterialsystem;

import :texture_manager;

void TextureManager::ReloadTexture(const std::string &tex, const LoadInfo &loadInfo)
{
	std::string ext;
	std::string imgFileNoExt = tex;
	if(ufile::get_extension(imgFileNoExt, &ext) == true)
		imgFileNoExt = imgFileNoExt.substr(0, imgFileNoExt.length() - ext.length() - 1);
	auto bLoading = false;
	std::string pathCache;
	auto tCur = FindTexture(imgFileNoExt, &pathCache, &bLoading);
	if(tCur == nullptr)
		return; // TODO: What if texture is currently still loading?
	ReloadTexture(*tCur, loadInfo);
}

bool TextureManager::Load(prosper::IPrContext &context, const std::string &cacheName, VFilePtr optFile, const LoadInfo &loadInfo, std::shared_ptr<void> *outTexture, bool bAbsolutePath)
{
	std::string pathCache;
	std::shared_ptr<void> text = nullptr;

	// Get cache name and path
	std::string ext;
	std::string imgFileNoExt = cacheName;
	if(ufile::get_extension(imgFileNoExt, &ext) == true)
		imgFileNoExt = imgFileNoExt.substr(0, imgFileNoExt.length() - ext.length() - 1);
	auto bLoading = false;
	auto cachedTexture = FindTexture(imgFileNoExt, &pathCache, &bLoading);

	ext.clear();
	if(ufile::get_extension(pathCache, &ext) == true)
		pathCache = pathCache.substr(0, pathCache.length() - ext.length() - 1);
	//

	auto loadFlags = loadInfo.flags;
	if(outTexture == nullptr || *outTexture == nullptr) {
		text = cachedTexture;
		if(text == nullptr)
			umath::set_flag(loadFlags, msys::TextureLoadFlags::Reload, false);
	}
	else
		text = *outTexture;
	if(umath::is_flag_set(loadFlags, msys::TextureLoadFlags::Reload) == false) {
		text = cachedTexture;
		if(text != NULL) {
			auto bReloadInternalTex = false;
			if(outTexture != NULL) {
				if(static_cast<msys::Texture *>(text.get())->HasValidVkTexture() == false && bLoading == false)
					bReloadInternalTex = true;
				else
					*outTexture = text;
			}
			if(bReloadInternalTex == false) {
				if(loadInfo.onLoadCallback != nullptr)
					static_cast<msys::Texture *>(text.get())->CallOnLoaded(loadInfo.onLoadCallback);
				return true;
			}
		}
	}
	msys::TextureType type;
	auto bLoadInstantly = (loadInfo.flags & msys::TextureLoadFlags::LoadInstantly) != msys::TextureLoadFlags::None;
	auto dontCache = umath::is_flag_set(loadInfo.flags, msys::TextureLoadFlags::DontCache);
	if(bLoadInstantly == false)
		dontCache = false; // This flag only makes sense if we're loading instantly
	auto found = false;
	auto path = translate_image_path(cacheName, type, (bAbsolutePath == false) ? (MaterialManager::GetRootMaterialLocation() + "/") : "", m_texFileHandler, &found);
	if(found == false) {
		// Attempt to determine by file extension
		auto f = std::dynamic_pointer_cast<VFilePtrInternalReal>(optFile);
		if(f) {
			std::string ext;
			if(ufile::get_extension(f->GetPath(), &ext)) {
				if(ustring::compare<std::string>(ext, "dds", false))
					type = msys::TextureType::DDS;
				else if(ustring::compare<std::string>(ext, "ktx", false))
					type = msys::TextureType::KTX;
				else if(ustring::compare<std::string>(ext, "png", false))
					type = msys::TextureType::PNG;
				else if(ustring::compare<std::string>(ext, "tga", false))
					type = msys::TextureType::TGA;
				else if(ustring::compare<std::string>(ext, "vtf", false))
					type = msys::TextureType::VTF;
				else if(ustring::compare<std::string>(ext, "vtex_c", false))
					type = msys::TextureType::VTex;
				else if(ustring::compare<std::string>(ext, "jpg", false))
					type = msys::TextureType::JPG;
				else if(ustring::compare<std::string>(ext, "bmp", false))
					type = msys::TextureType::BMP;
				else if(ustring::compare<std::string>(ext, "psd", false))
					type = msys::TextureType::PSD;
				else if(ustring::compare<std::string>(ext, "gif", false))
					type = msys::TextureType::GIF;
				else if(ustring::compare<std::string>(ext, "hdr", false))
					type = msys::TextureType::HDR;
				else if(ustring::compare<std::string>(ext, "pic", false))
					type = msys::TextureType::PIC;
			}
		}
	}
	//if(bReload == true)
	//	bLoadInstantly = true;
	if(bLoadInstantly == false && m_threadLoad == nullptr) {
		m_activeMutex = std::make_unique<std::mutex>();
		m_bThreadActive = true;
		m_loadMutex = std::make_unique<std::mutex>();

		m_queueVar = std::make_unique<std::condition_variable>();
		m_queueMutex = std::make_unique<std::mutex>();
		m_lockMutex = std::make_unique<std::mutex>();
		m_threadLoad = std::make_unique<std::thread>(std::bind(&TextureManager::TextureThread, this));
	}
	std::unique_ptr<TextureQueueItem> item = nullptr;
	if(type == msys::TextureType::DDS || type == msys::TextureType::KTX)
		item = std::make_unique<TextureQueueItemSurface>(type);
	else if(type == msys::TextureType::PNG)
		item = std::make_unique<TextureQueueItemPNG>();
	else if(type == msys::TextureType::TGA || type == msys::TextureType::JPG || type == msys::TextureType::BMP || type == msys::TextureType::PSD || type == msys::TextureType::GIF || type == msys::TextureType::HDR || type == msys::TextureType::PIC)
		item = std::make_unique<TextureQueueItemStbi>(type);
#ifndef DISABLE_VTF_SUPPORT
	else if(type == msys::TextureType::VTF)
		item = std::make_unique<TextureQueueItemVTF>();
#endif
#ifndef DISABLE_VTEX_SUPPORT
	else if(type == msys::TextureType::VTex)
		item = std::make_unique<TextureQueueItemVTex>();
#endif
	static_assert(umath::to_integral(msys::TextureType::Count) == 13, "Update this implementation when new texture types have been added!");
	item->name = cacheName; // Actual path to file
	item->path = path;
	item->cache = pathCache; // Normalized name without extension
	item->mipmapMode = loadInfo.mipmapLoadMode;
	item->sampler = loadInfo.sampler;
	item->addToCache = !dontCache;
	item->file = optFile;

	auto bReload = umath::is_flag_set(loadFlags, msys::TextureLoadFlags::Reload);
	if(text == nullptr) // bReload == false)
		text = std::make_shared<msys::Texture>(GetContext());
	auto *ptrTexture = static_cast<msys::Texture *>(text.get());
	ptrTexture->SetFlags(ptrTexture->GetFlags() & ~msys::Texture::Flags::Loaded);
	if(loadInfo.onLoadCallback != nullptr)
		ptrTexture->CallOnLoaded(loadInfo.onLoadCallback);
	ptrTexture->ClearVkTexture();
	ptrTexture->SetName(item->cache);
	{
		util::Path filePath {item->path};
		std::string rpath;
		if(FileManager::FindAbsolutePath(filePath.GetString(), rpath)) {
			filePath = {rpath};
			filePath.MakeRelative(util::Path::CreatePath(FileManager::GetProgramPath()) + MaterialManager::GetRootMaterialLocation());
		}
		ptrTexture->SetFileInfo(filePath, type);
	}
	item->context = context.shared_from_this();

	m_texturesTmp.push_back(std::static_pointer_cast<msys::Texture>(text));
	if(outTexture != nullptr)
		*outTexture = text;
	if(!FileManager::Exists(path.c_str()))
		bLoadInstantly = true; // No point in queueing it if the texture doesn't even exist
	if(bLoadInstantly == true) {
		InitializeTextureData(*item);
		InitializeImage(*item);
		bool bGenMipmaps = true;
		auto *surface = dynamic_cast<TextureQueueItemSurface *>(item.get());
		if(surface != nullptr) {
			if(!surface->cubemap) {
				//if(dds->ddsimg[0]->get_num_mipmaps() > 0)
				//	bGenMipmaps = false;
			}
		}
		if(item->valid == true) {
			if(bGenMipmaps) // && InitializeMipmaps(item) == true)
			{
				/*item->mipmapid = 0;
				glBindTexture(GL_TEXTURE_2D,text->ID);
				while(item->mipmapid != -1)
				{
				GenerateNextMipmap(item);
				InitializeGLMipmap(item);
				}*/ // Generate on the CPU
			}
		}
		FinalizeTexture(*item);
		return true;
	}
	PushOnLoadQueue(std::move(item));
	return false;
}

bool TextureManager::Load(prosper::IPrContext &context, const std::string &cacheName, VFilePtr f, const LoadInfo &loadInfo, std::shared_ptr<void> *outTexture) { return Load(context, cacheName, f, loadInfo, outTexture, false); }
bool TextureManager::Load(prosper::IPrContext &context, const std::string &imgFile, const LoadInfo &loadInfo, std::shared_ptr<void> *outTexture, bool bAbsolutePath) { return Load(context, imgFile, nullptr, loadInfo, outTexture, bAbsolutePath); }
