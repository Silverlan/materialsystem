/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "texturemanager/load/texture_processor.hpp"
#include "texturemanager/load/texture_format_handler.hpp"
#include "texturemanager/load/texture_loader.hpp"
#include <prosper_context.hpp>
#include <prosper_util.hpp>
#include <prosper_command_buffer.hpp>
#include <image/prosper_texture.hpp>
#include <image/prosper_image.hpp>
#include <image/prosper_sampler.hpp>
#include <util_image_buffer.hpp>

bool msys::TextureProcessor::PrepareImage(prosper::IPrContext &context)
{
	return InitializeProsperImage(context) && InitializeImageBuffers(context);
}

bool msys::TextureProcessor::FinalizeImage(prosper::IPrContext &context)
{
	if(targetGpuConversionFormat.has_value() && ConvertImageFormat(context,*targetGpuConversionFormat) == false)
		return false;

	if(m_generateMipmaps && GenerateMipmaps(context) == false)
		return false;

	auto &inputTexInfo = handler->GetInputTextureInfo();
	auto hasMipmaps = image->GetMipmapCount() > 0;
	prosper::util::TextureCreateInfo createInfo {};
	createInfo.sampler = hasMipmaps ? m_loader.GetTextureSampler() : m_loader.GetTextureSamplerNoMipmap();
	auto &swizzle = inputTexInfo.swizzle;
	prosper::util::ImageViewCreateInfo imgViewCreateInfo {};
	imgViewCreateInfo.swizzleRed = swizzle.at(0);
	imgViewCreateInfo.swizzleGreen = swizzle.at(1);
	imgViewCreateInfo.swizzleBlue = swizzle.at(2);
	imgViewCreateInfo.swizzleAlpha = swizzle.at(3);
	createInfo.flags |= prosper::util::TextureCreateInfo::Flags::CreateImageViewForEachLayer;
	texture = context.CreateTexture(createInfo,*image,imgViewCreateInfo);
	return true;
}

msys::TextureProcessor::TextureProcessor(TextureLoader &loader,std::unique_ptr<ITextureFormatHandler> &&handler)
	: handler{std::move(handler)},m_loader{loader}
{}

bool msys::TextureProcessor::Load()
{
	if(!handler->LoadData())
		return false;
	return !m_loader.DoesAllowMultiThreadedGpuResourceAllocation() || PrepareImage(m_loader.GetContext());
}
bool msys::TextureProcessor::Finalize()
{
	return (m_loader.DoesAllowMultiThreadedGpuResourceAllocation() || PrepareImage(m_loader.GetContext())) && FinalizeImage(m_loader.GetContext());
}

bool msys::TextureProcessor::InitializeProsperImage(prosper::IPrContext &context)
{
	auto &inputTextureInfo = handler->GetInputTextureInfo();
	imageFormat = inputTextureInfo.format;
	const auto width = inputTextureInfo.width;
	const auto height = inputTextureInfo.height;

	// In some cases the format may not be supported by the GPU altogether. We may still be able to convert it to a compatible format by hand.
	std::function<void(const void*,std::shared_ptr<uimg::ImageBuffer>&,uint32_t,uint32_t)> manualConverter = nullptr;
	const auto usage = prosper::ImageUsageFlags::TransferSrcBit | prosper::ImageUsageFlags::TransferDstBit | prosper::ImageUsageFlags::SampledBit;
	if(imageFormat == prosper::Format::B8G8R8_UNorm_PoorCoverage && context.IsImageFormatSupported(imageFormat,usage,prosper::ImageType::e2D,prosper::ImageTiling::Optimal) == false)
	{
		cpuImageConverter = [](const void *imgData,std::shared_ptr<uimg::ImageBuffer> &outImg,uint32_t width,uint32_t height) {
			outImg = uimg::ImageBuffer::Create(imgData,width,height,uimg::Format::RGB8);
			outImg->Convert(uimg::Format::RGBA8);
		};

		imageFormat = prosper::Format::B8G8R8A8_UNorm;
	}
	if(context.IsImageFormatSupported(imageFormat,usage) == false || (targetGpuConversionFormat.has_value() && context.IsImageFormatSupported(*targetGpuConversionFormat,usage) == false))
		return false;

	mipmapCount = inputTextureInfo.mipmapCount;
	m_generateMipmaps = (mipmapMode == TextureMipmapMode::Generate || (mipmapMode == TextureMipmapMode::LoadOrGenerate && mipmapCount <= 1)) ? true : false;
	if(m_generateMipmaps == true)
		mipmapCount = prosper::util::calculate_mipmap_count(width,height);

	auto cubemap = umath::is_flag_set(inputTextureInfo.flags,ITextureFormatHandler::InputTextureInfo::Flags::CubemapBit);

	// Initialize output image
	prosper::util::ImageCreateInfo createInfo {};
	createInfo.width = width;
	createInfo.height = height;
	createInfo.format = imageFormat;
	if(mipmapCount > 1u)
		createInfo.flags |= prosper::util::ImageCreateInfo::Flags::FullMipmapChain;
	createInfo.memoryFeatures = prosper::MemoryFeatureFlags::DeviceLocal;
	createInfo.tiling = prosper::ImageTiling::Optimal;
	createInfo.usage = usage;
	createInfo.layers = cubemap ? 6 : 1;
	createInfo.postCreateLayout = prosper::ImageLayout::TransferDstOptimal;
	if(cubemap)
		createInfo.flags |= prosper::util::ImageCreateInfo::Flags::Cubemap;

	auto img = context.CreateImage(createInfo);
	if(img == nullptr)
		return false;
	img->SetDebugName("texture_asset_img");
	image = img;
	return true;
}

