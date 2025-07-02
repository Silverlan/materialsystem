// SPDX-FileCopyrightText: (c) 2020 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#include "shaders/source2/c_shader_generate_tangent_space_normal_map.hpp"
#include <shader/prosper_shader_t.hpp>
#include <shader/prosper_pipeline_create_info.hpp>
#include <prosper_util.hpp>

decltype(msys::source2::ShaderGenerateTangentSpaceNormalMap::DESCRIPTOR_SET_TEXTURE) msys::source2::ShaderGenerateTangentSpaceNormalMap::DESCRIPTOR_SET_TEXTURE = {
  "TEXTURE",
  {prosper::DescriptorSetInfo::Binding {"NORMAL", prosper::DescriptorType::CombinedImageSampler, prosper::ShaderStageFlags::FragmentBit}},
};
msys::source2::ShaderGenerateTangentSpaceNormalMap::ShaderGenerateTangentSpaceNormalMap(prosper::IPrContext &context, const std::string &identifier) : ShaderGenerateTangentSpaceNormalMap {context, identifier, "programs/util/source2/generate_tangent_space_normal_map"} {}

msys::source2::ShaderGenerateTangentSpaceNormalMap::ShaderGenerateTangentSpaceNormalMap(prosper::IPrContext &context, const std::string &identifier, const std::string &fragmentShader) : ShaderBaseImageProcessing {context, identifier, fragmentShader} {}

void msys::source2::ShaderGenerateTangentSpaceNormalMap::InitializeShaderResources()
{
	ShaderGraphics::InitializeShaderResources();

	AddDefaultVertexAttributes();
	AddDescriptorSetGroup(DESCRIPTOR_SET_TEXTURE);
}
void msys::source2::ShaderGenerateTangentSpaceNormalMap::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) { ShaderGraphics::InitializeGfxPipeline(pipelineInfo, pipelineIdx); }

void msys::source2::ShaderGenerateTangentSpaceNormalMap::InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass, uint32_t pipelineIdx)
{
	CreateCachedRenderPass<msys::source2::ShaderGenerateTangentSpaceNormalMap>(
	  std::vector<prosper::util::RenderPassCreateInfo::AttachmentInfo> {
	    {prosper::Format::R32G32B32A32_SFloat} // Normal
	  },
	  outRenderPass, pipelineIdx);
}

////////////////////

msys::source2::ShaderGenerateTangentSpaceNormalMapProto::ShaderGenerateTangentSpaceNormalMapProto(prosper::IPrContext &context, const std::string &identifier) : ShaderGenerateTangentSpaceNormalMap {context, identifier, "programs/util/source2/generate_tangent_space_normal_map_proto"} {}
