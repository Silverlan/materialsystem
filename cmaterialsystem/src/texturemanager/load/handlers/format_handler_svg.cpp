/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "texturemanager/load/handlers/format_handler_svg.hpp"
#include <lunasvg.h>
#include <util_image.hpp>
#include <prosper_util_image_buffer.hpp>
#include <fsys/ifile.hpp>

bool msys::TextureFormatHandlerSvg::GetDataPtr(uint32_t layer, uint32_t mipmapIdx, void **outPtr, size_t &outSize)
{
	if(layer != 0 || mipmapIdx != 0)
		return false;
	*outPtr = m_imgBuf->GetData();
	outSize = m_imgBuf->GetSize();
	return true;
}
bool msys::TextureFormatHandlerSvg::LoadData(InputTextureInfo &texInfo)
{
	std::vector<uint8_t> data;
	data.resize(m_file->GetSize());
	if(m_file->Read(data.data(), data.size()) != data.size())
		return false;
	auto document = lunasvg::Document::loadFromData(reinterpret_cast<char *>(data.data()), data.size());
	if(document == nullptr)
		return false;
	auto bitmap = document->renderToBitmap();
	if(bitmap.isNull())
		return false;
	bitmap.convertToRGBA();
	auto imgBuf = uimg::ImageBuffer::Create(bitmap.data(), bitmap.width(), bitmap.height(), uimg::Format::RGBA8, false);
	if(!imgBuf)
		return false;
	m_imgBuf = imgBuf;
	texInfo.flags |= InputTextureInfo::Flags::SrgbBit;
	texInfo.width = imgBuf->GetWidth();
	texInfo.height = imgBuf->GetHeight();
	texInfo.format = prosper::util::get_vk_format(imgBuf->GetFormat());
	return true;
}
