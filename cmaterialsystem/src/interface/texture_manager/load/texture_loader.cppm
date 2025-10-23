// SPDX-FileCopyrightText: (c) 2021 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "cmatsysdefinitions.hpp"
#include <string>
#include <memory>
#include <functional>
#include <mutex>

export module pragma.cmaterialsystem:texture_manager.texture_loader;

export import :texture_manager.texture_processor;

#undef AddJob

export namespace msys {
	DLLCMATSYS void setup_sampler_mipmap_mode(prosper::util::SamplerCreateInfo &createInfo, TextureMipmapMode mode);
	class DLLCMATSYS TextureLoader : public util::TAssetFormatLoader<TextureProcessor> {
	  public:
		TextureLoader(util::IAssetManager &assetManager, prosper::IPrContext &context);
		void SetAllowMultiThreadedGpuResourceAllocation(bool b) { m_allowMultiThreadedGpuResourceAllocation = b; }
		bool DoesAllowMultiThreadedGpuResourceAllocation() const { return m_allowMultiThreadedGpuResourceAllocation; }
		prosper::IPrContext &GetContext() { return m_context; }

		const std::shared_ptr<prosper::ISampler> &GetTextureSampler() const { return m_textureSampler; }
		const std::shared_ptr<prosper::ISampler> &GetTextureSamplerNoMipmap() const { return m_textureSamplerNoMipmap; }
	  private:
		bool m_allowMultiThreadedGpuResourceAllocation = true;
		prosper::IPrContext &m_context;

		std::shared_ptr<prosper::ISampler> m_textureSampler;
		std::shared_ptr<prosper::ISampler> m_textureSamplerNoMipmap;
	};
};
