/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __MSYS_TEXTURE_LOADER_HPP__
#define __MSYS_TEXTURE_LOADER_HPP__

#include "cmatsysdefinitions.h"
#include "texturemanager/load/texture_load_job.hpp"
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
	class DLLCMATSYS TextureLoader
	{
	public:
		TextureLoader();
		~TextureLoader();

		void Poll(prosper::IPrContext &context);
		bool AddJob(prosper::IPrContext &context,const std::string &ext,const std::shared_ptr<ufile::IFile> &file);

		void RegisterFormatHandler(const std::string &ext,const std::function<std::shared_ptr<ITextureFormatHandler>()> &factory);
	private:
		ctpl::thread_pool m_pool;
		std::mutex m_queueMutex;
		std::priority_queue<TextureLoadJob,std::vector<TextureLoadJob>,CompareTextureLoadJob> m_jobs;

		std::mutex m_completeQueueMutex;
		std::queue<TextureLoadJob> m_completeQueue;
		std::atomic<bool> m_hasCompletedJobs = false;
		std::unordered_map<std::string,std::function<std::shared_ptr<ITextureFormatHandler>()>> m_formatHandlers;
	};
};

#endif
