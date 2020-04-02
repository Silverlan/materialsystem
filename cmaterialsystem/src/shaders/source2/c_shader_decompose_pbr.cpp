/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifdef _WIN32
#include <Windows.h>
#undef MemoryBarrier
#endif
#include "shaders/source2/c_shader_decompose_pbr.hpp"
#include <prosper_util.hpp>
#include <prosper_context.hpp>
#include <prosper_descriptor_set_group.hpp>
#include <prosper_command_buffer.hpp>
#include <image/prosper_render_target.hpp>
#include <image/prosper_texture.hpp>

#pragma optimize("",off)
decltype(msys::source2::ShaderDecomposePBR::DESCRIPTOR_SET_TEXTURE) msys::source2::ShaderDecomposePBR::DESCRIPTOR_SET_TEXTURE = {
	{
		prosper::Shader::DescriptorSetInfo::Binding { // Albedo Map
			Anvil::DescriptorType::COMBINED_IMAGE_SAMPLER,
			Anvil::ShaderStageFlagBits::FRAGMENT_BIT
		},
		prosper::Shader::DescriptorSetInfo::Binding { // Normal Map
			Anvil::DescriptorType::COMBINED_IMAGE_SAMPLER,
			Anvil::ShaderStageFlagBits::FRAGMENT_BIT
		}
	}
};
msys::source2::ShaderDecomposePBR::ShaderDecomposePBR(prosper::Context &context,const std::string &identifier)
	: ShaderBaseImageProcessing{context,identifier,"util/source2/fs_decompose_pbr.gls"}
{}

msys::source2::ShaderDecomposePBR::DecomposedImageSet msys::source2::ShaderDecomposePBR::DecomposePBR(prosper::Context &context,prosper::Texture &albedoMap,prosper::Texture &normalMap)
{
	prosper::util::ImageCreateInfo imgCreateInfo {};
	imgCreateInfo.format = Anvil::Format::R8G8B8A8_UNORM;
	imgCreateInfo.memoryFeatures = prosper::util::MemoryFeatureFlags::GPUBulk;
	imgCreateInfo.postCreateLayout = Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL;
	imgCreateInfo.tiling = Anvil::ImageTiling::OPTIMAL;
	imgCreateInfo.usage = Anvil::ImageUsageFlagBits::COLOR_ATTACHMENT_BIT | Anvil::ImageUsageFlagBits::TRANSFER_SRC_BIT;

	auto &imgAlbedo = albedoMap.GetImage();
	auto &imgNormal = normalMap.GetImage();

	auto extents = imgAlbedo->GetExtents();
	imgCreateInfo.width = extents.width;
	imgCreateInfo.height = extents.height;
	auto &dev = context.GetDevice();
	auto imgAlbedoOut = prosper::util::create_image(context.GetDevice(),imgCreateInfo);
	auto imgMetallicRoughnessOut = prosper::util::create_image(context.GetDevice(),imgCreateInfo);

	prosper::util::ImageViewCreateInfo imgViewCreateInfo {};
	auto texAlbedoOut = prosper::util::create_texture(dev,{},imgAlbedoOut,&imgViewCreateInfo);
	auto texMetallicRoughnessOut = prosper::util::create_texture(dev,{},imgMetallicRoughnessOut,&imgViewCreateInfo);
	auto rt = prosper::util::create_render_target(dev,{texMetallicRoughnessOut,texAlbedoOut},GetRenderPass());

	auto dsg = CreateDescriptorSetGroup(DESCRIPTOR_SET_TEXTURE.setIndex);
	auto &ds = *dsg->GetDescriptorSet();
	prosper::util::set_descriptor_set_binding_texture(ds,albedoMap,umath::to_integral(TextureBinding::AlbedoMap));
	prosper::util::set_descriptor_set_binding_texture(ds,normalMap,umath::to_integral(TextureBinding::NormalMap));
	auto &setupCmd = context.GetSetupCommandBuffer();
	if(prosper::util::record_begin_render_pass(**setupCmd,*rt))
	{
		if(BeginDraw(setupCmd))
		{
			Draw(*ds);
			EndDraw();
		}
		prosper::util::record_end_render_pass(**setupCmd);
	}
	context.FlushSetupCommandBuffer();

	return {
		texMetallicRoughnessOut->GetImage(),
		texAlbedoOut->GetImage()
	};
}

void msys::source2::ShaderDecomposePBR::InitializeGfxPipeline(Anvil::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx)
{
	ShaderGraphics::InitializeGfxPipeline(pipelineInfo,pipelineIdx);

	AddDefaultVertexAttributes(pipelineInfo);
	AddDescriptorSetGroup(pipelineInfo,DESCRIPTOR_SET_TEXTURE);
}

void msys::source2::ShaderDecomposePBR::InitializeRenderPass(std::shared_ptr<prosper::RenderPass> &outRenderPass,uint32_t pipelineIdx)
{
	CreateCachedRenderPass<msys::source2::ShaderDecomposePBR>(
		std::vector<prosper::util::RenderPassCreateInfo::AttachmentInfo>{
			{Anvil::Format::R8G8B8A8_UNORM}, // MetallicRoughness
			{Anvil::Format::R8G8B8A8_UNORM} // Albedo
	},outRenderPass,pipelineIdx);
}
#pragma optimize("",on)
