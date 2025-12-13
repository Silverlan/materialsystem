// SPDX-FileCopyrightText: (c) 2020 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.cmaterialsystem;

import :shaders.extract_image_channel;

decltype(pragma::material::ShaderExtractImageChannel::DESCRIPTOR_SET_TEXTURE) pragma::material::ShaderExtractImageChannel::DESCRIPTOR_SET_TEXTURE = {
  "TEXTURE",
  {prosper::DescriptorSetInfo::Binding {"IMAGE", prosper::DescriptorType::CombinedImageSampler, prosper::ShaderStageFlags::FragmentBit}},
};
pragma::material::ShaderExtractImageChannel::ShaderExtractImageChannel(prosper::IPrContext &context, const std::string &identifier) : ShaderBaseImageProcessing {context, identifier, "programs/util/extract_image_channel"} { SetPipelineCount(2); }

void pragma::material::ShaderExtractImageChannel::InitializeShaderResources()
{
	ShaderGraphics::InitializeShaderResources();

	AddDefaultVertexAttributes();
	AddDescriptorSetGroup(DESCRIPTOR_SET_TEXTURE);
	AttachPushConstantRange(0u, sizeof(PushConstants), prosper::ShaderStageFlags::FragmentBit);
}
void pragma::material::ShaderExtractImageChannel::InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) { ShaderGraphics::InitializeGfxPipeline(pipelineInfo, pipelineIdx); }

void pragma::material::ShaderExtractImageChannel::InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass, uint32_t pipelineIdx)
{
	auto pipeline = static_cast<Pipeline>(pipelineIdx);
	switch(pipeline) {
	case Pipeline::RGBA8:
		CreateCachedRenderPass<ShaderExtractImageChannel>(std::vector<prosper::util::RenderPassCreateInfo::AttachmentInfo> {{prosper::Format::R8G8B8A8_UNorm}}, outRenderPass, pipelineIdx);
		break;
	case Pipeline::RGBA32:
		CreateCachedRenderPass<ShaderExtractImageChannel>(std::vector<prosper::util::RenderPassCreateInfo::AttachmentInfo> {{prosper::Format::R32G32B32A32_SFloat}}, outRenderPass, pipelineIdx);
		break;
	}
}

std::shared_ptr<prosper::IImage> pragma::material::ShaderExtractImageChannel::ExtractImageChannel(prosper::IPrContext &context, prosper::Texture &texSrc, const std::array<Channel, 4> &channelValues, Pipeline pipeline)
{
	prosper::Format outputFormat;
	switch(pipeline) {
	case Pipeline::RGBA8:
		outputFormat = prosper::Format::R8G8B8A8_UNorm;
		break;
	case Pipeline::RGBA32:
		outputFormat = prosper::Format::R32G32B32A32_SFloat;
		break;
	}
	prosper::util::ImageCreateInfo imgCreateInfo {};
	//imgCreateInfo.flags |= prosper::util::ImageCreateInfo::Flags::FullMipmapChain;
	imgCreateInfo.format = outputFormat;
	imgCreateInfo.memoryFeatures = prosper::MemoryFeatureFlags::GPUBulk;
	imgCreateInfo.postCreateLayout = prosper::ImageLayout::ColorAttachmentOptimal;
	imgCreateInfo.tiling = prosper::ImageTiling::Optimal;
	imgCreateInfo.usage = prosper::ImageUsageFlags::ColorAttachmentBit | prosper::ImageUsageFlags::TransferSrcBit;

	auto &imgSrc = texSrc.GetImage();
	auto extents = imgSrc.GetExtents();
	imgCreateInfo.width = extents.width;
	imgCreateInfo.height = extents.height;
	auto imgOutput = context.CreateImage(imgCreateInfo);

	prosper::util::ImageViewCreateInfo imgViewCreateInfo {};
	auto texOutput = context.CreateTexture({}, *imgOutput, imgViewCreateInfo);
	auto rt = context.CreateRenderTarget({texOutput}, GetRenderPass(pragma::math::to_integral(pipeline)));

	auto dsg = CreateDescriptorSetGroup(DESCRIPTOR_SET_TEXTURE.setIndex);
	auto &ds = *dsg->GetDescriptorSet();
	ds.SetBindingTexture(texSrc, pragma::math::to_integral(TextureBinding::ImageMap));
	auto &setupCmd = context.GetSetupCommandBuffer();
	if(setupCmd->RecordBeginRenderPass(*rt)) {
		prosper::ShaderBindState bindState {*setupCmd};
		if(RecordBeginDraw(bindState)) {
			if(RecordPushConstants(bindState, PushConstants {channelValues.at(0), channelValues.at(1), channelValues.at(2), channelValues.at(3)}))
				RecordDraw(bindState, ds);
			RecordEndDraw(bindState);
		}
		setupCmd->RecordEndRenderPass();
	}
	context.FlushSetupCommandBuffer();

	return rt->GetTexture().GetImage().shared_from_this();
}
