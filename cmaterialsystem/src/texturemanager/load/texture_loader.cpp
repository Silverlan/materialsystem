/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "texturemanager/load/texture_loader.hpp"
#include "texturemanager/load/texture_format_handler.hpp"
#include "texturemanager/load/texture_processor.hpp"
#pragma optimize("",off)
msys::TextureLoader::TextureLoader()
	: m_pool{5}
{
	throw std::runtime_error{"TODO: Turn this into a generic asset loader interface class, use it for different asset types (textures/audio/models/...)"};
}

msys::TextureLoader::~TextureLoader()
{}

void msys::TextureLoader::RegisterFormatHandler(const std::string &ext,const std::function<std::shared_ptr<ITextureFormatHandler>()> &factory)
{
	m_formatHandlers[ext] = factory;
}

bool msys::TextureLoader::AddJob(
	prosper::IPrContext &context,const std::string &identifier,const std::string &ext,const std::shared_ptr<ufile::IFile> &file,TextureLoadJobPriority priority
)
{
	auto it = m_formatHandlers.find(ext);
	if(it == m_formatHandlers.end())
		return false;
	auto handler = it->second();
	if(!handler)
		return false;
	auto itJob = m_texIdToJobId.find(identifier);
	if(itJob != m_texIdToJobId.end() && itJob->second.priority >= priority)
		return true; // Already queued up with the same (or higher) priority, no point in adding it again
	handler->SetFile(file);
	
	auto jobId = m_nextJobId++;
	TextureLoadJob job {};
	job.handler = handler;
	job.priority = priority;
	job.jobId = jobId;
	job.textureIdentifier = identifier;
	job.queueStartTime = std::chrono::high_resolution_clock::now();
	m_texIdToJobId[identifier] = {jobId,priority};

	m_queueMutex.lock();
		m_jobs.emplace(std::move(job));
	m_queueMutex.unlock();

	m_pool.push([this,&context](int id) {
		m_queueMutex.lock();
			auto job = std::move(const_cast<TextureLoadJob&>(m_jobs.top()));
			m_jobs.pop();
		m_queueMutex.unlock();
		
		job.taskStartTime = std::chrono::high_resolution_clock::now();
		if(!job.handler->LoadData())
			job.state = TextureLoadJob::State::Failed;
		else
		{
			job.processor = std::make_unique<TextureProcessor>(*job.handler);
			if(m_allowMultiThreadedGpuResourceAllocation == true && !job.processor->PrepareImage(context))
				job.state = TextureLoadJob::State::Failed;
			else
				job.state = TextureLoadJob::State::Succeeded;
		}
		job.completionTime = std::chrono::high_resolution_clock::now();

		m_completeQueueMutex.lock();
			m_completeQueue.push(job);
			m_hasCompletedJobs = true;
		m_completeQueueMutex.unlock();
	});
	return true;
}

void msys::TextureLoader::Poll(
	prosper::IPrContext &context,const std::function<void(const TextureLoadJob&)> &onComplete,
	const std::function<void(const TextureLoadJob&)> &onFailed
)
{
	if(!m_hasCompletedJobs)
		return;
	m_completeQueueMutex.lock();
		m_hasCompletedJobs = false;
		auto completeQueue = std::move(m_completeQueue);
		m_completeQueue = {};
	m_completeQueueMutex.unlock();

	while(!completeQueue.empty())
	{
		auto &job = completeQueue.front();
		job.completionTime = std::chrono::high_resolution_clock::now();
		auto success = false;
		if(job.state == TextureLoadJob::State::Succeeded)
		{
			assert(job.processor != nullptr);
			auto it = m_texIdToJobId.find(job.textureIdentifier);
			// If a texture is queued up multiple times, we only care about the latest job
			// and disregard previous ones.
			auto valid = (it != m_texIdToJobId.end() && it->second.jobId == job.jobId);
			if(valid)
			{
				m_texIdToJobId.erase(it);
				if((m_allowMultiThreadedGpuResourceAllocation == true || job.processor->PrepareImage(context)) && job.processor->FinalizeImage(context))
				{
					success = true;
					onComplete(job);
				}
			}
		}
		if(!success)
			onFailed(job);
		completeQueue.pop();
	}
}
#pragma optimize("",on)
