/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __TEXTUREMANAGER_H__
#define __TEXTUREMANAGER_H__

#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <fsys/filesystem.h>
#include "cmatsysdefinitions.h"
#include "texture.h"
#include "texture_load_flags.hpp"
#include <sharedutils/functioncallback.h>

namespace prosper
{
	class Sampler;
};

class TCallback;
class TextureQueueItem;
class Texture;
enum class TextureMipmapMode : int32_t;
#pragma warning(push)
#pragma warning(disable : 4251)
class DLLCMATSYS TextureManager
{
public:
	TextureManager(const TextureManager&)=delete;
	TextureManager &operator=(const TextureManager&)=delete;
	static const uint32_t MAX_TEXTURE_COUNT;

	struct DLLCMATSYS LoadInfo
	{
		LoadInfo();
		std::function<void(std::shared_ptr<Texture>)> onLoadCallback = nullptr;
		TextureMipmapMode mipmapLoadMode;
		std::shared_ptr<prosper::Sampler> sampler = nullptr;
		TextureLoadFlags flags = TextureLoadFlags::None;
	};
private:
	std::weak_ptr<prosper::Context> m_wpContext;
	std::vector<std::shared_ptr<Texture>> m_textures;
	std::unique_ptr<std::thread> m_threadLoad;
	std::unique_ptr<std::mutex> m_queueMutex;
	std::unique_ptr<std::mutex> m_lockMutex;
	std::unique_ptr<std::mutex> m_activeMutex;
	std::unique_ptr<std::mutex> m_loadMutex;
	std::unique_ptr<std::condition_variable> m_queueVar;
	std::queue<std::shared_ptr<TextureQueueItem>> m_loadQueue;
	std::queue<std::shared_ptr<TextureQueueItem>> m_initQueue;
	std::vector<std::shared_ptr<Texture>> m_texturesTmp;
	std::shared_ptr<prosper::Sampler> m_textureSampler;
	std::shared_ptr<prosper::Sampler> m_textureSamplerNoMipmap;
	std::vector<std::weak_ptr<prosper::Sampler>> m_customSamplers;
	std::function<VFilePtr(const std::string&)> m_texFileHandler = nullptr;
	std::shared_ptr<Texture> m_error;
	bool m_bThreadActive;
	void TextureThread();
	bool CheckTextureThreadLocked();
	std::shared_ptr<Texture> FindTexture(const std::string &imgFile,std::string *cache,bool *bLoading=nullptr);
	void InitializeTextureData(TextureQueueItem &item);
	void InitializeImage(TextureQueueItem &item);
	void PushOnLoadQueue(std::unique_ptr<TextureQueueItem> item);
	std::shared_ptr<Texture> GetQueuedTexture(TextureQueueItem &item,bool bErase=false);
	void FinalizeTexture(TextureQueueItem &item);
	void ReloadTexture(uint32_t texId,const LoadInfo &loadInfo);
	VFilePtr OpenTextureFile(const std::string &fpath);
public:
	static void SetupSamplerMipmapMode(prosper::util::SamplerCreateInfo &createInfo,TextureMipmapMode mode);
	TextureManager(prosper::Context &context);
	~TextureManager();
	prosper::Context &GetContext() const;
	void Update();
	void RegisterCustomSampler(const std::shared_ptr<prosper::Sampler> &sampler);
	const std::vector<std::weak_ptr<prosper::Sampler>> &GetCustomSamplers() const;
	std::shared_ptr<Texture> GetErrorTexture();
	std::shared_ptr<Texture> FindTexture(const std::string &imgFile,bool *bLoading=nullptr);
	std::shared_ptr<Texture> FindTexture(const std::string &imgFile,bool bLoadedOnly);
	std::shared_ptr<Texture> GetTexture(const std::string &name);
	void SetErrorTexture(const std::shared_ptr<Texture> &tex);
	std::shared_ptr<Texture> CreateTexture(const std::string &name,prosper::Texture &texture);
	//std::shared_ptr<Texture> CreateTexture(prosper::Context &context,const std::string &name,unsigned int w,unsigned int h);
	void SetTextureFileHandler(const std::function<VFilePtr(const std::string&)> &fileHandler);

	bool Load(prosper::Context &context,const std::string &imgFile,const LoadInfo &loadInfo,std::shared_ptr<void> *outTexture=nullptr,bool bAbsolutePath=false);
	void ReloadTextures(const LoadInfo &loadInfo);
	void ReloadTexture(Texture &texture,const LoadInfo &loadInfo);
	void ReloadTexture(const std::string &tex,const LoadInfo &loadInfo);
	void Clear();
	void ClearUnused();
	std::shared_ptr<prosper::Sampler> &GetTextureSampler();
};
#pragma warning(pop)

#endif