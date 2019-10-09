/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "texturemanager/texturemanager.h"
#include "texturemanager/texturequeue.h"
#include "textureinfo.h"
#include "texturemanager/texture.h"
#include <prosper_context.hpp>
#include <image/prosper_texture.hpp>
#include <prosper_util.hpp>
#include <prosper_command_buffer.hpp>
#include <config.h>
#include <wrappers/image.h>
#include <wrappers/memory_block.h>
#include <mathutil/glmutil.h>
#include <gli/gli.hpp>
#include <util_image.h>

#define FLUSH_INIT_CMD 1

#pragma optimize("",off)
static void change_image_transfer_dst_layout_to_shader_read(Anvil::CommandBufferBase &cmdBuffer,Anvil::Image &img)
{
	prosper::util::record_image_barrier(cmdBuffer,img,Anvil::ImageLayout::TRANSFER_DST_OPTIMAL,Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL);
}

struct ImageFormatLoader
{
	void *userData = nullptr;
	void(*get_image_info)(void *userData,const TextureQueueItem &item,uint32_t &outWidth,uint32_t &outHeight,Anvil::Format &outFormat,bool &outCubemap,uint32_t &outLayerCount,uint32_t &outMipmapCount,std::optional<Anvil::Format> &outConversionFormat) = nullptr;
	const void*(*get_image_data)(void *userData,const TextureQueueItem &item,uint32_t layer,uint32_t mipmapIdx,uint32_t &outDataSize) = nullptr;
};

