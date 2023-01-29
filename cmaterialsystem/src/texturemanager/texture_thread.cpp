/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "texturemanager/texturemanager.h"
#include "texturemanager/texturequeue.h"
#include "textureinfo.h"

bool TextureManager::CheckTextureThreadLocked()
{
	bool bContinue = false;
	m_activeMutex->lock();
	if(m_bThreadActive == false)
		bContinue = true;
	m_activeMutex->unlock();
	if(bContinue == false) {
		m_loadMutex->lock();
		bContinue = !m_loadQueue.empty();
		m_loadMutex->unlock();
	}
	return bContinue;
}

void TextureManager::Update()
{
	if(m_queueMutex == NULL)
		return;
	m_queueMutex->lock();
	if(!m_initQueue.empty()) {
		auto item = m_initQueue.front();
		m_initQueue.pop();
		if(!item->initialized)
			InitializeImage(*item);

		item->mipmapid = -1;
		/*if(item->mipmapid != -1)
		{
			if(item->mipmapid > 0)
				InitializeGLMipmap(item);
			if(item->mipmapid != -1)
				PushOnLoadQueue(item);
		}*/
		if(item->mipmapid == -1)
			FinalizeTexture(*item);
	}
	m_queueMutex->unlock();
}

void TextureManager::PushOnLoadQueue(std::unique_ptr<TextureQueueItem> item)
{
	m_loadMutex->lock();
	m_loadQueue.push(std::move(item));
	m_queueVar->notify_one();
	m_loadMutex->unlock();
}

std::shared_ptr<Texture> TextureManager::GetQueuedTexture(TextureQueueItem &item, bool bErase)
{
	std::shared_ptr<Texture> texture = nullptr;
	for(unsigned int i = 0; i < m_texturesTmp.size(); i++) {
		if(m_texturesTmp[i]->GetName() == item.cache) {
			texture = m_texturesTmp[i];
			if(bErase == true)
				m_texturesTmp.erase(m_texturesTmp.begin() + i);
			break;
		}
	}
	return texture;
}

void TextureManager::TextureThread()
{
	std::unique_lock<std::mutex> lock(*m_lockMutex);
	auto n = std::bind(&TextureManager::CheckTextureThreadLocked, this);
	for(;;) {
		m_queueVar->wait(lock, n);
		m_activeMutex->lock();
		if(m_bThreadActive == false) {
			m_activeMutex->unlock();
			break;
		}
		m_activeMutex->unlock();
		m_loadMutex->lock();
		auto item = std::move(m_loadQueue.front());
		m_loadQueue.pop();
		m_bBusy = true;
		m_loadMutex->unlock();
		if(item->mipmapid == -1) {
			InitializeTextureData(*item);
			//if(InitializeMipmaps(item) == true)
			item->mipmapid = 0;
		}
		else
			; //GenerateNextMipmap(item);
		m_queueMutex->lock();
		m_initQueue.push(item);
		m_bBusy = false;
		m_queueMutex->unlock();
	}
}
