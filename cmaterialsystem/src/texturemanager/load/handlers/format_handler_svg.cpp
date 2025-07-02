// SPDX-FileCopyrightText: (c) 2025 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#include "texturemanager/load/handlers/format_handler_svg.hpp"
#include <util_image.hpp>
#include <udm.hpp>
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
	uimg::SvgImageInfo svgInfo {};
	if(texInfo.textureData) {
		udm::LinkedPropertyWrapper prop {*texInfo.textureData};
		prop["width"] >> svgInfo.width;
		prop["height"] >> svgInfo.height;

		if(!(prop["styleSheet"] >> svgInfo.styleSheet)) {
			std::stringstream ss;
			auto styleSheet = prop["styleSheet"];
			for(auto &pair : styleSheet.ElIt()) {
				auto &className = pair.key;
				ss << className << "{";
				for(auto &kvPair : pair.property.ElIt()) {
					auto &key = kvPair.key;
					std::string value;
					if(!(kvPair.property >> value))
						continue;
					ss << key << ":" << value << ";";
				}
				ss << "}";
			}
			svgInfo.styleSheet = ss.str();
		}
	}
	auto imgBuf = uimg::load_svg(*m_file, svgInfo);
	if(!imgBuf)
		return false;
	m_imgBuf = imgBuf;
	texInfo.flags |= InputTextureInfo::Flags::SrgbBit;
	texInfo.width = imgBuf->GetWidth();
	texInfo.height = imgBuf->GetHeight();
	texInfo.format = prosper::util::get_vk_format(imgBuf->GetFormat());
	return true;
}
