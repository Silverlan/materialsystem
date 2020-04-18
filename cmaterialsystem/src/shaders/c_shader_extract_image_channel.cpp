/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifdef _WIN32
#include <Windows.h>
#undef MemoryBarrier
#endif
#include "shaders/c_shader_extract_image_channel.hpp"
#include <prosper_context.hpp>
#include <prosper_util.hpp>
#include <prosper_command_buffer.hpp>
#include <image/prosper_image.hpp>
#include <image/prosper_texture.hpp>
#include <image/prosper_render_target.hpp>
#include <prosper_descriptor_set_group.hpp>

#pragma optimize("",off)
decltype(msys::ShaderExtractImageChannel::DESCRIPTOR_SET_TEXTURE) msys::ShaderExtractImageChannel::DESCRIPTOR_SET_TEXTURE = {
	{
		prosper::Shader::DescriptorSetInfo::Binding { // Image map
			Anvil::DescriptorType::COMBINED_IMAGE_SAMPLER,
			Anvil::ShaderStageFlagBits::FRAGMENT_BIT
		}
	}
};
msys::ShaderExtractImageChannel::ShaderExtractImageChannel(prosper::Context &context,const std::string &identifier)
	: ShaderBaseImageProcessing{context,identifier,"util/fs_extract_image_channel.gls"}
{
	SetPipelineCount(2);
}

void msys::ShaderExtractImageChannel::InitializeGfxPipeline(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	ShaderGraphics::InitializeGfxPipeline(pipelineInfo,pipelineIdx);

	AddDefaultVertexAttributes(pipelineInfo);
	AddDescriptorSetGroup(pipelineInfo,DESCRIPTOR_SET_TEXTURE);
	AttachPushConstantRange(pipelineInfo,0u,sizeof(PushConstants),Anvil::ShaderStageFlagBits::FRAGMENT_BIT);
}

void msys::ShaderExtractImageChannel::InitializeRenderPass(std::shared_ptr<prosper::RenderPass> &outRenderPass,uint32_t pipelineIdx)
{
	auto pipeline = static_cast<Pipeline>(pipelineIdx);
	switch(pipeline)
	{
	case Pipeline::RGBA8:
		CreateCachedRenderPass<msys::ShaderExtractImageChannel>(
			std::vector<prosper::util::RenderPassCreateInfo::AttachmentInfo>{
				{Anvil::Format::R8G8B8A8_UNORM}
		},outRenderPass,pipelineIdx);
		break;
	case Pipeline::RGBA32:
		CreateCachedRenderPass<msys::ShaderExtractImageChannel>(
			std::vector<prosper::util::RenderPassCreateInfo::AttachmentInfo>{
				{Anvil::Format::R32G32B32A32_SFLOAT}
		},outRenderPass,pipelineIdx);
		break;
	}
}

std::shared_ptr<prosper::Image> msys::ShaderExtractImageChannel::ExtractImageChannel(prosper::Context &context,prosper::Texture &texSrc,const std::array<Channel,4> &channelValues,Pipeline pipeline)
{
	Anvil::Format outputFormat;
	switch(pipeline)
	{
	case Pipeline::RGBA8:
		outputFormat = Anvil::Format::R8G8B8A8_UNORM;
		break;
	case Pipeline::RGBA32:
		outputFormat = Anvil::Format::R32G32B32A32_SFLOAT;
		break;
	}
	prosper::util::ImageCreateInfo imgCreateInfo {};
	//imgCreateInfo.flags |= prosper::util::ImageCreateInfo::Flags::FullMipmapChain;
	imgCreateInfo.format = outputFormat;
	imgCreateInfo.memoryFeatures = prosper::util::MemoryFeatureFlags::GPUBulk;
	imgCreateInfo.postCreateLayout = Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
	imgCreateInfo.tiling = Anvil::ImageTiling::OPTIMAL;
	imgCreateInfo.usage = Anvil::ImageUsageFlagBits::COLOR_ATTACHMENT_BIT | Anvil::ImageUsageFlagBits::TRANSFER_SRC_BIT;

	auto &imgSrc = texSrc.GetImage();
	auto extents = imgSrc->GetExtents();
	imgCreateInfo.width = extents.width;
	imgCreateInfo.height = extents.height;
	auto &dev = context.GetDevice();
	auto imgOutput = prosper::util::create_image(context.GetDevice(),imgCreateInfo);

	prosper::util::ImageViewCreateInfo imgViewCreateInfo {};
	auto texOutput = prosper::util::create_texture(dev,{},imgOutput,&imgViewCreateInfo);
	auto rt = prosper::util::create_render_target(dev,{texOutput},GetRenderPass(umath::to_integral(pipeline)));

	auto dsg = CreateDescriptorSetGroup(DESCRIPTOR_SET_TEXTURE.setIndex);
	auto &ds = *dsg->GetDescriptorSet();
	prosper::util::set_descriptor_set_binding_texture(ds,texSrc,umath::to_integral(TextureBinding::ImageMap));
	auto &setupCmd = context.GetSetupCommandBuffer();
	if(prosper::util::record_begin_render_pass(**setupCmd,*rt))
	{
		if(BeginDraw(setupCmd))
		{
			if(RecordPushConstants(PushConstants{
				channelValues.at(0),
				channelValues.at(1),
				channelValues.at(2),
				channelValues.at(3)
			}))
				Draw(*ds);
			EndDraw();
		}
		prosper::util::record_end_render_pass(**setupCmd);
	}
	context.FlushSetupCommandBuffer();

	return rt->GetTexture()->GetImage();
}
#pragma optimize("",on)
