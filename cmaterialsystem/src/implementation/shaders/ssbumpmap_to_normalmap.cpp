// SPDX-FileCopyrightText: (c) 2020 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module pragma.cmaterialsystem;

import :shaders.ssbumpmap_to_normalmap;

decltype(pragma::material::ShaderSSBumpMapToNormalMap::DESCRIPTOR_SET_TEXTURE) pragma::material::ShaderSSBumpMapToNormalMap::DESCRIPTOR_SET_TEXTURE = {
  "TEXTURE",
  {prosper::DescriptorSetInfo::Binding {"SS_BUMP", prosper::DescriptorType::CombinedImageSampler, prosper::ShaderStageFlags::FragmentBit}},
};
pragma::material::ShaderSSBumpMapToNormalMap::ShaderSSBumpMapToNormalMap(prosper::IPrContext &context, const std::string &identifier) : ShaderBaseImageProcessing {context, identifier, "programs/util/ssbumpmap_to_normalmap"} {}

void pragma::material::ShaderSSBumpMapToNormalMap::InitializeShaderResources()
{
	ShaderGraphics::InitializeShaderResources();

	AddDefaultVertexAttributes();
	AddDescriptorSetGroup(DESCRIPTOR_SET_TEXTURE);
}
void pragma::material::ShaderSSBumpMapToNormalMap::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) { ShaderGraphics::InitializeGfxPipeline(pipelineInfo, pipelineIdx); }

void pragma::material::ShaderSSBumpMapToNormalMap::InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass, uint32_t pipelineIdx)
{
	CreateCachedRenderPass<ShaderSSBumpMapToNormalMap>(
	  std::vector<prosper::util::RenderPassCreateInfo::AttachmentInfo> {
	    {prosper::Format::R32G32B32A32_SFloat} // Normalmap
	  },
	  outRenderPass, pipelineIdx);
}
