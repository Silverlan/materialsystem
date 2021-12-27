/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef DISABLE_VTEX_SUPPORT
#include "texturemanager/load/handlers/format_handler_vtex.hpp"
#include "texturemanager/load/handlers/format_handler_vtf.hpp"
#include <util_source2.hpp>
#include <source2/resource.hpp>
#include <source2/resource_data.hpp>

static msys::detail::VulkanImageData vtex_format_to_vulkan_format(source2::VTexFormat format)
{
	msys::detail::VulkanImageData vkImgData {};
	switch(format)
	{
	case source2::VTexFormat::DXT1:
		vkImgData.format = prosper::Format::BC1_RGBA_UNorm_Block;
		break;
	case source2::VTexFormat::DXT5:
		vkImgData.format = prosper::Format::BC3_UNorm_Block;
		break;
	case source2::VTexFormat::RGBA8888:
		vkImgData.format = prosper::Format::R8G8B8A8_UNorm;
		break;
	case source2::VTexFormat::RGBA16161616:
		vkImgData.format = prosper::Format::R16G16B16A16_SNorm;
		break;
	case source2::VTexFormat::RGBA16161616F:
		vkImgData.format = prosper::Format::R16G16B16A16_SFloat;
		break;
	case source2::VTexFormat::RGB323232F: // Needs to be converted
		vkImgData.conversionFormat = prosper::Format::R32G32B32A32_SFloat;
		vkImgData.format = prosper::Format::R32G32B32_SFloat;
		break;
	case source2::VTexFormat::RGBA32323232F:
		vkImgData.format = prosper::Format::R32G32B32A32_SFloat;
		break;
	case source2::VTexFormat::BC6H:
		vkImgData.format = prosper::Format::BC6H_SFloat_Block;
		break;
	case source2::VTexFormat::BC7:
		vkImgData.format = prosper::Format::BC7_UNorm_Block;
		break;
	case source2::VTexFormat::BGRA8888:
		vkImgData.swizzle = {prosper::ComponentSwizzle::B,prosper::ComponentSwizzle::G,prosper::ComponentSwizzle::R,prosper::ComponentSwizzle::A};
		vkImgData.format = prosper::Format::B8G8R8A8_UNorm;
		break;
	case source2::VTexFormat::ATI1N:
		vkImgData.format = prosper::Format::BC4_UNorm_Block;
		break;
	case source2::VTexFormat::ATI2N:
		vkImgData.format = prosper::Format::BC5_UNorm_Block;
		break;
	default:
		vkImgData.format = prosper::Format::BC1_RGBA_UNorm_Block;
		break;
	}
	// Note: When adding a new format, make sure to also add it to TextureManager::InitializeTextureData
	return vkImgData;
}

bool msys::TextureFormatHandlerVtex::GetDataPtr(uint32_t layer,uint32_t mipmapIdx,void **outPtr,size_t &outSize)
{
	if(layer > 0)
		return false;
	m_texture->ReadTextureData(mipmapIdx,m_mipmapData);
	outSize = m_mipmapData.size();
	*outPtr = m_mipmapData.data();
	return true;
}

bool msys::TextureFormatHandlerVtex::LoadData(InputTextureInfo &texInfo)
{
	auto resource = source2::load_resource(*m_file);
	auto *dataBlock = resource ? resource->FindBlock(source2::BlockType::DATA) : nullptr;
	if(!dataBlock)
		return false;
	auto *texBlock = dynamic_cast<source2::resource::Texture*>(dataBlock);
	if(!texBlock)
		return false;
	auto texture = std::static_pointer_cast<source2::resource::Texture>(texBlock->shared_from_this());

	auto format = texture->GetFormat();
	switch(format)
	{
	case source2::VTexFormat::DXT1:
	case source2::VTexFormat::DXT5:
	case source2::VTexFormat::RGBA8888:
	case source2::VTexFormat::RGBA16161616:
	case source2::VTexFormat::RGBA16161616F:
	case source2::VTexFormat::RGB323232F:
	case source2::VTexFormat::RGBA32323232F:
	case source2::VTexFormat::BC6H:
	case source2::VTexFormat::BC7:
	case source2::VTexFormat::BGRA8888:
	case source2::VTexFormat::ATI1N:
	case source2::VTexFormat::ATI2N:
		break; // Note: When adding new formats, make sure to also add them to texture_initialize.cpp:vtex_format_to_vulkan_format
	default:
		return false; // Unsupported format
	}

	texInfo.width = texture->GetWidth();
	texInfo.height = texture->GetHeight();
	texInfo.layerCount = 1;
	texInfo.mipmapCount = texture->GetMipMapCount();

	auto vkFormat = vtex_format_to_vulkan_format(texture->GetFormat());
	texInfo.format = vkFormat.format;
	texInfo.swizzle = vkFormat.swizzle;
	texInfo.conversionFormat = vkFormat.conversionFormat;
	m_texture = texture;
	return true;
}

#endif
