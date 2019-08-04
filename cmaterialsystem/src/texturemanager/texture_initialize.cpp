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

static void change_image_transfer_dst_layout_to_shader_read(Anvil::CommandBufferBase &cmdBuffer,Anvil::Image &img)
{
	prosper::util::record_image_barrier(cmdBuffer,img,Anvil::ImageLayout::TRANSFER_DST_OPTIMAL,Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL);
}

static void initialize_generic_image(
	const TextureQueueItem &item,uint32_t numTextures,std::shared_ptr<prosper::Image> &outImage,
	const std::function<bool(uint32_t,std::vector<std::shared_ptr<prosper::Image>>&,uint32_t&,uint32_t&,Anvil::Format&,uint32_t&)> initializeMipLevels,
	const std::function<void(uint32_t,std::vector<std::shared_ptr<prosper::Buffer>>&)> &initializeMipBuffers
)
{
	auto &context = *item.context.lock();
	auto &dev = context.GetDevice();
	struct SrcImage
	{
		std::vector<std::shared_ptr<prosper::Image>> mipLevels;
	};
	struct SrcBuffer
	{
		std::vector<std::shared_ptr<prosper::Buffer>> mipLevels;
	};
	std::vector<SrcImage> srcImages(numTextures);
	std::shared_ptr<std::vector<SrcBuffer>> srcBuffers = nullptr; // Only needed in case of incompatible linear image formats
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t outMipmapLevels = 1;
	Anvil::Format format = Anvil::Format::R8G8B8A8_UNORM;
	auto bValidSrcImages = true;
	auto bUseSrcBuffers = false;
	for(auto layer=decltype(srcImages.size()){0};layer<srcImages.size();++layer)
	{
		auto &mipLevels = srcImages[layer].mipLevels;
		if(initializeMipLevels(layer,mipLevels,width,height,format,outMipmapLevels) == false)
		{
			bUseSrcBuffers = true;
			break;
		}
	}

	// Note: Originally we'd use staging images for loading the image mipmaps, which would then get copied into the final image.
	// This requires linear tiling, so for image formats that don't support linear tiling, we'd use staging buffers instead.
	// However, compressed image data does not work well with the staging-image method, so we just use staging buffers for all cases now.
	// TODO: All staging-image code can be removed once this has been tested thoroughly!
	bUseSrcBuffers = true;

	// Use buffers if linear images for this format aren't supported
	if(bUseSrcBuffers == true)
	{
		if(context.IsImageFormatSupported(format,Anvil::ImageUsageFlagBits::TRANSFER_DST_BIT | Anvil::ImageUsageFlagBits::SAMPLED_BIT) == false)
			bValidSrcImages = false;
		else
		{
			srcBuffers = std::make_shared<std::vector<SrcBuffer>>(numTextures);
			for(auto layer=decltype(srcImages.size()){0};layer<srcImages.size();++layer)
			{
				auto &mipLevels = (*srcBuffers)[layer].mipLevels;
				initializeMipBuffers(layer,mipLevels);
			}
		}
	}
	//
	auto bGenerateMipmaps = (item.mipmapMode == TextureMipmapMode::Generate || (item.mipmapMode == TextureMipmapMode::LoadOrGenerate && outMipmapLevels <= 1)) ? true : false;
	auto usage = Anvil::ImageUsageFlagBits::TRANSFER_SRC_BIT | Anvil::ImageUsageFlagBits::TRANSFER_DST_BIT | Anvil::ImageUsageFlagBits::SAMPLED_BIT;
	if(bGenerateMipmaps == true)
		outMipmapLevels = prosper::util::calculate_mipmap_count(width,height);
	auto bCubemap = item.cubemap;
	auto initializeImage = [&](Anvil::Format format) {
		prosper::util::ImageCreateInfo createInfo {};
		createInfo.width = width;
		createInfo.height = height;
		createInfo.format = format;
		if(outMipmapLevels > 1u)
			createInfo.flags |= prosper::util::ImageCreateInfo::Flags::FullMipmapChain;
		createInfo.memoryFeatures = prosper::util::MemoryFeatureFlags::DeviceLocal;
		createInfo.tiling = Anvil::ImageTiling::OPTIMAL;
		createInfo.usage = usage;
		createInfo.layers = (bCubemap == true) ? 6 : 1;
		createInfo.postCreateLayout = Anvil::ImageLayout::TRANSFER_DST_OPTIMAL;
		if(bCubemap == true)
			createInfo.flags |= prosper::util::ImageCreateInfo::Flags::Cubemap;
		outImage = prosper::util::create_image(dev,createInfo);
		outImage->SetDebugName("generic_dst_img");
	};
	if(context.IsImageFormatSupported(format,usage) == true)
		initializeImage(format);
	else
	{
		initializeImage(Anvil::Format::R8G8B8A8_UNORM);
		bValidSrcImages = false;
	}

	auto &setupCmd = context.GetSetupCommandBuffer();
	if(bValidSrcImages == true)
	{
		if(bUseSrcBuffers == false)
		{
			auto numLayers = umath::min(static_cast<uint32_t>(srcImages.size()),outImage->GetLayerCount());
			for(auto layer=decltype(numLayers){0};layer<numLayers;++layer)
			{
				auto &mipLevels = srcImages[layer].mipLevels;
				auto mipmapLevels = umath::min(static_cast<uint32_t>(mipLevels.size()),outImage->GetMipmapCount());
				for(auto level=decltype(mipmapLevels){0};level<mipmapLevels;++level)
				{
					auto &img = mipLevels[level];
					auto extents = (*img)->get_image_extent_2D(0u);

					prosper::util::CopyInfo copyInfo {};
					copyInfo.srcSubresource = Anvil::ImageSubresourceLayers{
						Anvil::ImageAspectFlagBits::COLOR_BIT,
						0u, // mip level
						0u, // base array layer
						1u // layer count
					};
					copyInfo.dstSubresource = Anvil::ImageSubresourceLayers{
						Anvil::ImageAspectFlagBits::COLOR_BIT,
						level, // mip level
						layer, // base array layer
						1u // layer count
					};
					copyInfo.width = extents.width;
					copyInfo.height = extents.height;
					prosper::util::record_copy_image(setupCmd->GetAnvilCommandBuffer(),copyInfo,img->GetAnvilImage(),outImage->GetAnvilImage());
				}
			}
		}
		else
		{
			prosper::util::BufferImageCopyInfo copyInfo {};
			auto numLayers = umath::min(static_cast<uint32_t>(srcImages.size()),outImage->GetLayerCount());
			for(auto layer=decltype(numLayers){0};layer<numLayers;++layer)
			{
				auto &mipLevels = (*srcBuffers)[layer].mipLevels;
				auto mipmapLevels = umath::min(static_cast<uint32_t>(mipLevels.size()),outImage->GetMipmapCount());
				for(auto level=decltype(mipmapLevels){0};level<mipmapLevels;++level)
				{
					auto extent = (*outImage)->get_image_extent_2D(level);
					copyInfo.mipLevel = level;
					copyInfo.width = extent.width;
					copyInfo.height = extent.height;
					prosper::util::record_copy_buffer_to_image(setupCmd->GetAnvilCommandBuffer(),copyInfo,*mipLevels.at(level),outImage->GetAnvilImage());
				}
			}
		}
	}
	if(bGenerateMipmaps == false)
		change_image_transfer_dst_layout_to_shader_read(setupCmd->GetAnvilCommandBuffer(),outImage->GetAnvilImage());
	context.FlushSetupCommandBuffer(); // Make sure image copy is complete before generating mipmaps

	if(bGenerateMipmaps == true)
	{
		auto &setupCmd = context.GetSetupCommandBuffer();
		prosper::util::record_generate_mipmaps(setupCmd->GetAnvilCommandBuffer(),outImage->GetAnvilImage(),Anvil::ImageLayout::TRANSFER_DST_OPTIMAL,Anvil::AccessFlagBits::TRANSFER_WRITE_BIT,Anvil::PipelineStageFlagBits::TRANSFER_BIT);
		context.FlushSetupCommandBuffer();
	}
}

