/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "texturemanager/load/texture_loader.hpp"
#include "texturemanager/load/texture_format_handler.hpp"
#include "texturemanager/load/texture_processor.hpp"
#pragma optimize("",off)
msys::TextureLoader::TextureLoader()
	: m_pool{5}
{}

msys::TextureLoader::~TextureLoader()
{}

void msys::TextureLoader::RegisterFormatHandler(const std::string &ext,const std::function<std::shared_ptr<ITextureFormatHandler>()> &factory)
{
	m_formatHandlers[ext] = factory;
}

bool msys::TextureLoader::AddJob(prosper::IPrContext &context,const std::string &ext,const std::shared_ptr<ufile::IFile> &file)
{
	auto it = m_formatHandlers.find(ext);
	if(it == m_formatHandlers.end())
		return false;
	auto handler = it->second();
	if(!handler)
		return false;
	handler->SetFile(file);
	
	TextureLoadJob job {};
	job.handler = handler;
	m_queueMutex.lock();
		m_jobs.emplace(std::move(job));
	m_queueMutex.unlock();

	m_pool.push([this,&context](int id) {
		m_queueMutex.lock();
			auto job = std::move(const_cast<TextureLoadJob&>(m_jobs.top()));
			m_jobs.pop();
		m_queueMutex.unlock();
		
		if(!job.handler->LoadData())
			job.state = TextureLoadJob::State::Failed;
		else
		{
			job.processor = std::make_unique<TextureProcessor>(*job.handler);
			if(!job.processor->PrepareImage(context))
				job.state = TextureLoadJob::State::Failed;
			else
				job.state = TextureLoadJob::State::Succeeded;
		}

		m_completeQueueMutex.lock();
			m_completeQueue.push(job);
			m_hasCompletedJobs = true;
		m_completeQueueMutex.unlock();
	});
	return true;
}

void msys::TextureLoader::Poll(prosper::IPrContext &context)
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
		if(job.state == TextureLoadJob::State::Succeeded)
		{
			assert(job.processor != nullptr);
			if(job.processor->FinalizeImage(context))
			{
				
			}
			else
			{
			
			}
		}
		completeQueue.pop();
	}
}
#pragma optimize("",on)
