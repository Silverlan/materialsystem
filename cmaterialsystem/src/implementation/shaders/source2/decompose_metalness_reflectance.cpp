// SPDX-FileCopyrightText: (c) 2020 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <cinttypes>
#include <memory>

#include <vector>
#include <string>

module pragma.cmaterialsystem;

import :shaders.source2.decompose_metalness_reflectance;

decltype(msys::source2::ShaderDecomposeMetalnessReflectance::DESCRIPTOR_SET_TEXTURE) msys::source2::ShaderDecomposeMetalnessReflectance::DESCRIPTOR_SET_TEXTURE = {
  "TEXTURE",
  {prosper::DescriptorSetInfo::Binding {"metalness_reflectance", prosper::DescriptorType::CombinedImageSampler, prosper::ShaderStageFlags::FragmentBit}},
};
msys::source2::ShaderDecomposeMetalnessReflectance::ShaderDecomposeMetalnessReflectance(prosper::IPrContext &context, const std::string &identifier) : ShaderBaseImageProcessing {context, identifier, "programs/util/source2/decompose_metalness_reflectance"} {}

void msys::source2::ShaderDecomposeMetalnessReflectance::InitializeShaderResources()
{
	ShaderGraphics::InitializeShaderResources();

	AddDefaultVertexAttributes();
	AddDescriptorSetGroup(DESCRIPTOR_SET_TEXTURE);
}

void msys::source2::ShaderDecomposeMetalnessReflectance::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) { ShaderGraphics::InitializeGfxPipeline(pipelineInfo, pipelineIdx); }

void msys::source2::ShaderDecomposeMetalnessReflectance::InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass, uint32_t pipelineIdx)
{
	CreateCachedRenderPass<msys::source2::ShaderDecomposeMetalnessReflectance>(
	  std::vector<prosper::util::RenderPassCreateInfo::AttachmentInfo> {
	    {prosper::Format::R8G8B8A8_UNorm} // RMA
	  },
	  outRenderPass, pipelineIdx);
}
