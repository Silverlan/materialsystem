// SPDX-FileCopyrightText: (c) 2020 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "cmatsysdefinitions.hpp"
#include <string>
#include <memory>

export module pragma.cmaterialsystem:shaders.source2.generate_tangent_space_normal_map;

export import pragma.prosper;

export namespace msys {
	namespace source2 {
		class DLLCMATSYS ShaderGenerateTangentSpaceNormalMap : public prosper::ShaderBaseImageProcessing {
		  public:
			static prosper::DescriptorSetInfo DESCRIPTOR_SET_TEXTURE;

			enum class TextureBinding : uint32_t {
				NormalMap = 0u,

				Count
			};

			ShaderGenerateTangentSpaceNormalMap(prosper::IPrContext &context, const std::string &identifier);
		  protected:
			ShaderGenerateTangentSpaceNormalMap(prosper::IPrContext &context, const std::string &identifier, const std::string &fragmentShader);
			virtual void InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) override;
			virtual void InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass, uint32_t pipelineIdx) override;
			virtual void InitializeShaderResources() override;
		};

		// SteamVR and Dota2
		class DLLCMATSYS ShaderGenerateTangentSpaceNormalMapProto : public ShaderGenerateTangentSpaceNormalMap {
		  public:
			ShaderGenerateTangentSpaceNormalMapProto(prosper::IPrContext &context, const std::string &identifier);
		};
	};
};
