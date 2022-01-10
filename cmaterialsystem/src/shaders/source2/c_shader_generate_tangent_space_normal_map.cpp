/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "shaders/source2/c_shader_generate_tangent_space_normal_map.hpp"
#include <shader/prosper_pipeline_create_info.hpp>
#include <prosper_util.hpp>

decltype(msys::source2::ShaderGenerateTangentSpaceNormalMap::DESCRIPTOR_SET_TEXTURE) msys::source2::ShaderGenerateTangentSpaceNormalMap::DESCRIPTOR_SET_TEXTURE = {
	{
		prosper::DescriptorSetInfo::Binding { // Normal Map
			prosper::DescriptorType::CombinedImageSampler,
			prosper::ShaderStageFlags::FragmentBit
		}
	}
};
msys::source2::ShaderGenerateTangentSpaceNormalMap::ShaderGenerateTangentSpaceNormalMap(prosper::IPrContext &context,const std::string &identifier)
	: ShaderGenerateTangentSpaceNormalMap{context,identifier,"util/source2/fs_generate_tangent_space_normal_map.gls"}
{}

msys::source2::ShaderGenerateTangentSpaceNormalMap::ShaderGenerateTangentSpaceNormalMap(prosper::IPrContext &context,const std::string &identifier,const std::string &fragmentShader)
	: ShaderBaseImageProcessing{context,identifier,fragmentShader}
{}

void msys::source2::ShaderGenerateTangentSpaceNormalMap::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	ShaderGraphics::InitializeGfxPipeline(pipelineInfo,pipelineIdx);

	AddDefaultVertexAttributes(pipelineInfo);
	AddDescriptorSetGroup(pipelineInfo,pipelineIdx,DESCRIPTOR_SET_TEXTURE);
}

void msys::source2::ShaderGenerateTangentSpaceNormalMap::InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass,uint32_t pipelineIdx)
{
	CreateCachedRenderPass<msys::source2::ShaderGenerateTangentSpaceNormalMap>(
		std::vector<prosper::util::RenderPassCreateInfo::AttachmentInfo>{
			{prosper::Format::R32G32B32A32_SFloat} // Normal
	},outRenderPass,pipelineIdx);
}

////////////////////

msys::source2::ShaderGenerateTangentSpaceNormalMapProto::ShaderGenerateTangentSpaceNormalMapProto(prosper::IPrContext &context,const std::string &identifier)
	: ShaderGenerateTangentSpaceNormalMap{context,identifier,"util/source2/fs_generate_tangent_space_normal_map_proto.gls"}
{}
