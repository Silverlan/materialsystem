// SPDX-FileCopyrightText: (c) 2020 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

export module pragma.cmaterialsystem:shaders.extract_image_channel;

export import pragma.prosper;

export namespace pragma::material {
	class DLLCMATSYS ShaderExtractImageChannel : public prosper::ShaderBaseImageProcessing {
	  public:
		static prosper::DescriptorSetInfo DESCRIPTOR_SET_TEXTURE;

		enum class TextureBinding : uint32_t {
			ImageMap = 0u,

			Count
		};

		enum class Pipeline : uint8_t { RGBA8 = 0, RGBA32 };

		enum class Channel : uint32_t {
			Red = 0,
			Green,
			Blue,
			Alpha,

			Zero,
			One
		};

#pragma pack(push, 1)
		struct PushConstants {
			Channel channelDstRed;
			Channel channelDstGreen;
			Channel channelDstBlue;
			Channel channelDstAlpha;
		};
#pragma pack(pop)

		ShaderExtractImageChannel(prosper::IPrContext &context, const std::string &identifier);
		std::shared_ptr<prosper::IImage> ExtractImageChannel(prosper::IPrContext &context, prosper::Texture &texSrc, const std::array<Channel, 4> &channelValues, Pipeline pipeline);
	  protected:
		virtual void InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) override;
		virtual void InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass, uint32_t pipelineIdx) override;
		virtual void InitializeShaderResources() override;
	};
};
