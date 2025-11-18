// SPDX-FileCopyrightText: (c) 2021 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.cmaterialsystem;

import :texture_manager.format_handlers.gli;

bool msys::TextureFormatHandlerGli::GetDataPtr(uint32_t layer, uint32_t mipmapIdx, void **outPtr, size_t &outSize)
{
	auto cubemap = umath::is_flag_set(m_inputTextureInfo.flags, InputTextureInfo::Flags::CubemapBit);
	auto gliLayer = cubemap ? 0 : layer;
	auto gliFace = cubemap ? layer : 0;
	outSize = m_texture.size(mipmapIdx);
	*outPtr = m_texture.data(gliLayer, gliFace, mipmapIdx);
	return *outPtr != nullptr;
}

void msys::TextureFormatHandlerGli::SetTexture(gli::texture &&tex) { m_texture = std::move(tex); }

bool msys::TextureFormatHandlerGli::LoadData(InputTextureInfo &texInfo)
{
	auto sz = m_file->GetSize();
	if(sz == 0)
		return false;
	std::vector<uint8_t> data(sz);
	m_file->Read(data.data(), sz);
	m_texture = gli::load(static_cast<char *>(static_cast<void *>(data.data())), data.size());
	if(m_texture.empty())
		return false;
	if(ShouldFlipTextureVertically()) {
		m_texture = gli::flip(m_texture);
		if(m_texture.empty())
			return false;
	}
	texInfo.flags |= InputTextureInfo::Flags::SrgbBit;
	auto isCubemap = m_texture.faces() == 6;
	umath::set_flag(texInfo.flags, InputTextureInfo::Flags::CubemapBit, isCubemap);
	auto extents = m_texture.extent();
	texInfo.width = extents.x;
	texInfo.height = extents.y;
	texInfo.layerCount = isCubemap ? 6 : 1;
	texInfo.mipmapCount = m_texture.levels();
	texInfo.format = static_cast<prosper::Format>(m_texture.format());
	return true;
}
