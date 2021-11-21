/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DISABLE_VTF_SUPPORT
#include "texturemanager/load/handlers/format_handler_vtf.hpp"
#include <fsys/ifile.hpp>
#include <VTFFile.h>

static msys::detail::VulkanImageData vtf_format_to_vulkan_format(VTFImageFormat format)
{
	msys::detail::VulkanImageData vkImgData {};
	switch(format)
	{
		case VTFImageFormat::IMAGE_FORMAT_DXT1:
			vkImgData.format = prosper::Format::BC1_RGBA_UNorm_Block;
			break;
		case VTFImageFormat::IMAGE_FORMAT_DXT3:
			vkImgData.format = prosper::Format::BC2_UNorm_Block;
			break;
		case VTFImageFormat::IMAGE_FORMAT_DXT5:
			vkImgData.format = prosper::Format::BC3_UNorm_Block;
			break;
		case VTFImageFormat::IMAGE_FORMAT_RGBA8888:
			vkImgData.format = prosper::Format::R8G8B8A8_UNorm;
			break;
		case VTFImageFormat::IMAGE_FORMAT_RGB888: // Needs to be converted
			vkImgData.conversionFormat = prosper::Format::R8G8B8A8_UNorm;
			vkImgData.format = prosper::Format::R8G8B8_UNorm_PoorCoverage;
			break;
		case VTFImageFormat::IMAGE_FORMAT_BGRA8888:
			vkImgData.format = prosper::Format::B8G8R8A8_UNorm;
			break;
		case VTFImageFormat::IMAGE_FORMAT_BGR888: // Needs to be converted
			vkImgData.conversionFormat = prosper::Format::B8G8R8A8_UNorm;
			vkImgData.format = prosper::Format::B8G8R8_UNorm_PoorCoverage;
			break;
		case VTFImageFormat::IMAGE_FORMAT_UV88:
			vkImgData.format = prosper::Format::R8G8_UNorm;
			break;
		case VTFImageFormat::IMAGE_FORMAT_RGBA16161616F:
			vkImgData.format = prosper::Format::R16G16B16A16_SFloat;
			break;
		case VTFImageFormat::IMAGE_FORMAT_RGBA32323232F:
			vkImgData.format = prosper::Format::R32G32B32A32_SFloat;
			break;
		case VTFImageFormat::IMAGE_FORMAT_ABGR8888:
			vkImgData.swizzle = {prosper::ComponentSwizzle::A,prosper::ComponentSwizzle::B,prosper::ComponentSwizzle::G,prosper::ComponentSwizzle::R};
			vkImgData.format = prosper::Format::A8B8G8R8_UNorm_Pack32;
			break;
		case VTFImageFormat::IMAGE_FORMAT_BGRX8888:
			vkImgData.format = prosper::Format::B8G8R8A8_UNorm;
			break;
		default:
			vkImgData.format = prosper::Format::BC1_RGBA_UNorm_Block;
			break;
	}
	// Note: When adding a new format, make sure to also add it to TextureManager::InitializeTextureData
	return vkImgData;
}

bool msys::TextureFormatHandlerVtf::GetDataPtr(uint32_t layer,uint32_t mipmapIdx,void **outPtr,size_t &outSize)
{
	outSize = VTFLib::CVTFFile::ComputeMipmapSize(
		m_texture->GetWidth(),m_texture->GetHeight(),
		m_texture->GetDepth(),mipmapIdx,m_texture->GetFormat()
	);
	*outPtr = m_texture->GetData(0,layer,0,mipmapIdx);
	return *outPtr != nullptr;
}

bool msys::TextureFormatHandlerVtf::LoadData(InputTextureInfo &texInfo)
{
	auto texture = std::make_unique<VTFLib::CVTFFile>();
	auto valid = texture->Load(m_file.get(),false);
	if(valid == false)
		return false;
	auto format = texture->GetFormat();
	switch(format)
	{
		case VTFImageFormat::IMAGE_FORMAT_DXT1:
		case VTFImageFormat::IMAGE_FORMAT_DXT3:
		case VTFImageFormat::IMAGE_FORMAT_DXT5:
		case VTFImageFormat::IMAGE_FORMAT_RGB888:
		case VTFImageFormat::IMAGE_FORMAT_RGBA8888:
		case VTFImageFormat::IMAGE_FORMAT_BGR888:
		case VTFImageFormat::IMAGE_FORMAT_BGRA8888:
		case VTFImageFormat::IMAGE_FORMAT_UV88:
		case VTFImageFormat::IMAGE_FORMAT_RGBA16161616F:
		case VTFImageFormat::IMAGE_FORMAT_RGBA32323232F:
		case VTFImageFormat::IMAGE_FORMAT_ABGR8888:
		case VTFImageFormat::IMAGE_FORMAT_BGRX8888:
			break; // Note: When adding new formats, make sure to also add them to texture_initialize.cpp:vtf_format_to_vulkan_format
		default:
			return false; // Unsupported format
	}
	auto cubemap = texture->GetFaceCount() == 6;
	texInfo.flags |= InputTextureInfo::Flags::SrgbBit;
	umath::set_flag(texInfo.flags,InputTextureInfo::Flags::CubemapBit,cubemap);
	texInfo.width = texture->GetWidth();
	texInfo.height = texture->GetHeight();
	texInfo.layerCount = cubemap ? 6 : 1;
	texInfo.mipmapCount = texture->GetMipmapCount();

	auto vkFormat = vtf_format_to_vulkan_format(texture->GetFormat());
	texInfo.format = vkFormat.format;
	texInfo.swizzle = vkFormat.swizzle;
	texInfo.conversionFormat = vkFormat.conversionFormat;

	if(texture->GetFlag(VTFImageFlag::TEXTUREFLAGS_NOMIP))
		texInfo.mipmapCount = 1u;
	m_texture = std::move(texture);
	return true;
}
#endif
