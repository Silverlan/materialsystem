/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "shaders/source2/c_shader_generate_tangent_space_normal_map.hpp"
#include <prosper_util.hpp>

#pragma optimize("",off)
decltype(msys::source2::ShaderGenerateTangentSpaceNormalMap::DESCRIPTOR_SET_TEXTURE) msys::source2::ShaderGenerateTangentSpaceNormalMap::DESCRIPTOR_SET_TEXTURE = {
	{
		prosper::Shader::DescriptorSetInfo::Binding { // Normal Map
			Anvil::DescriptorType::COMBINED_IMAGE_SAMPLER,
			Anvil::ShaderStageFlagBits::FRAGMENT_BIT
		}
	}
};
msys::source2::ShaderGenerateTangentSpaceNormalMap::ShaderGenerateTangentSpaceNormalMap(prosper::Context &context,const std::string &identifier)
	: ShaderGenerateTangentSpaceNormalMap{context,identifier,"util/source2/fs_generate_tangent_space_normal_map.gls"}
{}

msys::source2::ShaderGenerateTangentSpaceNormalMap::ShaderGenerateTangentSpaceNormalMap(prosper::Context &context,const std::string &identifier,const std::string &fragmentShader)
	: ShaderBaseImageProcessing{context,identifier,fragmentShader}
{}

void msys::source2::ShaderGenerateTangentSpaceNormalMap::InitializeGfxPipeline(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	ShaderGraphics::InitializeGfxPipeline(pipelineInfo,pipelineIdx);

	AddDefaultVertexAttributes(pipelineInfo);
	AddDescriptorSetGroup(pipelineInfo,DESCRIPTOR_SET_TEXTURE);
}

void msys::source2::ShaderGenerateTangentSpaceNormalMap::InitializeRenderPass(std::shared_ptr<prosper::RenderPass> &outRenderPass,uint32_t pipelineIdx)
{
	CreateCachedRenderPass<msys::source2::ShaderGenerateTangentSpaceNormalMap>(
		std::vector<prosper::util::RenderPassCreateInfo::AttachmentInfo>{
			{Anvil::Format::R32G32B32A32_SFLOAT} // Normal
	},outRenderPass,pipelineIdx);
}

////////////////////

msys::source2::ShaderGenerateTangentSpaceNormalMapProto::ShaderGenerateTangentSpaceNormalMapProto(prosper::Context &context,const std::string &identifier)
	: ShaderGenerateTangentSpaceNormalMap{context,identifier,"util/source2/fs_generate_tangent_space_normal_map_proto.gls"}
{}
#pragma optimize("",on)
