/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "texturemanager/load/handlers/format_handler_gli.hpp"
#include <sharedutils/util_ifile.hpp>

bool msys::TextureFormatHandlerGli::GetDataPtr(uint32_t layer, uint32_t mipmapIdx, void **outPtr, size_t &outSize)
{
	auto cubemap = umath::is_flag_set(m_inputTextureInfo.flags, InputTextureInfo::Flags::CubemapBit);
	auto gliLayer = cubemap ? 0 : layer;
	auto gliFace = cubemap ? layer : 0;
	outSize = m_texture.size(mipmapIdx);
	*outPtr = m_texture.data(gliLayer, gliFace, mipmapIdx);
	return *outPtr != nullptr;
}

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
