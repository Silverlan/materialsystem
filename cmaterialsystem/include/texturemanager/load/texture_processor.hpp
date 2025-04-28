/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __MSYS_TEXTURE_PROCESSOR_HPP__
#define __MSYS_TEXTURE_PROCESSOR_HPP__

#include "cmatsysdefinitions.h"
#include "texture_type.h"
#include "texturemanager/load/texture_format_handler.hpp"
#include <sharedutils/asset_loader/file_asset_processor.hpp>
#include <prosper_enums.hpp>
#include <cinttypes>
#include <optional>
#include <functional>
#include <memory>

namespace prosper {
	class IBuffer;
	class IPrContext;
	class IImage;
	class Texture;
	class ISampler;
};
namespace uimg {
	class ImageBuffer;
};
namespace util {
	struct Asset;
	class AssetFormatLoader;
	class IAssetFormatHandler;
};
namespace udm {
	struct Property;
};
namespace msys {
	class TextureLoader;
	struct TextureAsset;

	class DLLCMATSYS TextureProcessor : public util::FileAssetProcessor {
	  public:
		struct BufferInfo {
			std::shared_ptr<prosper::IBuffer> buffer = nullptr;
			uint32_t layerIndex = 0u;
			uint32_t mipmapIndex = 0u;
		};

		TextureProcessor(util::AssetFormatLoader &loader, std::unique_ptr<util::IAssetFormatHandler> &&handler);
		virtual bool Load() override;
		virtual bool Finalize() override;
		bool InitializeProsperImage(prosper::IPrContext &context);
		bool InitializeImageBuffers(prosper::IPrContext &context);
		bool InitializeTexture(prosper::IPrContext &context);
		bool CopyBuffersToImage(prosper::IPrContext &context);
		bool ConvertImageFormat(prosper::IPrContext &context, prosper::Format targetFormat);
		bool GenerateMipmaps(prosper::IPrContext &context);
		void SetTextureData(const std::shared_ptr<udm::Property> &textureData);

		bool PrepareImage(prosper::IPrContext &context);
		bool FinalizeImage(prosper::IPrContext &context);

		TextureMipmapMode mipmapMode = TextureMipmapMode::LoadOrGenerate;
		std::shared_ptr<prosper::IImage> image;
		std::shared_ptr<prosper::IImage> convertedImage;
		std::shared_ptr<prosper::Texture> texture;
		prosper::Format imageFormat = prosper::Format::Unknown;
		uint32_t mipmapCount = 1;
		std::optional<prosper::Format> targetGpuConversionFormat {};
		std::function<void(const void *, std::shared_ptr<uimg::ImageBuffer> &, uint32_t, uint32_t)> cpuImageConverter = nullptr;
		std::vector<BufferInfo> buffers {};
	  private:
		TextureLoader &GetLoader();
		ITextureFormatHandler &GetHandler();

		bool m_generateMipmaps = false;
		std::vector<std::shared_ptr<uimg::ImageBuffer>> m_tmpImgBuffers {};
	};
};

#endif
