/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "shaders/c_shader_ssbumpmap_to_normalmap.hpp"
#include <shader/prosper_pipeline_create_info.hpp>
#include <shader/prosper_shader_t.hpp>
#include <prosper_util.hpp>

decltype(msys::ShaderSSBumpMapToNormalMap::DESCRIPTOR_SET_TEXTURE) msys::ShaderSSBumpMapToNormalMap::DESCRIPTOR_SET_TEXTURE = {{prosper::DescriptorSetInfo::Binding {// SSBumpMap
  prosper::DescriptorType::CombinedImageSampler, prosper::ShaderStageFlags::FragmentBit}}};
msys::ShaderSSBumpMapToNormalMap::ShaderSSBumpMapToNormalMap(prosper::IPrContext &context, const std::string &identifier) : ShaderBaseImageProcessing {context, identifier, "util/fs_ssbumpmap_to_normalmap.gls"} {}

void msys::ShaderSSBumpMapToNormalMap::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx)
{
	ShaderGraphics::InitializeGfxPipeline(pipelineInfo, pipelineIdx);

	AddDefaultVertexAttributes(pipelineInfo);
	AddDescriptorSetGroup(pipelineInfo, pipelineIdx, DESCRIPTOR_SET_TEXTURE);
}

void msys::ShaderSSBumpMapToNormalMap::InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass, uint32_t pipelineIdx)
{
	CreateCachedRenderPass<msys::ShaderSSBumpMapToNormalMap>(
	  std::vector<prosper::util::RenderPassCreateInfo::AttachmentInfo> {
	    {prosper::Format::R32G32B32A32_SFloat} // Normalmap
	  },
	  outRenderPass, pipelineIdx);
}