bool msys::TextureProcessor::InitializeImageBuffers(prosper::IPrContext &context)
{
	// Initialize image data as buffers, then copy to output image
	auto &inputTextureInfo = handler->GetInputTextureInfo();
	auto numLayers = inputTextureInfo.layerCount;
	auto mipmapCount = inputTextureInfo.mipmapCount;
	buffers.reserve(numLayers *mipmapCount);
	auto &imgBuffers = m_tmpImgBuffers;
	imgBuffers.reserve(numLayers *mipmapCount);
	for(auto iLayer=decltype(numLayers){0u};iLayer<numLayers;++iLayer)
	{
		for(auto iMipmap=decltype(mipmapCount){0u};iMipmap<mipmapCount;++iMipmap)
		{
			size_t dataSize;
			void *data;

			if(handler->GetDataPtr(iLayer,iMipmap,&data,dataSize) == false || data == nullptr)
				continue;

			if(cpuImageConverter)
			{
				uint32_t wMipmap,hMipmap;
				prosper::util::calculate_mipmap_size(inputTextureInfo.width,inputTextureInfo.height,&wMipmap,&hMipmap,iMipmap);
				std::shared_ptr<uimg::ImageBuffer> imgBuffer = nullptr;
				cpuImageConverter(data,imgBuffer,wMipmap,hMipmap);
				data = imgBuffer->GetData();
				dataSize = imgBuffer->GetSize();

				imgBuffers.push_back(imgBuffer);
			}

			// Initialize buffer with source image data
			auto buf = context.AllocateTemporaryBuffer(dataSize,0u /* alignment */,data);
			buffers.push_back({buf,iLayer,iMipmap}); // We need to keep the buffers alive until the copy has completed
		}
	}
	return true;
}

bool msys::TextureProcessor::CopyBuffersToImage(prosper::IPrContext &context)
{
	// Note: We need a separate loop here, because the underlying VkBuffer of 'AllocateTemporaryBuffer' may get invalidated if the function
	// is called multiple times.
	auto &setupCmd = context.GetSetupCommandBuffer();
	for(auto &bufInfo : buffers)
	{
		// Copy buffer contents to output image
		auto extent = image->GetExtents(bufInfo.mipmapIndex);
		prosper::util::BufferImageCopyInfo copyInfo {};
		copyInfo.mipLevel = bufInfo.mipmapIndex;
		copyInfo.baseArrayLayer = bufInfo.layerIndex;
		copyInfo.width = extent.width;
		copyInfo.height = extent.height;
		setupCmd->RecordCopyBufferToImage(copyInfo,*bufInfo.buffer,*image);
	}

	if(m_generateMipmaps)
		setupCmd->RecordImageBarrier(*image,prosper::ImageLayout::TransferDstOptimal,prosper::ImageLayout::ShaderReadOnlyOptimal);
	context.FlushSetupCommandBuffer(); // Make sure image copy is complete before generating mipmaps

	// Don't need these anymore
	buffers.clear();
	m_tmpImgBuffers.clear();
	return true;
}

bool msys::TextureProcessor::ConvertImageFormat(prosper::IPrContext &context,prosper::Format targetFormat)
{
	auto &setupCmd = context.GetSetupCommandBuffer();

	// Image needs to be converted
	auto createInfo = image->GetCreateInfo();
	createInfo.format = targetFormat;
	createInfo.tiling = prosper::ImageTiling::Optimal;
	createInfo.postCreateLayout = prosper::ImageLayout::TransferDstOptimal;

	auto convertedImage = image->Copy(*setupCmd,createInfo);
	setupCmd->RecordImageBarrier(*image,prosper::ImageLayout::ShaderReadOnlyOptimal,prosper::ImageLayout::TransferSrcOptimal);
	setupCmd->RecordBlitImage({},*image,*convertedImage);
	setupCmd->RecordImageBarrier(*convertedImage,prosper::ImageLayout::TransferDstOptimal,prosper::ImageLayout::ShaderReadOnlyOptimal);

	context.FlushSetupCommandBuffer();
	image = convertedImage;
	return true;
}
bool msys::TextureProcessor::GenerateMipmaps(prosper::IPrContext &context)
{
	auto &setupCmd = context.GetSetupCommandBuffer();
	setupCmd->RecordGenerateMipmaps(*image,prosper::ImageLayout::TransferDstOptimal,prosper::AccessFlags::TransferWriteBit,prosper::PipelineStageFlags::TransferBit);
	context.FlushSetupCommandBuffer();
	return true;
}
