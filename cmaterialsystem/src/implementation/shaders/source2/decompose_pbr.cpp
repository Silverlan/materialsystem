// SPDX-FileCopyrightText: (c) 2020 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <vector>
#include <string>

module pragma.cmaterialsystem;

import :shaders.source2.decompose_pbr;

decltype(msys::source2::ShaderDecomposePBR::DESCRIPTOR_SET_TEXTURE) msys::source2::ShaderDecomposePBR::DESCRIPTOR_SET_TEXTURE = {
  "TEXTURE",
  {prosper::DescriptorSetInfo::Binding {"ALBEDO", prosper::DescriptorType::CombinedImageSampler, prosper::ShaderStageFlags::FragmentBit}, prosper::DescriptorSetInfo::Binding {"NORMAL", prosper::DescriptorType::CombinedImageSampler, prosper::ShaderStageFlags::FragmentBit},
    prosper::DescriptorSetInfo::Binding {"ANISOTROPIC_GLOSSINESS", prosper::DescriptorType::CombinedImageSampler, prosper::ShaderStageFlags::FragmentBit},
    prosper::DescriptorSetInfo::Binding {"AMBIENT_OCCLUSION", prosper::DescriptorType::CombinedImageSampler, prosper::ShaderStageFlags::FragmentBit}},
};
msys::source2::ShaderDecomposePBR::ShaderDecomposePBR(prosper::IPrContext &context, const std::string &identifier) : ShaderBaseImageProcessing {context, identifier, "programs/util/source2/decompose_pbr"} {}

msys::source2::ShaderDecomposePBR::DecomposedImageSet msys::source2::ShaderDecomposePBR::DecomposePBR(prosper::IPrContext &context, prosper::Texture &albedoMap, prosper::Texture &normalMap, prosper::Texture &aoMap, Flags flags, prosper::Texture *optAniGlossMap)
{
	prosper::util::ImageCreateInfo imgCreateInfo {};
	imgCreateInfo.format = prosper::Format::R8G8B8A8_UNorm;
	imgCreateInfo.memoryFeatures = prosper::MemoryFeatureFlags::GPUBulk;
	imgCreateInfo.postCreateLayout = prosper::ImageLayout::ColorAttachmentOptimal;
	imgCreateInfo.tiling = prosper::ImageTiling::Optimal;
	imgCreateInfo.usage = prosper::ImageUsageFlags::ColorAttachmentBit | prosper::ImageUsageFlags::TransferSrcBit;

	auto &imgAlbedo = albedoMap.GetImage();
	auto &imgNormal = normalMap.GetImage();
	auto &imgAo = aoMap.GetImage();

	auto extents = imgAlbedo.GetExtents();
	imgCreateInfo.width = extents.width;
	imgCreateInfo.height = extents.height;
	auto imgAlbedoOut = context.CreateImage(imgCreateInfo);
	auto imgMetallicRoughnessOut = context.CreateImage(imgCreateInfo);

	prosper::util::ImageViewCreateInfo imgViewCreateInfo {};
	auto texAlbedoOut = context.CreateTexture({}, *imgAlbedoOut, imgViewCreateInfo);
	auto texMetallicRoughnessOut = context.CreateTexture({}, *imgMetallicRoughnessOut, imgViewCreateInfo);
	auto rt = context.CreateRenderTarget({texMetallicRoughnessOut, texAlbedoOut}, GetRenderPass());

	auto dsg = CreateDescriptorSetGroup(DESCRIPTOR_SET_TEXTURE.setIndex);
	auto &ds = *dsg->GetDescriptorSet();
	ds.SetBindingTexture(albedoMap, umath::to_integral(TextureBinding::AlbedoMap));
	ds.SetBindingTexture(normalMap, umath::to_integral(TextureBinding::NormalMap));
	ds.SetBindingTexture(aoMap, umath::to_integral(TextureBinding::AmbientOcclusionMap));

	umath::set_flag(flags, Flags::SpecularWorkflow, umath::is_flag_set(flags, Flags::SpecularWorkflow) && optAniGlossMap != nullptr);
	if(umath::is_flag_set(flags, Flags::SpecularWorkflow))
		ds.SetBindingTexture(*optAniGlossMap, umath::to_integral(TextureBinding::AnisotropicGlossinessMap));
	else // AnisoGloss map will not be used, so we'll just bind whatever
		ds.SetBindingTexture(albedoMap, umath::to_integral(TextureBinding::AnisotropicGlossinessMap));

	auto &setupCmd = context.GetSetupCommandBuffer();
	if(setupCmd->RecordBeginRenderPass(*rt)) {
		prosper::ShaderBindState bindState {*setupCmd};
		if(RecordBeginDraw(bindState)) {
			PushConstants pushConstants {};
			pushConstants.flags = flags;
			if(RecordPushConstants(bindState, pushConstants))
				RecordDraw(bindState, ds);
			RecordEndDraw(bindState);
		}
		setupCmd->RecordEndRenderPass();
	}
	context.FlushSetupCommandBuffer();

	return {texMetallicRoughnessOut->GetImage().shared_from_this(), texAlbedoOut->GetImage().shared_from_this()};
}

void msys::source2::ShaderDecomposePBR::InitializeShaderResources()
{
	ShaderGraphics::InitializeShaderResources();

	AddDefaultVertexAttributes();
	AddDescriptorSetGroup(DESCRIPTOR_SET_TEXTURE);
	AttachPushConstantRange(0u, sizeof(PushConstants), prosper::ShaderStageFlags::FragmentBit);
}

void msys::source2::ShaderDecomposePBR::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx)
{
	ShaderGraphics::InitializeGfxPipeline(pipelineInfo, pipelineIdx);

	SetGenericAlphaColorBlendAttachmentProperties(pipelineInfo);
}

void msys::source2::ShaderDecomposePBR::InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass, uint32_t pipelineIdx)
{
	CreateCachedRenderPass<msys::source2::ShaderDecomposePBR>(
	  std::vector<prosper::util::RenderPassCreateInfo::AttachmentInfo> {
	    {prosper::Format::R8G8B8A8_UNorm}, // MetallicRoughness
	    {prosper::Format::R8G8B8A8_UNorm}  // Albedo
	  },
	  outRenderPass, pipelineIdx);
}
