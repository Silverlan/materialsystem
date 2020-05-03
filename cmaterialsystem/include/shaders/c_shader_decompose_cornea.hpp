/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __C_SHADER_DECOMPOSE_CORNEA_HPP__
#define __C_SHADER_DECOMPOSE_CORNEA_HPP__

#include "cmatsysdefinitions.h"
#include <shader/prosper_shader_base_image_processing.hpp>

namespace msys
{
	class DLLCMATSYS ShaderDecomposeCornea
		: public prosper::ShaderBaseImageProcessing
	{
	public:
		static prosper::DescriptorSetInfo DESCRIPTOR_SET_TEXTURE;

		enum class TextureBinding : uint32_t
		{
			IrisMap = 0u,
			CorneaMap,

			Count
		};

		ShaderDecomposeCornea(prosper::IPrContext &context,const std::string &identifier);
	protected:
		virtual void InitializeGfxPipeline(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx) override;
		virtual void InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass,uint32_t pipelineIdx) override;
	};
};

#endif
