// SPDX-FileCopyrightText: (c) 2020 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.cmaterialsystem;

import :shaders.source2.decompose_pbr;

decltype(pragma::material::source2::ShaderDecomposePBR::DESCRIPTOR_SET_TEXTURE) pragma::material::source2::ShaderDecomposePBR::DESCRIPTOR_SET_TEXTURE = {
  "TEXTURE",
  {prosper::DescriptorSetInfo::Binding {"ALBEDO", prosper::DescriptorType::CombinedImageSampler, prosper::ShaderStageFlags::FragmentBit}, prosper::DescriptorSetInfo::Binding {"NORMAL", prosper::DescriptorType::CombinedImageSampler, prosper::ShaderStageFlags::FragmentBit},
    prosper::DescriptorSetInfo::Binding {"ANISOTROPIC_GLOSSINESS", prosper::DescriptorType::CombinedImageSampler, prosper::ShaderStageFlags::FragmentBit},
    prosper::DescriptorSetInfo::Binding {"AMBIENT_OCCLUSION", prosper::DescriptorType::CombinedImageSampler, prosper::ShaderStageFlags::FragmentBit}},
};
pragma::material::source2::ShaderDecomposePBR::ShaderDecomposePBR(prosper::IPrContext &context, const std::string &identifier) : ShaderBaseImageProcessing {context, identifier, "programs/util/source2/decompose_pbr"} {}

pragma::material::source2::ShaderDecomposePBR::DecomposedImageSet pragma::material::source2::ShaderDecomposePBR::DecomposePBR(prosper::IPrContext &context, prosper::Texture &albedoMap, prosper::Texture &normalMap, prosper::Texture &aoMap, Flags flags, prosper::Texture *optAniGlossMap)
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
	ds.SetBindingTexture(albedoMap, pragma::math::to_integral(TextureBinding::AlbedoMap));
	ds.SetBindingTexture(normalMap, pragma::math::to_integral(TextureBinding::NormalMap));
	ds.SetBindingTexture(aoMap, pragma::math::to_integral(TextureBinding::AmbientOcclusionMap));

	pragma::math::set_flag(flags, Flags::SpecularWorkflow, pragma::math::is_flag_set(flags, Flags::SpecularWorkflow) && optAniGlossMap != nullptr);
	if(pragma::math::is_flag_set(flags, Flags::SpecularWorkflow))
		ds.SetBindingTexture(*optAniGlossMap, pragma::math::to_integral(TextureBinding::AnisotropicGlossinessMap));
	else // AnisoGloss map will not be used, so we'll just bind whatever
		ds.SetBindingTexture(albedoMap, pragma::math::to_integral(TextureBinding::AnisotropicGlossinessMap));

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

void pragma::material::source2::ShaderDecomposePBR::InitializeShaderResources()
{
	ShaderGraphics::InitializeShaderResources();

	AddDefaultVertexAttributes();
	AddDescriptorSetGroup(DESCRIPTOR_SET_TEXTURE);
	AttachPushConstantRange(0u, sizeof(PushConstants), prosper::ShaderStageFlags::FragmentBit);
}

void pragma::material::source2::ShaderDecomposePBR::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx)
{
	ShaderGraphics::InitializeGfxPipeline(pipelineInfo, pipelineIdx);

	SetGenericAlphaColorBlendAttachmentProperties(pipelineInfo);
}

void pragma::material::source2::ShaderDecomposePBR::InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass, uint32_t pipelineIdx)
{
	CreateCachedRenderPass<ShaderDecomposePBR>(
	  std::vector<prosper::util::RenderPassCreateInfo::AttachmentInfo> {
	    {prosper::Format::R8G8B8A8_UNorm}, // MetallicRoughness
	    {prosper::Format::R8G8B8A8_UNorm}  // Albedo
	  },
	  outRenderPass, pipelineIdx);
}
