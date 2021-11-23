/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "texturemanager/load/texture_loader.hpp"
#include "texturemanager/load/texture_format_handler.hpp"
#include "texturemanager/load/texture_processor.hpp"
#pragma optimize("",off)
msys::TextureLoader::TextureLoader(prosper::IPrContext &context)
	: m_context{context}
{}
void msys::TextureLoader::RegisterFormatHandler(const std::string &ext,const std::function<std::unique_ptr<ITextureFormatHandler>()> &factory)
{
	m_formatHandlers[ext] = factory;
}

bool msys::TextureLoader::AddJob(
	prosper::IPrContext &context,const std::string &identifier,const std::string &ext,const std::shared_ptr<ufile::IFile> &file,util::AssetLoadJobPriority priority
)
{
	auto it = m_formatHandlers.find(ext);
	if(it == m_formatHandlers.end())
		return false;
	auto handler = it->second();
	if(!handler)
		return false;
	handler->SetFile(file);
	auto processor = std::make_unique<TextureProcessor>(*this,std::move(handler));
	return IAssetLoader::AddJob(identifier,std::move(processor),priority);
}
#pragma optimize("",on)
