/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "texturemanager/load/handlers/format_handler_uimg.hpp"
#include <util_image.hpp>
#include <prosper_util_image_buffer.hpp>
#include <fsys/ifile.hpp>

bool msys::TextureFormatHandlerUimg::GetDataPtr(uint32_t layer,uint32_t mipmapIdx,void **outPtr,size_t &outSize)
{
	if(layer != 0 || mipmapIdx != 0)
		return false;
	*outPtr = m_imgBuf->GetData();
	outSize = m_imgBuf->GetSize();
	return true;
}

bool msys::TextureFormatHandlerUimg::LoadData(InputTextureInfo &texInfo)
{
	auto imgBuf = uimg::load_image(*m_file);
	if(!imgBuf)
		return false;
	m_imgBuf = imgBuf;
	texInfo.flags |= InputTextureInfo::Flags::SrgbBit;
	texInfo.width = imgBuf->GetWidth();
	texInfo.height = imgBuf->GetHeight();
	texInfo.format = prosper::util::get_vk_format(imgBuf->GetFormat());
	return true;
}
