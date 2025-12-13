// SPDX-FileCopyrightText: (c) 2020 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.cmaterialsystem;

import :shaders.source2.generate_tangent_space_normal_map;

decltype(pragma::material::source2::ShaderGenerateTangentSpaceNormalMap::DESCRIPTOR_SET_TEXTURE) pragma::material::source2::ShaderGenerateTangentSpaceNormalMap::DESCRIPTOR_SET_TEXTURE = {
  "TEXTURE",
  {prosper::DescriptorSetInfo::Binding {"NORMAL", prosper::DescriptorType::CombinedImageSampler, prosper::ShaderStageFlags::FragmentBit}},
};
pragma::material::source2::ShaderGenerateTangentSpaceNormalMap::ShaderGenerateTangentSpaceNormalMap(prosper::IPrContext &context, const std::string &identifier) : ShaderGenerateTangentSpaceNormalMap {context, identifier, "programs/util/source2/generate_tangent_space_normal_map"} {}

pragma::material::source2::ShaderGenerateTangentSpaceNormalMap::ShaderGenerateTangentSpaceNormalMap(prosper::IPrContext &context, const std::string &identifier, const std::string &fragmentShader) : ShaderBaseImageProcessing {context, identifier, fragmentShader} {}

void pragma::material::source2::ShaderGenerateTangentSpaceNormalMap::InitializeShaderResources()
{
	ShaderGraphics::InitializeShaderResources();

	AddDefaultVertexAttributes();
	AddDescriptorSetGroup(DESCRIPTOR_SET_TEXTURE);
}
void pragma::material::source2::ShaderGenerateTangentSpaceNormalMap::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) { ShaderGraphics::InitializeGfxPipeline(pipelineInfo, pipelineIdx); }

void pragma::material::source2::ShaderGenerateTangentSpaceNormalMap::InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass, uint32_t pipelineIdx)
{
	CreateCachedRenderPass<ShaderGenerateTangentSpaceNormalMap>(
	  std::vector<prosper::util::RenderPassCreateInfo::AttachmentInfo> {
	    {prosper::Format::R32G32B32A32_SFloat} // Normal
	  },
	  outRenderPass, pipelineIdx);
}

////////////////////

pragma::material::source2::ShaderGenerateTangentSpaceNormalMapProto::ShaderGenerateTangentSpaceNormalMapProto(prosper::IPrContext &context, const std::string &identifier) : ShaderGenerateTangentSpaceNormalMap {context, identifier, "programs/util/source2/generate_tangent_space_normal_map_proto"} {}
