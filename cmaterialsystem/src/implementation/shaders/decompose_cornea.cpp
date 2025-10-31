// SPDX-FileCopyrightText: (c) 2020 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;



module pragma.cmaterialsystem;

import :shaders.decompose_cornea;

decltype(msys::ShaderDecomposeCornea::DESCRIPTOR_SET_TEXTURE) msys::ShaderDecomposeCornea::DESCRIPTOR_SET_TEXTURE = {"TEXTURE",
  {
    prosper::DescriptorSetInfo::Binding {"IRIS", prosper::DescriptorType::CombinedImageSampler, prosper::ShaderStageFlags::FragmentBit},
    prosper::DescriptorSetInfo::Binding {"CORNEA", prosper::DescriptorType::CombinedImageSampler, prosper::ShaderStageFlags::FragmentBit},
  }};
msys::ShaderDecomposeCornea::ShaderDecomposeCornea(prosper::IPrContext &context, const std::string &identifier) : ShaderBaseImageProcessing {context, identifier, "programs/util/decompose_cornea"} {}

void msys::ShaderDecomposeCornea::InitializeShaderResources()
{
	ShaderGraphics::InitializeShaderResources();

	AddDefaultVertexAttributes();
	AddDescriptorSetGroup(DESCRIPTOR_SET_TEXTURE);
}
void msys::ShaderDecomposeCornea::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) { ShaderGraphics::InitializeGfxPipeline(pipelineInfo, pipelineIdx); }

void msys::ShaderDecomposeCornea::InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass, uint32_t pipelineIdx)
{
	CreateCachedRenderPass<msys::ShaderDecomposeCornea>(
	  std::vector<prosper::util::RenderPassCreateInfo::AttachmentInfo> {
	    {prosper::Format::R8G8B8A8_UNorm}, // Albedo
	    {prosper::Format::R8G8B8A8_UNorm}, // Normal
	    {prosper::Format::R8G8B8A8_UNorm}, // Parallax
	    {prosper::Format::R8G8B8A8_UNorm}  // Noise
	  },
	  outRenderPass, pipelineIdx);
}
