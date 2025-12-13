// SPDX-FileCopyrightText: (c) 2021 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "definitions.hpp"

export module pragma.cmaterialsystem:texture_manager.texture_processor;

export import pragma.image;
export import pragma.materialsystem;
export import pragma.prosper;

export namespace pragma::material {
	class TextureLoader;
	class ITextureFormatHandler;
	class DLLCMATSYS TextureProcessor : public pragma::util::FileAssetProcessor {
	  public:
		struct BufferInfo {
			std::shared_ptr<prosper::IBuffer> buffer = nullptr;
			uint32_t layerIndex = 0u;
			uint32_t mipmapIndex = 0u;
		};

		TextureProcessor(pragma::util::AssetFormatLoader &loader, std::unique_ptr<pragma::util::IAssetFormatHandler> &&handler);
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
		std::function<void(const void *, std::shared_ptr<image::ImageBuffer> &, uint32_t, uint32_t)> cpuImageConverter = nullptr;
		std::vector<BufferInfo> buffers {};
	  private:
		TextureLoader &GetLoader();
		ITextureFormatHandler &GetHandler();

		bool m_generateMipmaps = false;
		std::vector<std::shared_ptr<image::ImageBuffer>> m_tmpImgBuffers {};
	};
};
