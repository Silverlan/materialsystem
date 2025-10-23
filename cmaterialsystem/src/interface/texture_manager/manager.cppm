// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <unordered_map>
#include <vector>
#include <string>
#include <algorithm>
#include <thread>
#include <mutex>
#include <queue>
#include <atomic>
#include <condition_variable>
#include "cmatsysdefinitions.hpp"

export module pragma.cmaterialsystem:texture_manager.manager;

export import :texture_manager.texture;
export import :texture_manager.texture_queue;

export {
	#pragma warning(push)
	#pragma warning(disable : 4251)
	class DLLCMATSYS TextureManager {
	public:
		TextureManager(const TextureManager &) = delete;
		TextureManager &operator=(const TextureManager &) = delete;
		static const uint32_t MAX_TEXTURE_COUNT;

		struct DLLCMATSYS LoadInfo {
			LoadInfo();
			CallbackHandle onLoadCallback {};
			msys::TextureMipmapMode mipmapLoadMode;
			std::shared_ptr<prosper::ISampler> sampler = nullptr;
			msys::TextureLoadFlags flags = msys::TextureLoadFlags::None;
		};
	public:
		static void SetupSamplerMipmapMode(prosper::util::SamplerCreateInfo &createInfo, msys::TextureMipmapMode mode);
		TextureManager(prosper::IPrContext &context);
		~TextureManager();
		prosper::IPrContext &GetContext() const;
		void Update();
		void RegisterCustomSampler(const std::shared_ptr<prosper::ISampler> &sampler);
		const std::vector<std::weak_ptr<prosper::ISampler>> &GetCustomSamplers() const;
		std::shared_ptr<msys::Texture> GetErrorTexture();
		std::shared_ptr<msys::Texture> FindTexture(const std::string &imgFile, bool *bLoading = nullptr);
		std::shared_ptr<msys::Texture> FindTexture(const std::string &imgFile, bool bLoadedOnly);
		std::shared_ptr<msys::Texture> GetTexture(const std::string &name);
		void SetErrorTexture(const std::shared_ptr<msys::Texture> &tex);
		std::shared_ptr<msys::Texture> CreateTexture(const std::string &name, prosper::Texture &texture);
		//std::shared_ptr<Texture> CreateTexture(prosper::Context &context,const std::string &name,unsigned int w,unsigned int h);
		void SetTextureFileHandler(const std::function<VFilePtr(const std::string &)> &fileHandler);

		void WaitForTextures();
		bool Load(prosper::IPrContext &context, const std::string &cacheName, VFilePtr f, const LoadInfo &loadInfo, std::shared_ptr<void> *outTexture = nullptr);
		bool Load(prosper::IPrContext &context, const std::string &imgFile, const LoadInfo &loadInfo, std::shared_ptr<void> *outTexture = nullptr, bool bAbsolutePath = false);
		void ReloadTextures(const LoadInfo &loadInfo);
		void ReloadTexture(msys::Texture &texture, const LoadInfo &loadInfo);
		void ReloadTexture(const std::string &tex, const LoadInfo &loadInfo);
		uint32_t Clear();
		uint32_t ClearUnused();
		std::shared_ptr<prosper::ISampler> &GetTextureSampler();
		const std::vector<std::shared_ptr<msys::Texture>> &GetTextures() const { return m_textures; }
	private:
		bool Load(prosper::IPrContext &context, const std::string &cacheName, VFilePtr optFile, const LoadInfo &loadInfo, std::shared_ptr<void> *outTexture, bool bAbsolutePath);

		bool HasWork();
		std::weak_ptr<prosper::IPrContext> m_wpContext;
		std::vector<std::shared_ptr<msys::Texture>> m_textures;
		std::atomic<bool> m_bBusy = false;
		std::unique_ptr<std::thread> m_threadLoad;
		std::unique_ptr<std::mutex> m_queueMutex;
		std::unique_ptr<std::mutex> m_lockMutex;
		std::unique_ptr<std::mutex> m_activeMutex;
		std::unique_ptr<std::mutex> m_loadMutex;
		std::unique_ptr<std::condition_variable> m_queueVar;
		std::queue<std::shared_ptr<TextureQueueItem>> m_loadQueue;
		std::queue<std::shared_ptr<TextureQueueItem>> m_initQueue;
		std::vector<std::shared_ptr<msys::Texture>> m_texturesTmp;
		std::shared_ptr<prosper::ISampler> m_textureSampler;
		std::shared_ptr<prosper::ISampler> m_textureSamplerNoMipmap;
		std::vector<std::weak_ptr<prosper::ISampler>> m_customSamplers;
		std::function<VFilePtr(const std::string &)> m_texFileHandler = nullptr;
		std::shared_ptr<msys::Texture> m_error;
		bool m_bThreadActive;
		void TextureThread();
		bool CheckTextureThreadLocked();
		std::shared_ptr<msys::Texture> FindTexture(const std::string &imgFile, std::string *cache, bool *bLoading = nullptr);
		void InitializeTextureData(TextureQueueItem &item);
		void InitializeImage(TextureQueueItem &item);
		void PushOnLoadQueue(std::unique_ptr<TextureQueueItem> item);
		std::shared_ptr<msys::Texture> GetQueuedTexture(TextureQueueItem &item, bool bErase = false);
		void FinalizeTexture(TextureQueueItem &item);
		void ReloadTexture(uint32_t texId, const LoadInfo &loadInfo);
		VFilePtr OpenTextureFile(const std::string &fpath);
	};
	#pragma warning(pop)
}
