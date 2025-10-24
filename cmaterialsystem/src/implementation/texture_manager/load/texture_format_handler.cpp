// SPDX-FileCopyrightText: (c) 2021 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.cmaterialsystem;

import :texture_manager.texture_format_handler;

static bool g_shouldFlipVertically = false;
void msys::ITextureFormatHandler::SetFlipTexturesVertically(bool flip) { g_shouldFlipVertically = flip; }
bool msys::ITextureFormatHandler::ShouldFlipTextureVertically() { return g_shouldFlipVertically; }

msys::ITextureFormatHandler::ITextureFormatHandler(util::IAssetManager &assetManager) : util::IAssetFormatHandler {assetManager} {}

bool msys::ITextureFormatHandler::LoadData() { return LoadData(m_inputTextureInfo); }

void msys::ITextureFormatHandler::SetTextureData(const std::shared_ptr<udm::Property> &textureData) { m_inputTextureInfo.textureData = textureData; }

void msys::ITextureFormatHandler::Flip(const InputTextureInfo &texInfo)
{
	auto targetType = gli::texture::target_type::TARGET_2D;
	auto formatType = static_cast<gli::texture::format_type>(texInfo.format);
	gli::texture::extent_type extent {texInfo.width, texInfo.height, 1};
	gli::texture::size_type layers = texInfo.layerCount;
	gli::texture::size_type faces = 1;
	gli::texture::size_type levels = texInfo.mipmapCount;
	gli::texture tex {targetType, formatType, extent, layers, faces, levels};
	uint32_t face = 0;
	for(uint32_t layer = 0u; layer < layers; ++layer) {
		for(uint32_t mipLevel = 0u; mipLevel < levels; ++mipLevel) {
			void *ptr;
			size_t size;
			if(!GetDataPtr(layer, mipLevel, &ptr, size))
				continue;
			gli::texture gliView {tex, targetType, formatType, layer, layer, face, face, mipLevel, mipLevel};
			auto *gliData = gliView.data(0, 0, 0);
			auto gliSize = gliView.size(0);
			if(!gliData || gliSize != size)
				continue;
			// Flip using gli, then copy the flipped data back.
			// TODO: Flipping could be done without a copy
			memcpy(gliData, ptr, size);
			gli::flip(gliView);
			memcpy(ptr, gliData, size);
		}
	}
}
