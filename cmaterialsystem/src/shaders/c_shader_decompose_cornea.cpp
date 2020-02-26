/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "shaders/c_shader_decompose_cornea.hpp"
#include <prosper_util.hpp>

#pragma optimize("",off)
decltype(msys::ShaderDecomposeCornea::DESCRIPTOR_SET_TEXTURE) msys::ShaderDecomposeCornea::DESCRIPTOR_SET_TEXTURE = {
	{
		prosper::Shader::DescriptorSetInfo::Binding { // Iris Map
			Anvil::DescriptorType::COMBINED_IMAGE_SAMPLER,
			Anvil::ShaderStageFlagBits::FRAGMENT_BIT
		},
		prosper::Shader::DescriptorSetInfo::Binding { // Cornea Map
			Anvil::DescriptorType::COMBINED_IMAGE_SAMPLER,
			Anvil::ShaderStageFlagBits::FRAGMENT_BIT
		}
	}
};
msys::ShaderDecomposeCornea::ShaderDecomposeCornea(prosper::Context &context,const std::string &identifier)
	: ShaderBaseImageProcessing{context,identifier,"util/fs_decompose_cornea.gls"}
{}

void msys::ShaderDecomposeCornea::InitializeGfxPipeline(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	ShaderGraphics::InitializeGfxPipeline(pipelineInfo,pipelineIdx);

	AddDefaultVertexAttributes(pipelineInfo);
	AddDescriptorSetGroup(pipelineInfo,DESCRIPTOR_SET_TEXTURE);
}

void msys::ShaderDecomposeCornea::InitializeRenderPass(std::shared_ptr<prosper::RenderPass> &outRenderPass,uint32_t pipelineIdx)
{
	CreateCachedRenderPass<msys::ShaderDecomposeCornea>(
		std::vector<prosper::util::RenderPassCreateInfo::AttachmentInfo>{
			{Anvil::Format::R8G8B8A8_UNORM}, // Albedo
			{Anvil::Format::R8G8B8A8_UNORM}, // Normal
			{Anvil::Format::R8G8B8A8_UNORM}, // Parallax
			{Anvil::Format::R8G8B8A8_UNORM} // Noise
	},outRenderPass,pipelineIdx);
}
#pragma optimize("",on)