static void initialize_image(TextureQueueItem &item,const Texture &texture,const ImageFormatLoader &imgLoader,std::shared_ptr<prosper::Image> &outImage)
{
	auto &context = *item.context.lock();
	auto &dev = context.GetDevice();

	uint32_t width = 0;
	uint32_t height = 0;
	Anvil::Format format = Anvil::Format::R8G8B8A8_UNORM;
	std::optional<Anvil::Format> conversionFormat = {};
	uint32_t numLayers = 1u;
	uint32_t numMipMaps = 1u;
	imgLoader.get_image_info(imgLoader.userData,item,width,height,format,item.cubemap,numLayers,numMipMaps,conversionFormat);

	const auto usage = Anvil::ImageUsageFlagBits::TRANSFER_SRC_BIT | Anvil::ImageUsageFlagBits::TRANSFER_DST_BIT | Anvil::ImageUsageFlagBits::SAMPLED_BIT;
	if(conversionFormat.has_value())
	{
		auto isFormatManuallySupported = (format == Anvil::Format::B8G8R8_UNORM);
		if(
			(isFormatManuallySupported == false && context.IsImageFormatSupported(format,usage,Anvil::ImageType::_2D,Anvil::ImageTiling::LINEAR) == false) ||
			context.IsImageFormatSupported(*conversionFormat,usage) == false
		)
			return;
	}
	else if(context.IsImageFormatSupported(format,usage) == false)
		return;

	auto numMipMapsLoad = numMipMaps;
	auto bGenerateMipmaps = (item.mipmapMode == TextureMipmapMode::Generate || (item.mipmapMode == TextureMipmapMode::LoadOrGenerate && numMipMaps <= 1)) ? true : false;
	if(bGenerateMipmaps == true)
		numMipMaps = prosper::util::calculate_mipmap_count(width,height);

	auto bCubemap = item.cubemap;

	// Initialize output image
	prosper::util::ImageCreateInfo createInfo {};
	createInfo.width = width;
	createInfo.height = height;
	createInfo.format = texture.internalFormat;
	if(numMipMaps > 1u)
		createInfo.flags |= prosper::util::ImageCreateInfo::Flags::FullMipmapChain;
	createInfo.memoryFeatures = prosper::util::MemoryFeatureFlags::DeviceLocal;
	createInfo.tiling = conversionFormat.has_value() ? Anvil::ImageTiling::LINEAR : Anvil::ImageTiling::OPTIMAL;
	createInfo.usage = usage;
	createInfo.layers = (bCubemap == true) ? 6 : 1;
	createInfo.postCreateLayout = Anvil::ImageLayout::TRANSFER_DST_OPTIMAL;
	if(bCubemap == true)
		createInfo.flags |= prosper::util::ImageCreateInfo::Flags::Cubemap;
	outImage = prosper::util::create_image(dev,createInfo);
	outImage->SetDebugName("texture_asset_img");

	// Note: We need to use staging buffers instead of images, because compressed image data does not work well with
	// the staging image method.

	std::vector<std::shared_ptr<prosper::Buffer>> buffers {};
	buffers.reserve(numLayers *numMipMapsLoad);
	auto &setupCmd = context.GetSetupCommandBuffer();
	for(auto iLayer=decltype(numLayers){0u};iLayer<numLayers;++iLayer)
	{
		for(auto iMipmap=decltype(numMipMapsLoad){0u};iMipmap<numMipMapsLoad;++iMipmap)
		{
			uint32_t dataSize;
			auto *data = imgLoader.get_image_data(imgLoader.userData,item,iLayer,iMipmap,dataSize);

			// Note: Some formats are very uncommonly supported, even in linear format.
			// These should generally be avoided, but some exceptions are handled here by
			// being converted manually.
			std::vector<uint8_t> tmpData {};
			if(format == Anvil::Format::B8G8R8_UNORM)
			{
				using PixelData = std::array<uint8_t,4>;
				tmpData.resize(width *height *sizeof(PixelData));
				for(uint32_t px=0u;px<(width *height);++px)
				{
					auto &srcColor = static_cast<const std::array<uint8_t,3>*>(data)[px];
					auto &dstColor = reinterpret_cast<PixelData*>(tmpData.data())[px];
					dstColor = {srcColor.at(0),srcColor.at(1),srcColor.at(2),std::numeric_limits<uint8_t>::max()};
				}
				data = tmpData.data();
				dataSize = tmpData.size() *sizeof(tmpData.front());
			}
			if(data == nullptr)
				continue;

			// Initialize buffer with source image data
			prosper::util::BufferCreateInfo createInfo {};
			createInfo.usageFlags = Anvil::BufferUsageFlagBits::TRANSFER_SRC_BIT;
			createInfo.size = dataSize;
			createInfo.memoryFeatures = prosper::util::MemoryFeatureFlags::CPUToGPU;
			auto buf = prosper::util::create_buffer(dev,createInfo,data);
			buffers.push_back(buf); // We need to keep the buffers alive until the copy has completed
			
			// Copy buffer contents to output image
			auto extent = (*outImage)->get_image_extent_2D(iMipmap);
			prosper::util::BufferImageCopyInfo copyInfo {};
			copyInfo.mipLevel = iMipmap;
			copyInfo.baseArrayLayer = iLayer;
			copyInfo.width = extent.width;
			copyInfo.height = extent.height;
			prosper::util::record_copy_buffer_to_image(setupCmd->GetAnvilCommandBuffer(),copyInfo,*buf,outImage->GetAnvilImage());
		}
	}

	if(bGenerateMipmaps == false)
		change_image_transfer_dst_layout_to_shader_read(setupCmd->GetAnvilCommandBuffer(),outImage->GetAnvilImage());
	context.FlushSetupCommandBuffer(); // Make sure image copy is complete before generating mipmaps

	if(conversionFormat.has_value())
	{
		auto &setupCmd = context.GetSetupCommandBuffer();

		// Image needs to be converted
		prosper::util::ImageCreateInfo createInfo;
		outImage->GetCreateInfo(createInfo);
		createInfo.format = *conversionFormat;
		createInfo.tiling = Anvil::ImageTiling::OPTIMAL;
		createInfo.postCreateLayout = Anvil::ImageLayout::TRANSFER_DST_OPTIMAL;

		auto convertedImage = outImage->Copy(*setupCmd,createInfo);
		prosper::util::record_image_barrier(**setupCmd,**outImage,Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL,Anvil::ImageLayout::TRANSFER_SRC_OPTIMAL);
		prosper::util::record_blit_image(**setupCmd,{},**outImage,**convertedImage);
		prosper::util::record_image_barrier(**setupCmd,**convertedImage,Anvil::ImageLayout::TRANSFER_DST_OPTIMAL,Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL);

		context.FlushSetupCommandBuffer();
		outImage = convertedImage;
	}

	if(bGenerateMipmaps == true)
	{
		auto &setupCmd = context.GetSetupCommandBuffer();
		prosper::util::record_generate_mipmaps(setupCmd->GetAnvilCommandBuffer(),outImage->GetAnvilImage(),Anvil::ImageLayout::TRANSFER_DST_OPTIMAL,Anvil::AccessFlagBits::TRANSFER_WRITE_BIT,Anvil::PipelineStageFlagBits::TRANSFER_BIT);
		context.FlushSetupCommandBuffer();
	}
}

