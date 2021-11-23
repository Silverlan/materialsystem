/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __MSYS_TEXTURE_LOADER_HPP__
#define __MSYS_TEXTURE_LOADER_HPP__

#include "cmatsysdefinitions.h"
#include <sharedutils/asset_loader/asset_loader.hpp>
#include <sharedutils/ctpl_stl.h>
#include <string>
#include <memory>
#include <functional>
#include <mutex>

#undef AddJob

namespace prosper {class IPrContext;};
namespace ufile {struct IFile;};
namespace msys
{
	class ITextureFormatHandler;
	class DLLCMATSYS TextureLoader
		: public util::IAssetLoader
	{
	public:
		TextureLoader(prosper::IPrContext &context);
		void RegisterFormatHandler(const std::string &ext,const std::function<std::unique_ptr<ITextureFormatHandler>()> &factory);
		bool AddJob(
			prosper::IPrContext &context,const std::string &identifier,const std::string &ext,const std::shared_ptr<ufile::IFile> &file,util::AssetLoadJobPriority priority=0
		);
		void SetAllowMultiThreadedGpuResourceAllocation(bool b) {m_allowMultiThreadedGpuResourceAllocation = b;}
		bool DoesAllowMultiThreadedGpuResourceAllocation() const {return m_allowMultiThreadedGpuResourceAllocation;}
		prosper::IPrContext &GetContext() {return m_context;}
	private:
		bool m_allowMultiThreadedGpuResourceAllocation = true;
		std::unordered_map<std::string,std::function<std::unique_ptr<ITextureFormatHandler>()>> m_formatHandlers;
		prosper::IPrContext &m_context;
	};
};

#endif
