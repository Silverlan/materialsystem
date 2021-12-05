/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __MSYS_TEXTURE_PROCESSOR_HPP__
#define __MSYS_TEXTURE_PROCESSOR_HPP__

#include "cmatsysdefinitions.h"
#include "texture_type.h"
#include <sharedutils/asset_loader/asset_processor.hpp>
#include <prosper_enums.hpp>
#include <cinttypes>
#include <optional>
#include <functional>
#include <memory>

namespace prosper {class IBuffer; class IPrContext; class IImage; class Texture; class ISampler;};
namespace uimg {class ImageBuffer;};
namespace msys
{
	class ITextureFormatHandler;
	class TextureLoader;
	struct TextureAsset;
	class DLLCMATSYS TextureProcessor
		: public util::IAssetProcessor
	{
	public:
		struct BufferInfo
		{
			std::shared_ptr<prosper::IBuffer> buffer = nullptr;
			uint32_t layerIndex = 0u;
			uint32_t mipmapIndex = 0u;
		};

		TextureProcessor(TextureLoader &loader,std::unique_ptr<ITextureFormatHandler> &&handler);
		virtual bool Load() override;
		virtual bool Finalize() override;
		bool InitializeProsperImage(prosper::IPrContext &context);
		bool InitializeImageBuffers(prosper::IPrContext &context);
		bool InitializeTexture(prosper::IPrContext &context);
		bool CopyBuffersToImage(prosper::IPrContext &context);
		bool ConvertImageFormat(prosper::IPrContext &context,prosper::Format targetFormat);
		bool GenerateMipmaps(prosper::IPrContext &context);

		bool PrepareImage(prosper::IPrContext &context);
		bool FinalizeImage(prosper::IPrContext &context);

		std::shared_ptr<void> userData = nullptr;
		TextureMipmapMode mipmapMode = TextureMipmapMode::LoadOrGenerate;
		std::function<void(TextureAsset*)> onLoaded = nullptr;
		std::unique_ptr<ITextureFormatHandler> handler;
		std::shared_ptr<prosper::IImage> image;
		std::shared_ptr<prosper::IImage> convertedImage;
		std::shared_ptr<prosper::Texture> texture;
		prosper::Format imageFormat = prosper::Format::Unknown;
		uint32_t mipmapCount = 1;
		std::optional<prosper::Format> targetGpuConversionFormat {};
		std::function<void(const void*,std::shared_ptr<uimg::ImageBuffer>&,uint32_t,uint32_t)> cpuImageConverter = nullptr;
		std::vector<BufferInfo> buffers {};
	private:
		bool m_generateMipmaps = false;
		std::vector<std::shared_ptr<uimg::ImageBuffer>> m_tmpImgBuffers {};
		TextureLoader &m_loader;
	};
};

#endif
