// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <VTFFile.h>

module pragma.cmaterialsystem;

import :texture_manager;

#ifndef DISABLE_VTEX_SUPPORT
import source2;
#endif

#define FLUSH_INIT_CMD 1

static void change_image_transfer_dst_layout_to_shader_read(prosper::ICommandBuffer &cmdBuffer, prosper::IImage &img) { cmdBuffer.RecordImageBarrier(img, prosper::ImageLayout::TransferDstOptimal, prosper::ImageLayout::ShaderReadOnlyOptimal); }

struct ImageFormatLoader {
	void *userData = nullptr;
	std::function<void(void *, const pragma::material::TextureQueueItem &, uint32_t &, uint32_t &, prosper::Format &, bool &, uint32_t &, uint32_t &, std::optional<prosper::Format> &)> get_image_info = nullptr;
	std::function<const void *(void *, const pragma::material::TextureQueueItem &, uint32_t, uint32_t, uint32_t &)> get_image_data = nullptr;
};

static void initialize_image(pragma::material::TextureQueueItem &item, const pragma::material::Texture &texture, const ImageFormatLoader &imgLoader, std::shared_ptr<prosper::IImage> &outImage)
{
	auto &context = *item.context.lock();

	uint32_t width = 0;
	uint32_t height = 0;
	prosper::Format format = prosper::Format::R8G8B8A8_UNorm;
	std::optional<prosper::Format> conversionFormat = {};
	uint32_t numLayers = 1u;
	uint32_t numMipMaps = 1u;
	imgLoader.get_image_info(imgLoader.userData, item, width, height, format, item.cubemap, numLayers, numMipMaps, conversionFormat);

	// In some cases the format may not be supported by the GPU altogether. We may still be able to convert it to a compatible format by hand.
	std::function<void(const void *, std::shared_ptr<pragma::image::ImageBuffer> &, uint32_t, uint32_t)> manualConverter = nullptr;
	const auto usage = prosper::ImageUsageFlags::TransferSrcBit | prosper::ImageUsageFlags::TransferDstBit | prosper::ImageUsageFlags::SampledBit;
	if(format == prosper::Format::B8G8R8_UNorm_PoorCoverage && context.IsImageFormatSupported(format, usage, prosper::ImageType::e2D, prosper::ImageTiling::Optimal) == false) {
		manualConverter = [](const void *imgData, std::shared_ptr<pragma::image::ImageBuffer> &outImg, uint32_t width, uint32_t height) {
			outImg = pragma::image::ImageBuffer::Create(imgData, width, height, pragma::image::Format::RGB8);
			outImg->Convert(pragma::image::Format::RGBA8);
		};
		format = prosper::Format::B8G8R8A8_UNorm;
		conversionFormat = {};
	}
	if(context.IsImageFormatSupported(format, usage) == false || (conversionFormat.has_value() && context.IsImageFormatSupported(*conversionFormat, usage) == false))
		return;

	auto numMipMapsLoad = numMipMaps;
	auto bGenerateMipmaps = (item.mipmapMode == pragma::material::TextureMipmapMode::Generate || (item.mipmapMode == pragma::material::TextureMipmapMode::LoadOrGenerate && numMipMaps <= 1)) ? true : false;
	if(bGenerateMipmaps == true)
		numMipMaps = prosper::util::calculate_mipmap_count(width, height);

	auto bCubemap = item.cubemap;

	// Initialize output image
	prosper::util::ImageCreateInfo createInfo {};
	createInfo.width = width;
	createInfo.height = height;
	createInfo.format = format;
	if(numMipMaps > 1u)
		createInfo.flags |= prosper::util::ImageCreateInfo::Flags::FullMipmapChain;
	createInfo.memoryFeatures = prosper::MemoryFeatureFlags::DeviceLocal;
	createInfo.tiling = prosper::ImageTiling::Optimal;
	createInfo.usage = usage;
	createInfo.layers = (bCubemap == true) ? 6 : 1;
	createInfo.postCreateLayout = prosper::ImageLayout::TransferDstOptimal;
	if(bCubemap == true)
		createInfo.flags |= prosper::util::ImageCreateInfo::Flags::Cubemap;

	outImage = context.CreateImage(createInfo);
	if(outImage == nullptr)
		return;

#if 0
	static uint64_t totalAllocatedSize = 0;
	auto allocatedSize = outImage->GetMemoryBuffer()->GetSize();
	totalAllocatedSize += allocatedSize;
	std::cout<<"Allocated "<<pragma::util::get_pretty_bytes(allocatedSize)<<" for image '"<<item.name<<"'. Total allocated: "<<pragma::util::get_pretty_bytes(totalAllocatedSize)<<std::endl;
#endif

	outImage->SetDebugName("texture_asset_img");

	// Initialize image data as buffers, then copy to output image
	struct BufferInfo {
		std::shared_ptr<prosper::IBuffer> buffer = nullptr;
		uint32_t layerIndex = 0u;
		uint32_t mipmapIndex = 0u;
	};
	std::vector<BufferInfo> buffers {};
	buffers.reserve(numLayers * numMipMapsLoad);
	std::vector<std::shared_ptr<pragma::image::ImageBuffer>> imgBuffers {};
	imgBuffers.reserve(numLayers * numMipMapsLoad);
	auto &setupCmd = context.GetSetupCommandBuffer();
	for(auto iLayer = decltype(numLayers) {0u}; iLayer < numLayers; ++iLayer) {
		for(auto iMipmap = decltype(numMipMapsLoad) {0u}; iMipmap < numMipMapsLoad; ++iMipmap) {
			uint32_t dataSize;
			auto *data = imgLoader.get_image_data(imgLoader.userData, item, iLayer, iMipmap, dataSize);

			if(data == nullptr)
				continue;

			if(manualConverter) {
				uint32_t wMipmap, hMipmap;
				prosper::util::calculate_mipmap_size(width, height, &wMipmap, &hMipmap, iMipmap);
				std::shared_ptr<pragma::image::ImageBuffer> imgBuffer = nullptr;
				manualConverter(data, imgBuffer, wMipmap, hMipmap);
				data = imgBuffer->GetData();
				dataSize = imgBuffer->GetSize();

				imgBuffers.push_back(imgBuffer);
			}

			// Initialize buffer with source image data
			auto buf = context.AllocateTemporaryBuffer(dataSize, 0u /* alignment */, data);
			buffers.push_back({buf, iLayer, iMipmap}); // We need to keep the buffers alive until the copy has completed
		}
	}

	// Note: We need a separate loop here, because the underlying VkBuffer of 'AllocateTemporaryBuffer' may get invalidated if the function
	// is called multiple times.
	for(auto &bufInfo : buffers) {
		// Copy buffer contents to output image
		auto extent = outImage->GetExtents(bufInfo.mipmapIndex);
		prosper::util::BufferImageCopyInfo copyInfo {};
		copyInfo.mipLevel = bufInfo.mipmapIndex;
		copyInfo.baseArrayLayer = bufInfo.layerIndex;
		copyInfo.imageExtent = {extent.width, extent.height};
		setupCmd->RecordCopyBufferToImage(copyInfo, *bufInfo.buffer, *outImage);
	}

	if(bGenerateMipmaps == false)
		change_image_transfer_dst_layout_to_shader_read(*setupCmd, *outImage);
	context.FlushSetupCommandBuffer(); // Make sure image copy is complete before generating mipmaps

	// Don't need these anymore
	buffers.clear();
	imgBuffers.clear();

	if(conversionFormat.has_value()) {
		auto &setupCmd = context.GetSetupCommandBuffer();

		// Image needs to be converted
		auto createInfo = outImage->GetCreateInfo();
		createInfo.format = *conversionFormat;
		createInfo.tiling = prosper::ImageTiling::Optimal;
		createInfo.postCreateLayout = prosper::ImageLayout::TransferDstOptimal;

		auto convertedImage = outImage->Copy(*setupCmd, createInfo);
		setupCmd->RecordImageBarrier(*outImage, prosper::ImageLayout::ShaderReadOnlyOptimal, prosper::ImageLayout::TransferSrcOptimal);
		setupCmd->RecordBlitImage({}, *outImage, *convertedImage);
		setupCmd->RecordImageBarrier(*convertedImage, prosper::ImageLayout::TransferDstOptimal, prosper::ImageLayout::ShaderReadOnlyOptimal);

		context.FlushSetupCommandBuffer();
		outImage = convertedImage;
	}

	if(bGenerateMipmaps == true) {
		auto &setupCmd = context.GetSetupCommandBuffer();
		setupCmd->RecordGenerateMipmaps(*outImage, prosper::ImageLayout::TransferDstOptimal, prosper::AccessFlags::TransferWriteBit, prosper::PipelineStageFlags::TransferBit);
		context.FlushSetupCommandBuffer();
	}
}

