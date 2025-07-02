// SPDX-FileCopyrightText: © 2020 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __C_SHADER_SOURCE2_GENERATE_TANGENT_SPACE_NORMAL_MAP_HPP__
#define __C_SHADER_SOURCE2_GENERATE_TANGENT_SPACE_NORMAL_MAP_HPP__

#include "cmatsysdefinitions.h"
#include <shader/prosper_shader_base_image_processing.hpp>

namespace msys {
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

#endif
