// SPDX-FileCopyrightText: (c) 2020 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "definitions.hpp"

export module pragma.cmaterialsystem:shaders.source2.decompose_metalness_reflectance;

export import pragma.prosper;

export namespace pragma::material {
	namespace source2 {
		class DLLCMATSYS ShaderDecomposeMetalnessReflectance : public prosper::ShaderBaseImageProcessing {
		  public:
			static prosper::DescriptorSetInfo DESCRIPTOR_SET_TEXTURE;

			enum class TextureBinding : uint32_t {
				MetalnessReflectanceMap = 0u,

				Count
			};

			ShaderDecomposeMetalnessReflectance(prosper::IPrContext &context, const std::string &identifier);
		  protected:
			virtual void InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) override;
			virtual void InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass, uint32_t pipelineIdx) override;
			virtual void InitializeShaderResources() override;
		};
	};
};