struct VulkanImageData {
	// Original image format
	prosper::Format format = prosper::Format::R8G8B8A8_UNorm;

	// Some formats may not be supported in optimal layout by common GPUs, in which case we need to convert it
	std::optional<prosper::Format> conversionFormat = {};

	std::array<prosper::ComponentSwizzle, 4> swizzle = {prosper::ComponentSwizzle::R, prosper::ComponentSwizzle::G, prosper::ComponentSwizzle::B, prosper::ComponentSwizzle::A};
};
#ifndef DISABLE_VTF_SUPPORT
static VulkanImageData vtf_format_to_vulkan_format(VTFImageFormat format)
{
	VulkanImageData vkImgData {};
	switch(format) {
	case IMAGE_FORMAT_DXT1:
		vkImgData.format = prosper::Format::BC1_RGBA_UNorm_Block;
		break;
	case IMAGE_FORMAT_DXT3:
		vkImgData.format = prosper::Format::BC2_UNorm_Block;
		break;
	case IMAGE_FORMAT_DXT5:
		vkImgData.format = prosper::Format::BC3_UNorm_Block;
		break;
	case IMAGE_FORMAT_RGBA8888:
		vkImgData.format = prosper::Format::R8G8B8A8_UNorm;
		break;
	case IMAGE_FORMAT_RGB888: // Needs to be converted
		vkImgData.conversionFormat = prosper::Format::R8G8B8A8_UNorm;
		vkImgData.format = prosper::Format::R8G8B8_UNorm_PoorCoverage;
		break;
	case IMAGE_FORMAT_BGRA8888:
		vkImgData.format = prosper::Format::B8G8R8A8_UNorm;
		break;
	case IMAGE_FORMAT_BGR888: // Needs to be converted
		vkImgData.conversionFormat = prosper::Format::B8G8R8A8_UNorm;
		vkImgData.format = prosper::Format::B8G8R8_UNorm_PoorCoverage;
		break;
	case IMAGE_FORMAT_UV88:
		vkImgData.format = prosper::Format::R8G8_UNorm;
		break;
	case IMAGE_FORMAT_RGBA16161616F:
		vkImgData.format = prosper::Format::R16G16B16A16_SFloat;
		break;
	case IMAGE_FORMAT_RGBA32323232F:
		vkImgData.format = prosper::Format::R32G32B32A32_SFloat;
		break;
	case IMAGE_FORMAT_ABGR8888:
		vkImgData.swizzle = {prosper::ComponentSwizzle::A, prosper::ComponentSwizzle::B, prosper::ComponentSwizzle::G, prosper::ComponentSwizzle::R};
		vkImgData.format = prosper::Format::A8B8G8R8_UNorm_Pack32;
		break;
	case IMAGE_FORMAT_BGRX8888:
		vkImgData.format = prosper::Format::B8G8R8A8_UNorm;
		break;
	default:
		vkImgData.format = prosper::Format::BC1_RGBA_UNorm_Block;
		break;
	}
	// Note: When adding a new format, make sure to also add it to TextureManager::InitializeTextureData
	return vkImgData;
}

