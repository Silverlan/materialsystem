// SPDX-FileCopyrightText: © 2021 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#include "texturemanager/load/texture_loader.hpp"
#include "texturemanager/load/texture_format_handler.hpp"
#include "texturemanager/load/texture_processor.hpp"
#include <prosper_context.hpp>

void msys::setup_sampler_mipmap_mode(prosper::util::SamplerCreateInfo &createInfo, TextureMipmapMode mode)
{
	switch(mode) {
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

msys::TextureLoader::TextureLoader(util::IAssetManager &assetManager, prosper::IPrContext &context) : util::TAssetFormatLoader<TextureProcessor> {assetManager, "texture"}, m_context {context}
{
	auto samplerCreateInfo = prosper::util::SamplerCreateInfo {};
	setup_sampler_mipmap_mode(samplerCreateInfo, TextureMipmapMode::Load);
	m_textureSampler = context.CreateSampler(samplerCreateInfo);

	samplerCreateInfo = {};
	setup_sampler_mipmap_mode(samplerCreateInfo, TextureMipmapMode::Ignore);
	m_textureSamplerNoMipmap = context.CreateSampler(samplerCreateInfo);
}