#ifdef ENABLE_VTF_SUPPORT

static Anvil::Format vtf_format_to_vulkan_format(VTFImageFormat format,Anvil::Format *internalFormat=nullptr)
{
	switch(format)
	{
		case VTFImageFormat::IMAGE_FORMAT_DXT1:
			if(internalFormat != nullptr)
				*internalFormat = Anvil::Format::BC1_RGB_UNORM_BLOCK;
			return Anvil::Format::BC1_RGBA_UNORM_BLOCK;
		case VTFImageFormat::IMAGE_FORMAT_DXT3:
			if(internalFormat != nullptr)
				*internalFormat = Anvil::Format::BC2_UNORM_BLOCK;
			return Anvil::Format::BC2_UNORM_BLOCK;
		case VTFImageFormat::IMAGE_FORMAT_DXT5:
			if(internalFormat != nullptr)
				*internalFormat = Anvil::Format::BC3_UNORM_BLOCK;
			return Anvil::Format::BC3_UNORM_BLOCK;
		case VTFImageFormat::IMAGE_FORMAT_RGBA8888:
			if(internalFormat != nullptr)
				*internalFormat = Anvil::Format::R8G8B8A8_UNORM;
			return Anvil::Format::R8G8B8A8_UNORM;
		case VTFImageFormat::IMAGE_FORMAT_RGB888: // Needs to be converted
			if(internalFormat != nullptr)
				*internalFormat = Anvil::Format::R8G8B8_UNORM;
			return Anvil::Format::R8G8B8A8_UNORM;
		case VTFImageFormat::IMAGE_FORMAT_BGRA8888: // Needs to be converted
			if(internalFormat != nullptr)
				*internalFormat = Anvil::Format::B8G8R8A8_UNORM;
			return Anvil::Format::R8G8B8A8_UNORM;
		case VTFImageFormat::IMAGE_FORMAT_BGR888: // Needs to be converted
			if(internalFormat != nullptr)
				*internalFormat = Anvil::Format::B8G8R8_UNORM;
			return Anvil::Format::R8G8B8A8_UNORM;
		case VTFImageFormat::IMAGE_FORMAT_UV88: // Needs to be converted
			if(internalFormat != nullptr)
				*internalFormat = Anvil::Format::R8G8_UNORM;
			return Anvil::Format::R8G8B8A8_UNORM;
		case VTFImageFormat::IMAGE_FORMAT_RGBA16161616F:
			if(internalFormat != nullptr)
				*internalFormat = Anvil::Format::R16G16B16A16_SFLOAT;
			return Anvil::Format::R16G16B16A16_SFLOAT;
		case VTFImageFormat::IMAGE_FORMAT_RGBA32323232F:
			if(internalFormat != nullptr)
				*internalFormat = Anvil::Format::R32G32B32A32_SFLOAT;
			return Anvil::Format::R32G32B32A32_SFLOAT;
		default:
			if(internalFormat != nullptr)
				*internalFormat = Anvil::Format::BC1_RGBA_UNORM_BLOCK;
			return Anvil::Format::BC1_RGBA_UNORM_BLOCK;
	}
}

