/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "shaders/source2/c_shader_decompose_metalness_reflectance.hpp"
#include <prosper_util.hpp>

#pragma optimize("",off)
decltype(msys::source2::ShaderDecomposeMetalnessReflectance::DESCRIPTOR_SET_TEXTURE) msys::source2::ShaderDecomposeMetalnessReflectance::DESCRIPTOR_SET_TEXTURE = {
	{
		prosper::Shader::DescriptorSetInfo::Binding { // Metalness-reflectance Map
	Anvil::DescriptorType::COMBINED_IMAGE_SAMPLER,
	Anvil::ShaderStageFlagBits::FRAGMENT_BIT
}
	}
};
msys::source2::ShaderDecomposeMetalnessReflectance::ShaderDecomposeMetalnessReflectance(prosper::Context &context,const std::string &identifier)
	: ShaderBaseImageProcessing{context,identifier,"util/source2/fs_decompose_metalness_reflectance.gls"}
{}

void msys::source2::ShaderDecomposeMetalnessReflectance::InitializeGfxPipeline(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	ShaderGraphics::InitializeGfxPipeline(pipelineInfo,pipelineIdx);

	AddDefaultVertexAttributes(pipelineInfo);
	AddDescriptorSetGroup(pipelineInfo,DESCRIPTOR_SET_TEXTURE);
}

void msys::source2::ShaderDecomposeMetalnessReflectance::InitializeRenderPass(std::shared_ptr<prosper::RenderPass> &outRenderPass,uint32_t pipelineIdx)
{
	CreateCachedRenderPass<msys::source2::ShaderDecomposeMetalnessReflectance>(
		std::vector<prosper::util::RenderPassCreateInfo::AttachmentInfo>{
			{Anvil::Format::R32G32B32A32_SFLOAT}, // Metalness
			{Anvil::Format::R32G32B32A32_SFLOAT} // Reflectance
	},outRenderPass,pipelineIdx);
}
#pragma optimize("",on)
