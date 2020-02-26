/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "shaders/c_shader_ssbumpmap_to_normalmap.hpp"
#include <prosper_util.hpp>

#pragma optimize("",off)
decltype(msys::ShaderSSBumpMapToNormalMap::DESCRIPTOR_SET_TEXTURE) msys::ShaderSSBumpMapToNormalMap::DESCRIPTOR_SET_TEXTURE = {
	{
		prosper::Shader::DescriptorSetInfo::Binding { // SSBumpMap
			Anvil::DescriptorType::COMBINED_IMAGE_SAMPLER,
			Anvil::ShaderStageFlagBits::FRAGMENT_BIT
		}
	}
};
msys::ShaderSSBumpMapToNormalMap::ShaderSSBumpMapToNormalMap(prosper::Context &context,const std::string &identifier)
	: ShaderBaseImageProcessing{context,identifier,"util/fs_ssbumpmap_to_normalmap.gls"}
{}

void msys::ShaderSSBumpMapToNormalMap::InitializeGfxPipeline(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	ShaderGraphics::InitializeGfxPipeline(pipelineInfo,pipelineIdx);

	AddDefaultVertexAttributes(pipelineInfo);
	AddDescriptorSetGroup(pipelineInfo,DESCRIPTOR_SET_TEXTURE);
}

void msys::ShaderSSBumpMapToNormalMap::InitializeRenderPass(std::shared_ptr<prosper::RenderPass> &outRenderPass,uint32_t pipelineIdx)
{
	CreateCachedRenderPass<msys::ShaderSSBumpMapToNormalMap>(
		std::vector<prosper::util::RenderPassCreateInfo::AttachmentInfo>{
			{Anvil::Format::R32G32B32A32_SFLOAT} // Normalmap
	},outRenderPass,pipelineIdx);
}
#pragma optimize("",on)
