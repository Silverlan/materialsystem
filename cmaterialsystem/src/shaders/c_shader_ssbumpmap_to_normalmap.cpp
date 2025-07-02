// SPDX-FileCopyrightText: (c) 2020 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#include "shaders/c_shader_ssbumpmap_to_normalmap.hpp"
#include <shader/prosper_pipeline_create_info.hpp>
#include <shader/prosper_shader_t.hpp>
#include <prosper_util.hpp>

decltype(msys::ShaderSSBumpMapToNormalMap::DESCRIPTOR_SET_TEXTURE) msys::ShaderSSBumpMapToNormalMap::DESCRIPTOR_SET_TEXTURE = {
  "TEXTURE",
  {prosper::DescriptorSetInfo::Binding {"SS_BUMP", prosper::DescriptorType::CombinedImageSampler, prosper::ShaderStageFlags::FragmentBit}},
};
msys::ShaderSSBumpMapToNormalMap::ShaderSSBumpMapToNormalMap(prosper::IPrContext &context, const std::string &identifier) : ShaderBaseImageProcessing {context, identifier, "programs/util/ssbumpmap_to_normalmap"} {}

void msys::ShaderSSBumpMapToNormalMap::InitializeShaderResources()
{
	ShaderGraphics::InitializeShaderResources();

	AddDefaultVertexAttributes();
	AddDescriptorSetGroup(DESCRIPTOR_SET_TEXTURE);
}
void msys::ShaderSSBumpMapToNormalMap::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx)
{
	ShaderGraphics::InitializeGfxPipeline(pipelineInfo, pipelineIdx);
}

void msys::ShaderSSBumpMapToNormalMap::InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass, uint32_t pipelineIdx)
{
	CreateCachedRenderPass<msys::ShaderSSBumpMapToNormalMap>(
	  std::vector<prosper::util::RenderPassCreateInfo::AttachmentInfo> {
	    {prosper::Format::R32G32B32A32_SFloat} // Normalmap
	  },
	  outRenderPass, pipelineIdx);
}