#ifdef ENABLE_VTF_SUPPORT

struct VulkanImageData
{
	// Original image format
	Anvil::Format format = Anvil::Format::R8G8B8A8_UNORM;

	// Some formats may not be supported in optimal layout by common GPUs, in which case we need to convert it
	std::optional<Anvil::Format> conversionFormat = {};

	std::array<Anvil::ComponentSwizzle,4> swizzle = {Anvil::ComponentSwizzle::R,Anvil::ComponentSwizzle::G,Anvil::ComponentSwizzle::B,Anvil::ComponentSwizzle::A};
};
static VulkanImageData vtf_format_to_vulkan_format(VTFImageFormat format)
{
	VulkanImageData vkImgData {};
	switch(format)
	{
		case VTFImageFormat::IMAGE_FORMAT_DXT1:
			vkImgData.format = Anvil::Format::BC1_RGBA_UNORM_BLOCK;
			break;
		case VTFImageFormat::IMAGE_FORMAT_DXT3:
			vkImgData.format = Anvil::Format::BC2_UNORM_BLOCK;
			break;
		case VTFImageFormat::IMAGE_FORMAT_DXT5:
			vkImgData.format = Anvil::Format::BC3_UNORM_BLOCK;
			break;
		case VTFImageFormat::IMAGE_FORMAT_RGBA8888:
			vkImgData.format = Anvil::Format::R8G8B8A8_UNORM;
			break;
		case VTFImageFormat::IMAGE_FORMAT_RGB888: // Needs to be converted
			vkImgData.conversionFormat = Anvil::Format::R8G8B8A8_UNORM;
			vkImgData.format = Anvil::Format::R8G8B8_UNORM;
			break;
		case VTFImageFormat::IMAGE_FORMAT_BGRA8888:
			vkImgData.swizzle = {Anvil::ComponentSwizzle::B,Anvil::ComponentSwizzle::G,Anvil::ComponentSwizzle::R,Anvil::ComponentSwizzle::A};
			vkImgData.format = Anvil::Format::B8G8R8A8_UNORM;
			break;
		case VTFImageFormat::IMAGE_FORMAT_BGR888: // Needs to be converted
			vkImgData.swizzle = {Anvil::ComponentSwizzle::B,Anvil::ComponentSwizzle::G,Anvil::ComponentSwizzle::R,Anvil::ComponentSwizzle::A};
			vkImgData.conversionFormat = Anvil::Format::B8G8R8A8_UNORM;
			vkImgData.format = Anvil::Format::B8G8R8_UNORM;
			break;
		case VTFImageFormat::IMAGE_FORMAT_UV88:
			vkImgData.format = Anvil::Format::R8G8_UNORM;
			break;
		case VTFImageFormat::IMAGE_FORMAT_RGBA16161616F:
			vkImgData.format = Anvil::Format::R16G16B16A16_SFLOAT;
			break;
		case VTFImageFormat::IMAGE_FORMAT_RGBA32323232F:
			vkImgData.format = Anvil::Format::R32G32B32A32_SFLOAT;
			break;
		case VTFImageFormat::IMAGE_FORMAT_ABGR8888:
			vkImgData.swizzle = {Anvil::ComponentSwizzle::A,Anvil::ComponentSwizzle::B,Anvil::ComponentSwizzle::G,Anvil::ComponentSwizzle::R};
			vkImgData.format = Anvil::Format::A8B8G8R8_UNORM_PACK32;
			break;
		case VTFImageFormat::IMAGE_FORMAT_BGRX8888:
			vkImgData.swizzle = {Anvil::ComponentSwizzle::B,Anvil::ComponentSwizzle::G,Anvil::ComponentSwizzle::R,Anvil::ComponentSwizzle::ONE};
			vkImgData.format = Anvil::Format::B8G8R8A8_UNORM;
			break;
		default:
			vkImgData.format = Anvil::Format::BC1_RGBA_UNORM_BLOCK;
			break;
	}
	// Note: When adding a new format, make sure to also add it to TextureManager::InitializeTextureData
	return vkImgData;
}

