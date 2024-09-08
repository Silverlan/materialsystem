/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "shaders/source2/c_shader_decompose_metalness_reflectance.hpp"
#include <shader/prosper_shader_t.hpp>
#include <shader/prosper_pipeline_create_info.hpp>
#include <prosper_util.hpp>

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
