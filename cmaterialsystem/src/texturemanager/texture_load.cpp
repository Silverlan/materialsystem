/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "texturemanager/texturemanager.h"
#include "texturemanager/texturequeue.h"
#include "textureinfo.h"
#include "impl_texture_formats.h"
#include <prosper_context.hpp>
#include <sharedutils/util_string.h>
#include <sharedutils/util_file.h>
#include <map>

void TextureManager::ReloadTexture(const std::string &tex,const LoadInfo &loadInfo)
{
	std::string ext;
	std::string imgFileNoExt = tex;
	if(ufile::get_extension(imgFileNoExt,&ext) == true)
		imgFileNoExt = imgFileNoExt.substr(0,imgFileNoExt.length() -ext.length() -1);
	auto bLoading = false;
	std::string pathCache;
	auto tCur = FindTexture(imgFileNoExt,&pathCache,&bLoading);
	if(tCur == nullptr)
		return; // TODO: What if texture is currently still loading?
	ReloadTexture(*tCur,loadInfo);
}

bool TextureManager::Load(prosper::Context &context,const std::string &imgFile,const LoadInfo &loadInfo,std::shared_ptr<void> *outTexture,bool bAbsolutePath)
{
	std::string pathCache;
	std::shared_ptr<void> text = nullptr;

	// Get cache name and path
	std::string ext;
	std::string imgFileNoExt = imgFile;
	if(ufile::get_extension(imgFileNoExt,&ext) == true)
		imgFileNoExt = imgFileNoExt.substr(0,imgFileNoExt.length() -ext.length() -1);
	auto bLoading = false;
	auto cachedTexture = FindTexture(imgFileNoExt,&pathCache,&bLoading);

	ext.clear();
	if(ufile::get_extension(pathCache,&ext) == true)
		pathCache = pathCache.substr(0,pathCache.length() -ext.length() -1);
	//

	auto loadFlags = loadInfo.flags;
	if(outTexture == nullptr || *outTexture == nullptr)
	{
		text = cachedTexture;
		if(text == nullptr)
			umath::set_flag(loadFlags,TextureLoadFlags::Reload,false);
	}
	else
		text = *outTexture;
	if(umath::is_flag_set(loadFlags,TextureLoadFlags::Reload) == false)
	{
		text = cachedTexture;
		if(text != NULL)
		{
			auto bReloadInternalTex = false;
			if(outTexture != NULL)
			{
				if(static_cast<Texture*>(text.get())->HasValidVkTexture() == false && bLoading == false)
					bReloadInternalTex = true;
				else
					*outTexture = text;
			}
			if(bReloadInternalTex == false)
			{
				if(loadInfo.onLoadCallback != nullptr)
					static_cast<Texture*>(text.get())->CallOnLoaded(loadInfo.onLoadCallback);
				return true;
			}
		}
	}
	TextureType type;
	auto bLoadInstantly = (loadInfo.flags &TextureLoadFlags::LoadInstantly) != TextureLoadFlags::None;
	auto path = translate_image_path(imgFile,type,(bAbsolutePath == false) ? (MaterialManager::GetRootMaterialLocation() +"/") : "",m_texFileHandler);
	//if(bReload == true)
	//	bLoadInstantly = true;
	if(bLoadInstantly == false && m_threadLoad == nullptr)
	{
		m_activeMutex = std::make_unique<std::mutex>();
		m_bThreadActive = true;
		m_loadMutex = std::make_unique<std::mutex>();

		m_queueVar = std::make_unique<std::condition_variable>();
		m_queueMutex = std::make_unique<std::mutex>();
		m_lockMutex = std::make_unique<std::mutex>();
		m_threadLoad = std::make_unique<std::thread>(std::bind(&TextureManager::TextureThread,this));
	}
	std::unique_ptr<TextureQueueItem> item = nullptr;
	if(type == TextureType::DDS || type == TextureType::KTX)
		item = std::make_unique<TextureQueueItemSurface>(type);
	else if(type == TextureType::PNG)
		item = std::make_unique<TextureQueueItemPNG>();
	else if(type == TextureType::TGA)
		item = std::make_unique<TextureQueueItemTGA>();
#ifndef DISABLE_VTF_SUPPORT
	else if(type == TextureType::VTF)
		item = std::make_unique<TextureQueueItemVTF>();
#endif
#ifndef DISABLE_VTEX_SUPPORT
	else if(type == TextureType::VTex)
		item = std::make_unique<TextureQueueItemVTex>();
#endif
	item->name = imgFile; // Actual path to file
	item->path = path;
	item->cache = pathCache; // Normalized name without extension
	item->mipmapMode = loadInfo.mipmapLoadMode;
	item->sampler = loadInfo.sampler;

	auto bReload = umath::is_flag_set(loadFlags,TextureLoadFlags::Reload);
	if(bReload == false)
		text = std::make_shared<Texture>(GetContext());
	auto *ptrTexture = static_cast<Texture*>(text.get());
	ptrTexture->SetFlags(ptrTexture->GetFlags() &~Texture::Flags::Loaded);
	if(loadInfo.onLoadCallback != nullptr)
		ptrTexture->CallOnLoaded(loadInfo.onLoadCallback);
	ptrTexture->ClearVkTexture();
	ptrTexture->SetName(item->cache);
	item->context = context.shared_from_this();

	m_texturesTmp.push_back(std::static_pointer_cast<Texture>(text));
	if(outTexture != nullptr)
		*outTexture = text;
	if(!FileManager::Exists(path.c_str()))
		bLoadInstantly = true; // No point in queuing it if the texture doesn't even exist
	if(bLoadInstantly == true)
	{
		InitializeTextureData(*item);
		InitializeImage(*item);
		bool bGenMipmaps = true;
		auto *surface = dynamic_cast<TextureQueueItemSurface*>(item.get());
		if(surface != nullptr)
		{
			if(!surface->cubemap)
			{
				//if(dds->ddsimg[0]->get_num_mipmaps() > 0)
				//	bGenMipmaps = false;
			}
		}
		if(item->valid == true)
		{
			if(bGenMipmaps)// && InitializeMipmaps(item) == true)
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