#endif

#include <iostream>
void TextureManager::InitializeImage(TextureQueueItem &item)
{
	item.initialized = true;
	auto texture = GetQueuedTexture(item);
	texture->SetFlags(texture->GetFlags() | Texture::Flags::Error);
	if(item.valid == false)
	{
		auto texError = GetErrorTexture();
		if(texError != nullptr)
		{
			texture->texture = texError->texture;
			texture->width = texError->width;
			texture->height = texError->height;
			if(texError->texture != nullptr)
				texture->internalFormat = texError->texture->GetImage()->GetFormat();
		}
		else
			texture->texture = nullptr;
	}
	else
	{
		std::shared_ptr<prosper::Image> image = nullptr;
		std::array<Anvil::ComponentSwizzle,4> swizzle = {Anvil::ComponentSwizzle::R,Anvil::ComponentSwizzle::G,Anvil::ComponentSwizzle::B,Anvil::ComponentSwizzle::A};
		auto *surface = dynamic_cast<TextureQueueItemSurface*>(&item);
		if(surface != nullptr)
		{
			auto &img = surface->texture;
			auto extent = img->extent();
			texture->width = extent.x;
			texture->height = extent.y;
			texture->internalFormat = static_cast<Anvil::Format>(img->format());
			// In theory the input image should have a srgb flag if it's srgb, but in practice that's almost never the case,
			// so we just assume the image is srgb by default.
			// if(gli::is_srgb(img->format()))
			texture->SetFlags(Texture::Flags::SRGB);
			ImageFormatLoader gliLoader {};
			gliLoader.userData = static_cast<gli::texture2d*>(img.get());
			gliLoader.get_image_info = [](
				void *userData,const TextureQueueItem &item,uint32_t &outWidth,uint32_t &outHeight,Anvil::Format &outFormat,bool &outCubemap,uint32_t &outLayerCount,uint32_t &outMipmapCount,
				std::optional<Anvil::Format> &outConversionFormat
			) -> void {
				auto &texture = *static_cast<gli::texture2d*>(userData);
				auto img = texture[0];
				auto extents = img.extent();
				outWidth = extents.x;
				outHeight = extents.y;
				outFormat = static_cast<Anvil::Format>(texture.format());
				outCubemap = (texture.faces() == 6);
				outLayerCount = item.cubemap ? texture.faces() : texture.layers();
				outMipmapCount = texture.levels();
			};
			gliLoader.get_image_data = [](void *userData,const TextureQueueItem &item,uint32_t layer,uint32_t mipmapIdx,uint32_t &outDataSize) -> const void* {
				auto &texture = *static_cast<gli::texture2d*>(userData);
				auto gliLayer = item.cubemap ? 0 : layer;
				auto gliFace = item.cubemap ? layer : 0;
				outDataSize = texture.size(mipmapIdx);
				return texture.data(gliLayer,gliFace,mipmapIdx);
			};
			initialize_image(item,*texture,gliLoader,image);
		}
		else
		{
			auto *png = dynamic_cast<TextureQueueItemPNG*>(&item);
			if(png != nullptr && png->pnginfo->GetChannelCount() == 4)
			{
				auto &pngInfo = *png->pnginfo;
				texture->width = pngInfo.GetWidth();
				texture->height = pngInfo.GetHeight();
				texture->internalFormat = prosper::util::get_vk_format(pngInfo.GetType());
				texture->SetFlags(Texture::Flags::SRGB);

				ImageFormatLoader pngLoader {};
				pngLoader.userData = static_cast<uimg::Image*>(png->pnginfo.get());
				pngLoader.get_image_info = [](
					void *userData,const TextureQueueItem &item,uint32_t &outWidth,uint32_t &outHeight,Anvil::Format &outFormat,bool &outCubemap,uint32_t &outLayerCount,uint32_t &outMipmapCount,
					std::optional<Anvil::Format> &outConversionFormat
				) -> void {
					auto &png = *static_cast<uimg::Image*>(userData);
					outWidth = png.GetWidth();
					outHeight = png.GetHeight();
					outFormat = prosper::util::get_vk_format(png.GetType());
					outCubemap = false;
					outLayerCount = 1;
					outMipmapCount = 1;
				};
				pngLoader.get_image_data = [](void *userData,const TextureQueueItem &item,uint32_t layer,uint32_t mipmapIdx,uint32_t &outDataSize) -> const void* {
					auto &png = *static_cast<uimg::Image*>(userData);
					auto &data = png.GetData();
					outDataSize = data.size() *sizeof(data.front());
					return data.data();
				};
				initialize_image(item,*texture,pngLoader,image);
			}
			else
			{
				auto *tga = dynamic_cast<TextureQueueItemTGA*>(&item);
				if(tga != nullptr && tga->tgainfo->GetChannelCount() == 4)
				{
					static constexpr auto TGA_VK_FORMAT = Anvil::Format::R8G8B8A8_UNORM;
					auto &tgaInfo = *tga->tgainfo;
					texture->width = tgaInfo.GetWidth();
					texture->height = tgaInfo.GetHeight();
					texture->internalFormat = TGA_VK_FORMAT;
					texture->SetFlags(Texture::Flags::SRGB);

					ImageFormatLoader tgaLoader {};
					tgaLoader.userData = static_cast<uimg::Image*>(tga->tgainfo.get());
					tgaLoader.get_image_info = [](
						void *userData,const TextureQueueItem &item,uint32_t &outWidth,uint32_t &outHeight,Anvil::Format &outFormat,bool &outCubemap,uint32_t &outLayerCount,uint32_t &outMipmapCount,
						std::optional<Anvil::Format> &outConversionFormat
					) -> void {
						auto &tga = *static_cast<uimg::Image*>(userData);
						outWidth = tga.GetWidth();
						outHeight = tga.GetHeight();
						outFormat = TGA_VK_FORMAT;
						outCubemap = false;
						outLayerCount = 1;
						outMipmapCount = 1;
					};
					tgaLoader.get_image_data = [](void *userData,const TextureQueueItem &item,uint32_t layer,uint32_t mipmapIdx,uint32_t &outDataSize) -> const void* {
						auto &tga = *static_cast<uimg::Image*>(userData);
						auto &data = tga.GetData();
						outDataSize = data.size() *sizeof(data.front());
						return data.data();
					};
					initialize_image(item,*texture,tgaLoader,image);
				}
#ifdef ENABLE_VTF_SUPPORT
				else
				{
					auto *vtf = dynamic_cast<TextureQueueItemVTF*>(&item);
					if(vtf != nullptr)
					{
						auto &vtfFile = vtf->texture;
						texture->width = vtfFile->GetWidth();
						texture->height = vtfFile->GetHeight();
						auto vkImgData = vtf_format_to_vulkan_format(vtfFile->GetFormat());
						texture->internalFormat = vkImgData.conversionFormat.has_value() ? *vkImgData.conversionFormat : vkImgData.format;
						
						//if(vtfFile->GetFlag(VTFImageFlag::TEXTUREFLAGS_SRGB))
						// In theory the input image should have a srgb flag if it's srgb, but in practice that's almost never the case,
						// so we just assume the image is srgb by default.
						texture->SetFlags(Texture::Flags::SRGB);
						// swizzle = vkImgData.swizzle;

						ImageFormatLoader vtfLoader {};
						vtfLoader.userData = static_cast<VTFLib::CVTFFile*>(vtfFile.get());
						vtfLoader.get_image_info = [](
							void *userData,const TextureQueueItem &item,uint32_t &outWidth,uint32_t &outHeight,Anvil::Format &outFormat,bool &outCubemap,uint32_t &outLayerCount,uint32_t &outMipmapCount,
							std::optional<Anvil::Format> &outConversionFormat
						) -> void {
							auto &vtfFile = *static_cast<VTFLib::CVTFFile*>(userData);
							auto vkFormat = vtf_format_to_vulkan_format(vtfFile.GetFormat());
							outWidth = vtfFile.GetWidth();
							outHeight = vtfFile.GetHeight();
							outFormat = vkFormat.format;
							outConversionFormat = vkFormat.conversionFormat;
							outCubemap = vtfFile.GetFaceCount() == 6;
							outLayerCount = outCubemap ? 6 : 1;
							outMipmapCount = vtfFile.GetMipmapCount() +1u; // The VTF library does not count the main image as a mipmap, but we do!
							if(vtfFile.GetFlag(VTFImageFlag::TEXTUREFLAGS_NOMIP))
								outMipmapCount = 1u;
						};
						vtfLoader.get_image_data = [](void *userData,const TextureQueueItem &item,uint32_t layer,uint32_t mipmapIdx,uint32_t &outDataSize) -> const void* {
							auto &vtfFile = *static_cast<VTFLib::CVTFFile*>(userData);
							outDataSize = VTFLib::CVTFFile::ComputeMipmapSize(vtfFile.GetWidth(),vtfFile.GetHeight(),vtfFile.GetDepth(),mipmapIdx,vtfFile.GetFormat());
							return vtfFile.GetData(0,layer,0,mipmapIdx);
						};
						initialize_image(item,*texture,vtfLoader,image);
					}
				}
#endif
			}
		}
		if(image != nullptr)
		{
			auto &context = *item.context.lock();
			auto &dev = context.GetDevice();
			prosper::util::TextureCreateInfo createInfo {};
			createInfo.sampler = (item.sampler != nullptr) ? item.sampler : ((image->GetMipmapCount() > 1) ? m_textureSampler : m_textureSamplerNoMipmap);
			prosper::util::ImageViewCreateInfo imgViewCreateInfo {};
			imgViewCreateInfo.swizzleRed = swizzle.at(0);
			imgViewCreateInfo.swizzleGreen = swizzle.at(1);
			imgViewCreateInfo.swizzleBlue = swizzle.at(2);
			imgViewCreateInfo.swizzleAlpha = swizzle.at(3);
			createInfo.flags |= prosper::util::TextureCreateInfo::Flags::CreateImageViewForEachLayer;
			texture->texture = prosper::util::create_texture(dev,createInfo,std::move(image),&imgViewCreateInfo);

			texture->SetFlags(texture->GetFlags() & ~Texture::Flags::Error);
		}
	}
}

void TextureManager::FinalizeTexture(TextureQueueItem &item)
{
	auto texture = GetQueuedTexture(item,true);
	if(texture->IsIndexed() == false)
	{
		m_textures.push_back(texture);
		texture->SetFlags(texture->GetFlags() | Texture::Flags::Indexed);
	}
	texture->SetFlags(texture->GetFlags() | Texture::Flags::Loaded);
	texture->RunOnLoadedCallbacks();
}
#pragma optimize("",on)
