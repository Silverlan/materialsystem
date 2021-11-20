/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __MSYS_TEXTURE_PROCESSOR_HPP__
#define __MSYS_TEXTURE_PROCESSOR_HPP__

#include "cmatsysdefinitions.h"
#include "texture_type.h"
#include <prosper_enums.hpp>
#include <cinttypes>
#include <optional>
#include <functional>
#include <memory>

namespace prosper {class IBuffer; class IPrContext; class IImage;};
namespace uimg {class ImageBuffer;};
namespace msys
{
	class ITextureFormatHandler;
	struct DLLCMATSYS TextureProcessor
	{
		struct BufferInfo
		{
			std::shared_ptr<prosper::IBuffer> buffer = nullptr;
			uint32_t layerIndex = 0u;
			uint32_t mipmapIndex = 0u;
		};

		TextureProcessor(ITextureFormatHandler &handler)
			: handler{handler}
		{}
		bool InitializeProsperImage(prosper::IPrContext &context);
		bool InitializeImageBuffers(prosper::IPrContext &context);
		bool CopyBuffersToImage(prosper::IPrContext &context);
		bool ConvertImageFormat(prosper::IPrContext &context,prosper::Format targetFormat);
		bool GenerateMipmaps(prosper::IPrContext &context);

		bool PrepareImage(prosper::IPrContext &context);
		bool FinalizeImage(prosper::IPrContext &context);

		TextureMipmapMode mipmapMode = TextureMipmapMode::LoadOrGenerate;
		ITextureFormatHandler &handler;
		std::shared_ptr<prosper::IImage> image;
		prosper::Format imageFormat = prosper::Format::Unknown;
		uint32_t mipmapCount = 1;
		std::optional<prosper::Format> targetGpuConversionFormat {};
		std::function<void(const void*,std::shared_ptr<uimg::ImageBuffer>&,uint32_t,uint32_t)> cpuImageConverter = nullptr;
		std::vector<BufferInfo> buffers {};
	private:
		bool m_generateMipmaps = false;
		std::vector<std::shared_ptr<uimg::ImageBuffer>> m_tmpImgBuffers {};
	};
};

#endif
