/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __C_SHADER_DECOMPOSE_PBR_HPP__
#define __C_SHADER_DECOMPOSE_PBR_HPP__

#include "cmatsysdefinitions.h"
#include <shader/prosper_shader_base_image_processing.hpp>

namespace prosper
{
	class Image;
	class Texture;
};
namespace msys
{
	namespace source2
	{
		class DLLCMATSYS ShaderDecomposePBR
			: public prosper::ShaderBaseImageProcessing
		{
		public:
			static prosper::Shader::DescriptorSetInfo DESCRIPTOR_SET_TEXTURE;

			enum class TextureBinding : uint32_t
			{
				AlbedoMap = 0u,
				NormalMap,

				Count
			};

			struct DLLCMATSYS DecomposedImageSet
			{
				std::shared_ptr<prosper::Image> metallicRoughnessMap = nullptr;
				std::shared_ptr<prosper::Image> albedoMap = nullptr;
			};

			ShaderDecomposePBR(prosper::Context &context,const std::string &identifier);
			DecomposedImageSet DecomposePBR(prosper::Context &context,prosper::Texture &albedoMap,prosper::Texture &normalMap);
		protected:
			virtual void InitializeGfxPipeline(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx) override;
			virtual void InitializeRenderPass(std::shared_ptr<prosper::RenderPass> &outRenderPass,uint32_t pipelineIdx) override;
		};
	};
};

#endif
