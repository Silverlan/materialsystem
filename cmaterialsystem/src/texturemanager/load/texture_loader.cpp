/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "texturemanager/load/texture_loader.hpp"
#include "texturemanager/load/texture_format_handler.hpp"
#include "texturemanager/load/texture_processor.hpp"
#include <prosper_context.hpp>
#pragma optimize("",off)
static void setup_sampler_mipmap_mode(prosper::util::SamplerCreateInfo &createInfo,TextureMipmapMode mode)
{
	switch(mode)
	{
		case TextureMipmapMode::Ignore:
		{
			createInfo.minFilter = prosper::Filter::Nearest;
			createInfo.magFilter = prosper::Filter::Nearest;
			createInfo.mipmapMode = prosper::SamplerMipmapMode::Nearest;
			createInfo.minLod = 0.f;
			createInfo.maxLod = 0.f;
			break;
		}
		default:
		{
			createInfo.minFilter = prosper::Filter::Linear;
			createInfo.magFilter = prosper::Filter::Linear;
			createInfo.mipmapMode = prosper::SamplerMipmapMode::Linear;
			break;
		}
	}
}

msys::TextureLoader::TextureLoader(prosper::IPrContext &context)
	: m_context{context}
{
	auto samplerCreateInfo = prosper::util::SamplerCreateInfo {};
	setup_sampler_mipmap_mode(samplerCreateInfo,TextureMipmapMode::Load);
	m_textureSampler = context.CreateSampler(samplerCreateInfo);

	samplerCreateInfo = {};
	setup_sampler_mipmap_mode(samplerCreateInfo,TextureMipmapMode::Ignore);
	m_textureSamplerNoMipmap = context.CreateSampler(samplerCreateInfo);
}
void msys::TextureLoader::RegisterFormatHandler(const std::string &ext,const std::function<std::unique_ptr<ITextureFormatHandler>()> &factory)
{
	m_formatHandlers[ext] = factory;
}

std::optional<util::AssetLoadJobId> msys::TextureLoader::AddJob(
	const std::string &identifier,const std::string &ext,const std::shared_ptr<ufile::IFile> &file,util::AssetLoadJobPriority priority
)
{
	auto it = m_formatHandlers.find(ext);
	if(it == m_formatHandlers.end())
		return {};
	auto handler = it->second();
	if(!handler)
		return {};
	handler->SetFile(file);
	auto processor = std::make_unique<TextureProcessor>(*this,std::move(handler));
	return IAssetLoader::AddJob(identifier,std::move(processor),priority);
}
#pragma optimize("",on)