#endif

#ifndef DISABLE_VTEX_SUPPORT
static VulkanImageData vtex_format_to_vulkan_format(source2::VTexFormat format)
{
	VulkanImageData vkImgData {};
	switch(format) {
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
		vkImgData.swizzle = {prosper::ComponentSwizzle::B, prosper::ComponentSwizzle::G, prosper::ComponentSwizzle::R, prosper::ComponentSwizzle::A};
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

#endif

void TextureManager::InitializeImage(pragma::material::TextureQueueItem &item)
{
	item.initialized = true;
	auto texture = GetQueuedTexture(item);
	texture->SetFlags(texture->GetFlags() | pragma::material::Texture::Flags::Error);
	if(item.valid == false) {
		auto texError = GetErrorTexture();
		texture->SetVkTexture(texError ? texError->GetVkTexture() : nullptr);
	}
	else {
		std::shared_ptr<prosper::IImage> image = nullptr;
		std::array<prosper::ComponentSwizzle, 4> swizzle = {prosper::ComponentSwizzle::R, prosper::ComponentSwizzle::G, prosper::ComponentSwizzle::B, prosper::ComponentSwizzle::A};
		auto *surface = dynamic_cast<pragma::material::TextureQueueItemSurface *>(&item);
		if(surface != nullptr) {
			auto &img = surface->texture;
			// In theory the input image should have a srgb flag if it's srgb, but in practice that's almost never the case,
			// so we just assume the image is srgb by default.
			// if(gli::is_srgb(img->format()))
			texture->AddFlags(pragma::material::Texture::Flags::SRGB);
			ImageFormatLoader gliLoader {};
			gliLoader.userData = static_cast<gli::texture2d *>(img.get());
			gliLoader.get_image_info = [](void *userData, const pragma::material::TextureQueueItem &item, uint32_t &outWidth, uint32_t &outHeight, prosper::Format &outFormat, bool &outCubemap, uint32_t &outLayerCount, uint32_t &outMipmapCount, std::optional<prosper::Format> &outConversionFormat) -> void {
				auto &texture = *static_cast<gli::texture2d *>(userData);
				auto extents = texture.extent();
				outWidth = extents.x;
				outHeight = extents.y;
				outFormat = static_cast<prosper::Format>(texture.format());
				outCubemap = (texture.faces() == 6);
				outLayerCount = item.cubemap ? texture.faces() : texture.layers();
				outMipmapCount = texture.levels();
			};
			gliLoader.get_image_data = [](void *userData, const pragma::material::TextureQueueItem &item, uint32_t layer, uint32_t mipmapIdx, uint32_t &outDataSize) -> const void * {
				auto &texture = *static_cast<gli::texture2d *>(userData);
				auto gliLayer = item.cubemap ? 0 : layer;
				auto gliFace = item.cubemap ? layer : 0;
				outDataSize = texture.size(mipmapIdx);
				return texture.data(gliLayer, gliFace, mipmapIdx);
			};
			initialize_image(item, *texture, gliLoader, image);
		}
		else {
			auto *png = dynamic_cast<pragma::material::TextureQueueItemPNG *>(&item);
			if(png != nullptr && png->pnginfo->GetChannelCount() == 4) {
				texture->AddFlags(pragma::material::Texture::Flags::SRGB);

				ImageFormatLoader pngLoader {};
				pngLoader.userData = static_cast<pragma::image::ImageBuffer *>(png->pnginfo.get());
				pngLoader.get_image_info
				  = [](void *userData, const pragma::material::TextureQueueItem &item, uint32_t &outWidth, uint32_t &outHeight, prosper::Format &outFormat, bool &outCubemap, uint32_t &outLayerCount, uint32_t &outMipmapCount, std::optional<prosper::Format> &outConversionFormat) -> void {
					auto &png = *static_cast<pragma::image::ImageBuffer *>(userData);
					outWidth = png.GetWidth();
					outHeight = png.GetHeight();
					outFormat = prosper::util::get_vk_format(png.GetFormat());
					outCubemap = false;
					outLayerCount = 1;
					outMipmapCount = 1;
				};
				pngLoader.get_image_data = [](void *userData, const pragma::material::TextureQueueItem &item, uint32_t layer, uint32_t mipmapIdx, uint32_t &outDataSize) -> const void * {
					auto &png = *static_cast<pragma::image::ImageBuffer *>(userData);
					outDataSize = png.GetSize();
					return png.GetData();
				};
				initialize_image(item, *texture, pngLoader, image);
			}
			else {
				auto *stbi = dynamic_cast<pragma::material::TextureQueueItemStbi *>(&item);
				if(stbi != nullptr && (stbi->imageBuffer->GetChannelCount() == 3 || stbi->imageBuffer->GetChannelCount() == 4)) {
					static constexpr auto TGA_VK_FORMAT = prosper::Format::R8G8B8A8_UNorm;
					texture->AddFlags(pragma::material::Texture::Flags::SRGB);

					auto &tgaInfo = *stbi->imageBuffer;
					auto imgBuffer = pragma::image::ImageBuffer::Create(tgaInfo.GetData(), tgaInfo.GetWidth(), tgaInfo.GetHeight(), (stbi->imageBuffer->GetChannelCount() == 3) ? pragma::image::Format::RGB8 : pragma::image::Format::RGBA8);
					imgBuffer->Convert(pragma::image::Format::RGBA8);

					ImageFormatLoader tgaLoader {};
					tgaLoader.userData = static_cast<pragma::image::ImageBuffer *>(stbi->imageBuffer.get());
					tgaLoader.get_image_info
					  = [](void *userData, const pragma::material::TextureQueueItem &item, uint32_t &outWidth, uint32_t &outHeight, prosper::Format &outFormat, bool &outCubemap, uint32_t &outLayerCount, uint32_t &outMipmapCount, std::optional<prosper::Format> &outConversionFormat) -> void {
						auto &tga = *static_cast<pragma::image::ImageBuffer *>(userData);
						outWidth = tga.GetWidth();
						outHeight = tga.GetHeight();
						outFormat = TGA_VK_FORMAT;
						outCubemap = false;
						outLayerCount = 1;
						outMipmapCount = 1;
					};

					tgaLoader.get_image_data = [imgBuffer](void *userData, const pragma::material::TextureQueueItem &item, uint32_t layer, uint32_t mipmapIdx, uint32_t &outDataSize) -> const void * {
						auto &tga = *static_cast<pragma::image::ImageBuffer *>(userData);
						outDataSize = imgBuffer->GetSize();
						return imgBuffer->GetData();
					};
					initialize_image(item, *texture, tgaLoader, image);
				}
				else {
#ifndef DISABLE_VTF_SUPPORT
					auto *vtf = dynamic_cast<pragma::material::TextureQueueItemVTF *>(&item);
					if(vtf != nullptr) {
						//if(vtfFile->GetFlag(VTFImageFlag::TEXTUREFLAGS_SRGB))
						// In theory the input image should have a srgb flag if it's srgb, but in practice that's almost never the case,
						// so we just assume the image is srgb by default.
						texture->AddFlags(pragma::material::Texture::Flags::SRGB);

						auto &vtfFile = vtf->texture;
						ImageFormatLoader vtfLoader {};
						vtfLoader.userData = static_cast<VTFLib::CVTFFile *>(vtfFile.get());
						vtfLoader.get_image_info
						  = [&swizzle](void *userData, const pragma::material::TextureQueueItem &item, uint32_t &outWidth, uint32_t &outHeight, prosper::Format &outFormat, bool &outCubemap, uint32_t &outLayerCount, uint32_t &outMipmapCount, std::optional<prosper::Format> &outConversionFormat) -> void {
							auto &vtfFile = *static_cast<VTFLib::CVTFFile *>(userData);
							auto vkFormat = vtf_format_to_vulkan_format(vtfFile.GetFormat());
							outWidth = vtfFile.GetWidth();
							outHeight = vtfFile.GetHeight();
							outFormat = vkFormat.format;
							outConversionFormat = vkFormat.conversionFormat;
							outCubemap = vtfFile.GetFaceCount() == 6;
							outLayerCount = outCubemap ? 6 : 1;
							outMipmapCount = vtfFile.GetMipmapCount();
							swizzle = vkFormat.swizzle;
							if(vtfFile.GetFlag(TEXTUREFLAGS_NOMIP))
								outMipmapCount = 1u;
						};
						vtfLoader.get_image_data = [](void *userData, const pragma::material::TextureQueueItem &item, uint32_t layer, uint32_t mipmapIdx, uint32_t &outDataSize) -> const void * {
							auto &vtfFile = *static_cast<VTFLib::CVTFFile *>(userData);
							outDataSize = VTFLib::CVTFFile::ComputeMipmapSize(vtfFile.GetWidth(), vtfFile.GetHeight(), vtfFile.GetDepth(), mipmapIdx, vtfFile.GetFormat());
							return vtfFile.GetData(0, layer, 0, mipmapIdx);
						};
						initialize_image(item, *texture, vtfLoader, image);
					}
#endif
#ifndef DISABLE_VTEX_SUPPORT
					auto *vtex = dynamic_cast<pragma::material::TextureQueueItemVTex *>(&item);
					if(vtex != nullptr) {
						texture->AddFlags(pragma::material::Texture::Flags::SRGB);

						auto &vtexFile = vtex->texture;
						ImageFormatLoader vtexLoader {};
						vtexLoader.userData = static_cast<source2::resource::Texture *>(vtexFile.get());
						vtexLoader.get_image_info
						  = [&swizzle](void *userData, const pragma::material::TextureQueueItem &item, uint32_t &outWidth, uint32_t &outHeight, prosper::Format &outFormat, bool &outCubemap, uint32_t &outLayerCount, uint32_t &outMipmapCount, std::optional<prosper::Format> &outConversionFormat) -> void {
							auto &vtexFile = *static_cast<source2::resource::Texture *>(userData);
							auto vkFormat = vtex_format_to_vulkan_format(vtexFile.GetFormat());
							outWidth = vtexFile.GetWidth();
							outHeight = vtexFile.GetHeight();
							outFormat = vkFormat.format;
							outConversionFormat = vkFormat.conversionFormat;
							outCubemap = false; // TODO
							outLayerCount = outCubemap ? 6 : 1;
							outMipmapCount = vtexFile.GetMipMapCount();
							swizzle = vkFormat.swizzle;
						};
						std::vector<uint8_t> mipmapData {};
						vtexLoader.get_image_data = [&mipmapData](void *userData, const pragma::material::TextureQueueItem &item, uint32_t layer, uint32_t mipmapIdx, uint32_t &outDataSize) -> const void * {
							auto &vtexFile = *static_cast<source2::resource::Texture *>(userData);
							mipmapData.clear();
							vtexFile.ReadTextureData(mipmapIdx, mipmapData);
							outDataSize = mipmapData.size();
							return mipmapData.data();
						};
						initialize_image(item, *texture, vtexLoader, image);
					}
#endif
					static_assert(pragma::math::to_integral(pragma::material::TextureType::Count) == 13, "Update this implementation when new texture types have been added!");
				}
			}
		}
		if(image != nullptr) {
			auto &context = *item.context.lock();
			prosper::util::TextureCreateInfo createInfo {};
			createInfo.sampler = (item.sampler != nullptr) ? item.sampler : ((image->GetMipmapCount() > 1) ? m_textureSampler : m_textureSamplerNoMipmap);
			prosper::util::ImageViewCreateInfo imgViewCreateInfo {};
			imgViewCreateInfo.swizzleRed = swizzle.at(0);
			imgViewCreateInfo.swizzleGreen = swizzle.at(1);
			imgViewCreateInfo.swizzleBlue = swizzle.at(2);
			imgViewCreateInfo.swizzleAlpha = swizzle.at(3);
			createInfo.flags |= prosper::util::TextureCreateInfo::Flags::CreateImageViewForEachLayer;
			auto vkTex = context.CreateTexture(createInfo, *image, imgViewCreateInfo);
			vkTex->SetDebugName(item.name);
			texture->SetVkTexture(vkTex);

			texture->SetFlags(texture->GetFlags() & ~pragma::material::Texture::Flags::Error);
		}
	}
}

void TextureManager::FinalizeTexture(pragma::material::TextureQueueItem &item)
{
	auto texture = GetQueuedTexture(item, true);
	if(texture->IsIndexed() == false && item.addToCache) {
		if(m_textures.size() == m_textures.capacity())
			m_textures.reserve(m_textures.size() * 1.5f + 100);
		m_textures.push_back(texture);
		texture->SetFlags(texture->GetFlags() | pragma::material::Texture::Flags::Indexed);
	}
	texture->SetFlags(texture->GetFlags() | pragma::material::Texture::Flags::Loaded);
	texture->RunOnLoadedCallbacks();
}
