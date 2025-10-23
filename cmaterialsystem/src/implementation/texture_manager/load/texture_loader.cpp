// SPDX-FileCopyrightText: (c) 2021 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <memory>
#include <string>

module pragma.cmaterialsystem;

import :texture_manager.texture_loader;

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
