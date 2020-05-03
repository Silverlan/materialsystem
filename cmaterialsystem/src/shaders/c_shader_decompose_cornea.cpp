/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "shaders/c_shader_decompose_cornea.hpp"
#include <prosper_util.hpp>

#pragma optimize("",off)
decltype(msys::ShaderDecomposeCornea::DESCRIPTOR_SET_TEXTURE) msys::ShaderDecomposeCornea::DESCRIPTOR_SET_TEXTURE = {
	{
		prosper::DescriptorSetInfo::Binding { // Iris Map
			prosper::DescriptorType::CombinedImageSampler,
			prosper::ShaderStageFlags::FragmentBit
		},
		prosper::DescriptorSetInfo::Binding { // Cornea Map
			prosper::DescriptorType::CombinedImageSampler,
			prosper::ShaderStageFlags::FragmentBit
		}
	}
};
msys::ShaderDecomposeCornea::ShaderDecomposeCornea(prosper::IPrContext &context,const std::string &identifier)
	: ShaderBaseImageProcessing{context,identifier,"util/fs_decompose_cornea.gls"}
{}

void msys::ShaderDecomposeCornea::InitializeGfxPipeline(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	ShaderGraphics::InitializeGfxPipeline(pipelineInfo,pipelineIdx);

	AddDefaultVertexAttributes(pipelineInfo);
	AddDescriptorSetGroup(pipelineInfo,DESCRIPTOR_SET_TEXTURE);
}

void msys::ShaderDecomposeCornea::InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass,uint32_t pipelineIdx)
{
	CreateCachedRenderPass<msys::ShaderDecomposeCornea>(
		std::vector<prosper::util::RenderPassCreateInfo::AttachmentInfo>{
			{prosper::Format::R8G8B8A8_UNorm}, // Albedo
			{prosper::Format::R8G8B8A8_UNorm}, // Normal
			{prosper::Format::R8G8B8A8_UNorm}, // Parallax
			{prosper::Format::R8G8B8A8_UNorm} // Noise
	},outRenderPass,pipelineIdx);
}
#pragma optimize("",on)
