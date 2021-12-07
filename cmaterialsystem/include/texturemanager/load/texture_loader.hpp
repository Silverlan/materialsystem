/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __MSYS_TEXTURE_LOADER_HPP__
#define __MSYS_TEXTURE_LOADER_HPP__

#include "cmatsysdefinitions.h"
#include "texturemanager/load/texture_processor.hpp"
#include <sharedutils/asset_loader/asset_format_loader.hpp>
#include <sharedutils/ctpl_stl.h>
#include <string>
#include <memory>
#include <functional>
#include <mutex>

#undef AddJob

namespace prosper {class IPrContext; class ISampler; namespace util {struct SamplerCreateInfo;}; class IAssetProcessor;};
namespace ufile {struct IFile;};
enum class TextureMipmapMode : int32_t;
namespace msys
{
	DLLCMATSYS void setup_sampler_mipmap_mode(prosper::util::SamplerCreateInfo &createInfo,TextureMipmapMode mode);
	class DLLCMATSYS TextureLoader
		: public util::TAssetFormatLoader<TextureProcessor>
	{
	public:
		TextureLoader(util::IAssetManager &assetManager,prosper::IPrContext &context);
		void SetAllowMultiThreadedGpuResourceAllocation(bool b) {m_allowMultiThreadedGpuResourceAllocation = b;}
		bool DoesAllowMultiThreadedGpuResourceAllocation() const {return m_allowMultiThreadedGpuResourceAllocation;}
		prosper::IPrContext &GetContext() {return m_context;}

		const std::shared_ptr<prosper::ISampler> &GetTextureSampler() const {return m_textureSampler;}
		const std::shared_ptr<prosper::ISampler> &GetTextureSamplerNoMipmap() const {return m_textureSamplerNoMipmap;}
	private:
		bool m_allowMultiThreadedGpuResourceAllocation = true;
		prosper::IPrContext &m_context;

		std::shared_ptr<prosper::ISampler> m_textureSampler;
		std::shared_ptr<prosper::ISampler> m_textureSamplerNoMipmap;
	};
};

#endif