// Initialize VTF image
static void initialize_image(TextureQueueItem &item,std::vector<std::shared_ptr<VTFLib::CVTFFile>> &vtfFiles,std::shared_ptr<prosper::Image> &outImage)
{
	auto &context = *item.context.lock();
	auto &dev = context.GetDevice();
	auto numTextures = vtfFiles.size(); // vtf.GetFaceCount()?
	initialize_generic_image(item,numTextures,outImage,[&context,&vtfFiles,&item,&dev](uint32_t layer,std::vector<std::shared_ptr<prosper::Image>> &mipLevels,uint32_t &outWidth,uint32_t &outHeight,Anvil::Format &format,uint32_t &outMipmapLevels) -> bool {
		auto &vtf = *vtfFiles.at(layer);
		auto mipmapLevels = vtf.GetMipmapCount();
		auto vtfFormat = vtf.GetFormat();
		auto width = vtf.GetWidth();
		auto height = vtf.GetHeight();
		if(layer == 0)
		{
			outWidth = width;
			outHeight = height;
			format = vtf_format_to_vulkan_format(vtfFormat);
			if(item.mipmapMode == TextureMipmapMode::Ignore || item.mipmapMode == TextureMipmapMode::Generate)
				mipmapLevels = 1;
			outMipmapLevels = mipmapLevels;

			if(context.IsImageFormatSupported(format,Anvil::ImageUsageFlagBits::TRANSFER_SRC_BIT) == false)
				return false;
		}
		mipLevels.resize(mipmapLevels);
		for(auto level=decltype(mipmapLevels){0};level<mipmapLevels;++level)
		{
			vlUInt mipmapDepth = 0;
			uint32_t mipWidth,mipHeight;
			vtf.ComputeMipmapDimensions(width,height,0,level,mipWidth,mipHeight,mipmapDepth);

			prosper::util::ImageCreateInfo createInfo {};
			createInfo.width = mipWidth;
			createInfo.height = mipHeight;
			createInfo.format = format;
			createInfo.memoryFeatures = prosper::util::MemoryFeatureFlags::HostAccessable;
			createInfo.tiling = Anvil::ImageTiling::LINEAR;
			createInfo.usage = Anvil::ImageUsageFlagBits::TRANSFER_SRC_BIT;
			createInfo.postCreateLayout = Anvil::ImageLayout::TRANSFER_SRC_OPTIMAL;
			auto img = prosper::util::create_image(dev,createInfo);
			img->SetDebugName("vtf_src_img");

			auto *vtfDataSrc = vtf.GetData(0,layer,0,level);
			auto *vtfDataDst = vtfDataSrc;
			auto vtfFormat = vtf.GetFormat();
			uint32_t size = 0;

			std::vector<uint8_t> vtfDataCnv;
			if(vtfFormat == VTFImageFormat::IMAGE_FORMAT_BGRA8888 || vtfFormat == VTFImageFormat::IMAGE_FORMAT_BGR888 || vtfFormat == VTFImageFormat::IMAGE_FORMAT_UV88)
			{
				vtfDataCnv.resize(mipWidth *mipHeight *4);
				auto numPixels = mipWidth *mipHeight;
				decltype(numPixels) srcOffset = 0;
				std::array<uint8_t,4> pixelOrder = {0,1,2,3};
				if(vtfFormat == VTFImageFormat::IMAGE_FORMAT_BGRA8888)
				{
					pixelOrder.at(0) = 2; // Blue -> Red
					pixelOrder.at(2) = 0; // Red -> Blue
					for(auto i=decltype(numPixels){0};i<numPixels *4;i+=4)
					{
						vtfDataCnv.at(i +pixelOrder.at(0)) = vtfDataSrc[srcOffset];
						vtfDataCnv.at(i +pixelOrder.at(1)) = vtfDataSrc[srcOffset +1];
						vtfDataCnv.at(i +pixelOrder.at(2)) = vtfDataSrc[srcOffset +2];
						vtfDataCnv.at(i +pixelOrder.at(3)) = vtfDataSrc[srcOffset +3];

						srcOffset += 4;
					}
				}
				else if(vtfFormat == VTFImageFormat::IMAGE_FORMAT_UV88)
				{
					for(auto i=decltype(numPixels){0};i<numPixels *4;i+=4)
					{
						vtfDataCnv.at(i +pixelOrder.at(0)) = vtfDataSrc[srcOffset];
						vtfDataCnv.at(i +pixelOrder.at(1)) = vtfDataSrc[srcOffset +1];
						vtfDataCnv.at(i +pixelOrder.at(2)) = 0;
						vtfDataCnv.at(i +pixelOrder.at(3)) = std::numeric_limits<uint8_t>::max();

						srcOffset += 2;
					}
				}
				else
				{
					if(vtfFormat == VTFImageFormat::IMAGE_FORMAT_BGR888)
					{
						pixelOrder.at(0) = 2; // Blue -> Red
						pixelOrder.at(2) = 0; // Red -> Blue
					}
					for(auto i=decltype(numPixels){0};i<numPixels *4;i+=4)
					{
						vtfDataCnv.at(i +pixelOrder.at(0)) = vtfDataSrc[srcOffset];
						vtfDataCnv.at(i +pixelOrder.at(1)) = vtfDataSrc[srcOffset +1];
						vtfDataCnv.at(i +pixelOrder.at(2)) = vtfDataSrc[srcOffset +2];
						vtfDataCnv.at(i +pixelOrder.at(3)) = std::numeric_limits<std::remove_reference_t<decltype(vtfDataCnv.front())>>::max();

						srcOffset += 3;
					}
				}
				vtfDataDst = vtfDataCnv.data();
				size = vtfDataCnv.size();
			}
			else
				size = vtf.ComputeMipmapSize(width,height,0,level,vtfFormat);

			Anvil::SubresourceLayout resLayout;
			(*img)->get_aspect_subresource_layout(Anvil::ImageAspectFlagBits::COLOR_BIT,0u,0u,&resLayout);
			auto memBlock = (*img)->get_memory_block();
			memBlock->map(resLayout.offset,resLayout.size);
			memBlock->write(0ull,size,vtfDataDst);
			memBlock->unmap();

			mipLevels[level] = std::move(img);
		}
		return true;
	},[&vtfFiles,&context,&dev](uint32_t layer,std::vector<std::shared_ptr<prosper::Buffer>> &mipLevels) {
		auto &vtf = *vtfFiles.at(layer);
		auto width = vtf.GetWidth();
		auto height = vtf.GetHeight();
		auto mipmapLevels = vtf.GetMipmapCount();
		auto format = vtf.GetFormat();
		mipLevels.resize(mipmapLevels,nullptr);
		for(auto level=decltype(mipmapLevels){0};level<mipmapLevels;++level)
		{
			prosper::util::BufferCreateInfo createInfo {};
			createInfo.usageFlags = Anvil::BufferUsageFlagBits::TRANSFER_SRC_BIT;
			createInfo.size = vtf.ComputeMipmapSize(width,height,0,level,format);
			mipLevels[level] = prosper::util::create_buffer(dev,createInfo,vtf.GetData(0,layer,0,level));
		}
	});
}
#endif
// Initialize PNG image
static void initialize_png_image(TextureQueueItem &item,uimg::Image &png,std::shared_ptr<prosper::Image> &outImage)
{
	auto format = prosper::util::get_vk_format(png.GetType());
	auto &context = *item.context.lock();
	auto &dev = context.GetDevice();
	prosper::util::ImageCreateInfo createInfo {};
	createInfo.width = png.GetWidth();
	createInfo.height = png.GetHeight();
	createInfo.format = format;
	createInfo.tiling = Anvil::ImageTiling::LINEAR;
	createInfo.usage = Anvil::ImageUsageFlagBits::TRANSFER_SRC_BIT;
	createInfo.memoryFeatures = prosper::util::MemoryFeatureFlags::HostAccessable;
	createInfo.postCreateLayout = Anvil::ImageLayout::TRANSFER_SRC_OPTIMAL;
	auto imgSrc = prosper::util::create_image(dev,createInfo,png.GetData().data());
	imgSrc->SetDebugName("png_src_img");

	auto bGenerateMipmaps = (item.mipmapMode == TextureMipmapMode::Generate || item.mipmapMode == TextureMipmapMode::LoadOrGenerate) ? true : false;
	createInfo = {};
	createInfo.width = png.GetWidth();
	createInfo.height = png.GetHeight();
	createInfo.format = format;
	if(bGenerateMipmaps == true)
		createInfo.flags |= prosper::util::ImageCreateInfo::Flags::FullMipmapChain;
	createInfo.memoryFeatures = prosper::util::MemoryFeatureFlags::DeviceLocal;
	createInfo.tiling = Anvil::ImageTiling::OPTIMAL;
	createInfo.usage = Anvil::ImageUsageFlagBits::TRANSFER_SRC_BIT | Anvil::ImageUsageFlagBits::TRANSFER_DST_BIT | Anvil::ImageUsageFlagBits::SAMPLED_BIT;
	createInfo.postCreateLayout = Anvil::ImageLayout::TRANSFER_DST_OPTIMAL;
	outImage = prosper::util::create_image(dev,createInfo);
	outImage->SetDebugName("png_dst_img");

	auto &setupCmd = context.GetSetupCommandBuffer();
	prosper::util::CopyInfo copyInfo {};
	copyInfo.width = png.GetWidth();
	copyInfo.height = png.GetHeight();
	prosper::util::record_copy_image(setupCmd->GetAnvilCommandBuffer(),copyInfo,imgSrc->GetAnvilImage(),outImage->GetAnvilImage());
	if(bGenerateMipmaps == false)
		change_image_transfer_dst_layout_to_shader_read(setupCmd->GetAnvilCommandBuffer(),outImage->GetAnvilImage());
	context.FlushSetupCommandBuffer(); // Make sure image copy is complete before generating mipmaps

	if(bGenerateMipmaps == true)
	{
		auto &setupCmd = context.GetSetupCommandBuffer();
		prosper::util::record_generate_mipmaps(setupCmd->GetAnvilCommandBuffer(),outImage->GetAnvilImage(),Anvil::ImageLayout::TRANSFER_DST_OPTIMAL,Anvil::AccessFlagBits::TRANSFER_WRITE_BIT,Anvil::PipelineStageFlagBits::TRANSFER_BIT);
		context.FlushSetupCommandBuffer();
	}
}
// Initialize DDS or KTX image
static void initialize_image(TextureQueueItem &item,std::vector<std::unique_ptr<gli::texture2d>> &textures,std::shared_ptr<prosper::Image> &outImage)
{
	auto &context = *item.context.lock();
	auto &dev = context.GetDevice();
	initialize_generic_image(item,textures.size(),outImage,[&context,&textures,&item,&dev](uint32_t layer,std::vector<std::shared_ptr<prosper::Image>> &mipLevels,uint32_t &width,uint32_t &height,Anvil::Format &format,uint32_t &outMipmapLevels) -> bool {
		auto &tex2D = *textures[layer];
		auto img = tex2D[0];
		auto mipmapLevels = tex2D.levels();
		if(layer == 0) // Assuming same width, height and format for all images
		{
			auto extent = img.extent();
			width = extent.x;
			height = extent.y;
			format = static_cast<Anvil::Format>(tex2D.format());
			if(item.mipmapMode == TextureMipmapMode::Ignore || item.mipmapMode == TextureMipmapMode::Generate)
				mipmapLevels = 1;
			outMipmapLevels = mipmapLevels;
			if(context.IsImageFormatSupported(format,Anvil::ImageUsageFlagBits::TRANSFER_SRC_BIT) == false)
				return false;
		}
		mipLevels.resize(mipmapLevels);
		for(auto level=decltype(mipmapLevels){0};level<mipmapLevels;++level)
		{
			auto extent = tex2D[level].extent();

			prosper::util::ImageCreateInfo createInfo {};
			createInfo.width = extent.x;
			createInfo.height = extent.y;
			createInfo.format = format;
			createInfo.memoryFeatures = prosper::util::MemoryFeatureFlags::HostAccessable;
			createInfo.tiling = Anvil::ImageTiling::LINEAR;
			createInfo.usage = Anvil::ImageUsageFlagBits::TRANSFER_SRC_BIT;
			createInfo.postCreateLayout = Anvil::ImageLayout::TRANSFER_SRC_OPTIMAL;
			auto img = prosper::util::create_image(dev,createInfo);
			img->SetDebugName("");

			Anvil::SubresourceLayout resLayout;
			(*img)->get_aspect_subresource_layout(Anvil::ImageAspectFlagBits::COLOR_BIT,0u,0u,&resLayout);
			auto memBlock = (*img)->get_memory_block();
			memBlock->map(resLayout.offset,resLayout.size);
			memBlock->write(0ull,tex2D[level].size(),tex2D[level].data());
			memBlock->unmap();

			mipLevels[level] = std::move(img);
		}
		return true;
	},[&textures,&context,&dev](uint32_t layer,std::vector<std::shared_ptr<prosper::Buffer>> &mipLevels) {
		auto &tex2D = *textures[layer];
		auto mipmapLevels = tex2D.levels();
		mipLevels.resize(mipmapLevels,nullptr);
		for(auto level=decltype(mipmapLevels){0};level<mipmapLevels;++level)
		{
			prosper::util::BufferCreateInfo createInfo {};
			createInfo.usageFlags = Anvil::BufferUsageFlagBits::TRANSFER_SRC_BIT;
			createInfo.size = tex2D[level].size();
			mipLevels[level] = prosper::util::create_buffer(dev,createInfo,tex2D[level].data());
		}
	});
}
// Initialize TGA image
const auto TGA_VK_FORMAT = Anvil::Format::R8G8B8A8_UNORM;
static void initialize_tga_image(TextureQueueItem &item,uimg::Image &tga,std::shared_ptr<prosper::Image> &outImage)
{
	auto format = TGA_VK_FORMAT;
	auto &context = *item.context.lock();
	auto &dev = context.GetDevice();
	prosper::util::ImageCreateInfo createInfo {};
	createInfo.width = tga.GetWidth();
	createInfo.height = tga.GetHeight();
	createInfo.format = format;
	createInfo.tiling = Anvil::ImageTiling::LINEAR;
	createInfo.usage = Anvil::ImageUsageFlagBits::TRANSFER_SRC_BIT;
	createInfo.memoryFeatures = prosper::util::MemoryFeatureFlags::HostAccessable;
	createInfo.postCreateLayout = Anvil::ImageLayout::TRANSFER_SRC_OPTIMAL;
	auto imgSrc = prosper::util::create_image(dev,createInfo);
	imgSrc->SetDebugName("tga_src_img");
	prosper::util::initialize_image(dev,tga,imgSrc->GetAnvilImage());

	auto bGenerateMipmaps = (item.mipmapMode == TextureMipmapMode::Generate || item.mipmapMode == TextureMipmapMode::LoadOrGenerate) ? true : false;
	createInfo = {};
	createInfo.width = tga.GetWidth();
	createInfo.height = tga.GetHeight();
	createInfo.format = format;
	createInfo.flags |= prosper::util::ImageCreateInfo::Flags::FullMipmapChain;
	createInfo.memoryFeatures = prosper::util::MemoryFeatureFlags::DeviceLocal;
	createInfo.tiling = Anvil::ImageTiling::OPTIMAL;
	createInfo.usage = Anvil::ImageUsageFlagBits::TRANSFER_SRC_BIT | Anvil::ImageUsageFlagBits::TRANSFER_DST_BIT | Anvil::ImageUsageFlagBits::SAMPLED_BIT;
	createInfo.postCreateLayout = Anvil::ImageLayout::TRANSFER_DST_OPTIMAL;
	outImage = prosper::util::create_image(dev,createInfo);
	outImage->SetDebugName("tga_dst_img");

	auto &setupCmd = context.GetSetupCommandBuffer();
	prosper::util::CopyInfo copyInfo {};
	copyInfo.width = tga.GetWidth();
	copyInfo.height = tga.GetHeight();
	prosper::util::record_copy_image(setupCmd->GetAnvilCommandBuffer(),copyInfo,imgSrc->GetAnvilImage(),outImage->GetAnvilImage());
	if(bGenerateMipmaps == false)
		change_image_transfer_dst_layout_to_shader_read(setupCmd->GetAnvilCommandBuffer(),outImage->GetAnvilImage());
	context.FlushSetupCommandBuffer(); // Make sure image copy is complete before generating mipmaps

	if(bGenerateMipmaps == true)
	{
		auto &setupCmd = context.GetSetupCommandBuffer();
		prosper::util::record_generate_mipmaps(setupCmd->GetAnvilCommandBuffer(),outImage->GetAnvilImage(),Anvil::ImageLayout::TRANSFER_DST_OPTIMAL,Anvil::AccessFlagBits::TRANSFER_WRITE_BIT,Anvil::PipelineStageFlagBits::TRANSFER_BIT);
		context.FlushSetupCommandBuffer();
	}
}
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
		auto *surface = dynamic_cast<TextureQueueItemSurface*>(&item);
		if(surface != nullptr)
		{
			auto &img = surface->textures.front();
			auto extent = img->extent();
			texture->width = extent.x;
			texture->height = extent.y;
			texture->internalFormat = static_cast<Anvil::Format>(img->format());
			initialize_image(item,surface->textures,image);
		}
		else
		{
			auto *png = dynamic_cast<TextureQueueItemPNG*>(&item);
			if(png != nullptr)
			{
				auto &pngInfo = *png->pnginfo;
				texture->width = pngInfo.GetWidth();
				texture->height = pngInfo.GetHeight();
				texture->internalFormat = prosper::util::get_vk_format(pngInfo.GetType());
				initialize_png_image(item,pngInfo,image);
			}
			else
			{
				auto *tga = dynamic_cast<TextureQueueItemTGA*>(&item);
				if(tga != nullptr)
				{
					auto &tgaInfo = *tga->tgainfo;
					texture->width = tgaInfo.GetWidth();
					texture->height = tgaInfo.GetHeight();
					texture->internalFormat = TGA_VK_FORMAT;
					initialize_tga_image(item,tgaInfo,image);
				}
#ifdef ENABLE_VTF_SUPPORT
				else
				{
					auto *vtf = dynamic_cast<TextureQueueItemVTF*>(&item);
					if(vtf != nullptr)
					{
						auto &vtfFile = vtf->textures.front();
						texture->width = vtfFile->GetWidth();
						texture->height = vtfFile->GetHeight();
						vtf_format_to_vulkan_format(vtfFile->GetFormat(),&texture->internalFormat);
						initialize_image(item,vtf->textures,image);
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
			texture->texture = prosper::util::create_texture(dev,createInfo,std::move(image),&imgViewCreateInfo);
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
