// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

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

bool TextureManager::Load(prosper::IPrContext &context, const std::string &cacheName, pragma::fs::VFilePtr optFile, const LoadInfo &loadInfo, std::shared_ptr<void> *outTexture, bool bAbsolutePath)
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
			pragma::math::set_flag(loadFlags, pragma::material::TextureLoadFlags::Reload, false);
	}
	else
		text = *outTexture;
	if(pragma::math::is_flag_set(loadFlags, pragma::material::TextureLoadFlags::Reload) == false) {
		text = cachedTexture;
		if(text != nullptr) {
			auto bReloadInternalTex = false;
			if(outTexture != nullptr) {
				if(static_cast<pragma::material::Texture *>(text.get())->HasValidVkTexture() == false && bLoading == false)
					bReloadInternalTex = true;
				else
					*outTexture = text;
			}
			if(bReloadInternalTex == false) {
				if(loadInfo.onLoadCallback != nullptr)
					static_cast<pragma::material::Texture *>(text.get())->CallOnLoaded(loadInfo.onLoadCallback);
				return true;
			}
		}
	}
	pragma::material::TextureType type;
	auto bLoadInstantly = (loadInfo.flags & pragma::material::TextureLoadFlags::LoadInstantly) != pragma::material::TextureLoadFlags::None;
	auto dontCache = pragma::math::is_flag_set(loadInfo.flags, pragma::material::TextureLoadFlags::DontCache);
	if(bLoadInstantly == false)
		dontCache = false; // This flag only makes sense if we're loading instantly
	auto found = false;
	auto path = translate_image_path(cacheName, type, (bAbsolutePath == false) ? (MaterialManager::GetRootMaterialLocation() + "/") : "", m_texFileHandler, &found);
	if(found == false) {
		// Attempt to determine by file extension
		auto f = std::dynamic_pointer_cast<pragma::fs::VFilePtrInternalReal>(optFile);
		if(f) {
			std::string ext;
			if(ufile::get_extension(f->GetPath(), &ext)) {
				if(pragma::string::compare<std::string>(ext, "dds", false))
					type = pragma::material::TextureType::DDS;
				else if(pragma::string::compare<std::string>(ext, "ktx", false))
					type = pragma::material::TextureType::KTX;
				else if(pragma::string::compare<std::string>(ext, "png", false))
					type = pragma::material::TextureType::PNG;
				else if(pragma::string::compare<std::string>(ext, "tga", false))
					type = pragma::material::TextureType::TGA;
				else if(pragma::string::compare<std::string>(ext, "vtf", false))
					type = pragma::material::TextureType::VTF;
				else if(pragma::string::compare<std::string>(ext, "vtex_c", false))
					type = pragma::material::TextureType::VTex;
				else if(pragma::string::compare<std::string>(ext, "jpg", false))
					type = pragma::material::TextureType::JPG;
				else if(pragma::string::compare<std::string>(ext, "bmp", false))
					type = pragma::material::TextureType::BMP;
				else if(pragma::string::compare<std::string>(ext, "psd", false))
					type = pragma::material::TextureType::PSD;
				else if(pragma::string::compare<std::string>(ext, "gif", false))
					type = pragma::material::TextureType::GIF;
				else if(pragma::string::compare<std::string>(ext, "hdr", false))
					type = pragma::material::TextureType::HDR;
				else if(pragma::string::compare<std::string>(ext, "pic", false))
					type = pragma::material::TextureType::PIC;
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
	std::unique_ptr<pragma::material::TextureQueueItem> item = nullptr;
	if(type == pragma::material::TextureType::DDS || type == pragma::material::TextureType::KTX)
		item = std::make_unique<pragma::material::TextureQueueItemSurface>(type);
	else if(type == pragma::material::TextureType::PNG)
		item = std::make_unique<pragma::material::TextureQueueItemPNG>();
	else if(type == pragma::material::TextureType::TGA || type == pragma::material::TextureType::JPG || type == pragma::material::TextureType::BMP || type == pragma::material::TextureType::PSD || type == pragma::material::TextureType::GIF || type == pragma::material::TextureType::HDR || type == pragma::material::TextureType::PIC)
		item = std::make_unique<pragma::material::TextureQueueItemStbi>(type);
#ifndef DISABLE_VTF_SUPPORT
	else if(type == pragma::material::TextureType::VTF)
		item = std::make_unique<pragma::material::TextureQueueItemVTF>();
#endif
#ifndef DISABLE_VTEX_SUPPORT
	else if(type == pragma::material::TextureType::VTex)
		item = std::make_unique<pragma::material::TextureQueueItemVTex>();
#endif
	static_assert(pragma::math::to_integral(pragma::material::TextureType::Count) == 13, "Update this implementation when new texture types have been added!");
	item->name = cacheName; // Actual path to file
	item->path = path;
	item->cache = pathCache; // Normalized name without extension
	item->mipmapMode = loadInfo.mipmapLoadMode;
	item->sampler = loadInfo.sampler;
	item->addToCache = !dontCache;
	item->file = optFile;

	auto bReload = pragma::math::is_flag_set(loadFlags, pragma::material::TextureLoadFlags::Reload);
	if(text == nullptr) // bReload == false)
		text = std::make_shared<pragma::material::Texture>(GetContext());
	auto *ptrTexture = static_cast<pragma::material::Texture *>(text.get());
	ptrTexture->SetFlags(ptrTexture->GetFlags() & ~pragma::material::Texture::Flags::Loaded);
	if(loadInfo.onLoadCallback != nullptr)
		ptrTexture->CallOnLoaded(loadInfo.onLoadCallback);
	ptrTexture->ClearVkTexture();
	ptrTexture->SetName(item->cache);
	{
		pragma::util::Path filePath {item->path};
		std::string rpath;
		if(pragma::fs::find_absolute_path(filePath.GetString(), rpath)) {
			filePath = {rpath};
			filePath.MakeRelative(pragma::util::Path::CreatePath(pragma::fs::FileManager::GetProgramPath()) + MaterialManager::GetRootMaterialLocation());
		}
		ptrTexture->SetFileInfo(filePath, type);
	}
	item->context = context.shared_from_this();

	m_texturesTmp.push_back(std::static_pointer_cast<pragma::material::Texture>(text));
	if(outTexture != nullptr)
		*outTexture = text;
	if(!pragma::fs::exists(path))
		bLoadInstantly = true; // No point in queueing it if the texture doesn't even exist
	if(bLoadInstantly == true) {
		InitializeTextureData(*item);
		InitializeImage(*item);
		bool bGenMipmaps = true;
		auto *surface = dynamic_cast<pragma::material::TextureQueueItemSurface *>(item.get());
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

bool TextureManager::Load(prosper::IPrContext &context, const std::string &cacheName, pragma::fs::VFilePtr f, const LoadInfo &loadInfo, std::shared_ptr<void> *outTexture) { return Load(context, cacheName, f, loadInfo, outTexture, false); }
bool TextureManager::Load(prosper::IPrContext &context, const std::string &imgFile, const LoadInfo &loadInfo, std::shared_ptr<void> *outTexture, bool bAbsolutePath) { return Load(context, imgFile, nullptr, loadInfo, outTexture, bAbsolutePath); }
