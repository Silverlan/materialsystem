/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __C_SHADER_DECOMPOSE_PBR_HPP__
#define __C_SHADER_DECOMPOSE_PBR_HPP__

#include "cmatsysdefinitions.h"
#include <shader/prosper_shader_base_image_processing.hpp>

namespace prosper
{
	class IImage;
	class Texture;
};
namespace msys
{
	namespace source2
	{
		class DLLCMATSYS ShaderDecomposePBR
			: public prosper::ShaderBaseImageProcessing
		{
		public:
			static prosper::DescriptorSetInfo DESCRIPTOR_SET_TEXTURE;

			enum class TextureBinding : uint32_t
			{
				AlbedoMap = 0u,
				NormalMap,
				AnisotropicGlossinessMap,
				AmbientOcclusionMap,

				Count
			};

			enum class Flags : uint32_t
			{
				None = 0,
				TreatAlphaAsTransparency = 1,
				SpecularWorkflow = TreatAlphaAsTransparency<<1u,
				TreatAlphaAsSSS = SpecularWorkflow<<1u
			};

#pragma pack(push,1)
			struct PushConstants
			{
				Flags flags = Flags::None;
			};
#pragma pack(pop)

			struct DLLCMATSYS DecomposedImageSet
			{
				std::shared_ptr<prosper::IImage> rmaMap = nullptr;
				std::shared_ptr<prosper::IImage> albedoMap = nullptr;
			};

			ShaderDecomposePBR(prosper::IPrContext &context,const std::string &identifier);
			DecomposedImageSet DecomposePBR(
				prosper::IPrContext &context,prosper::Texture &albedoMap,prosper::Texture &normalMap,prosper::Texture &aoMap,
				Flags flags=Flags::None,prosper::Texture *optAniGlossMap=nullptr
			);
		protected:
			virtual void InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo,uint32_t pipelineIdx) override;
			virtual void InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass,uint32_t pipelineIdx) override;
		};
	};
};
REGISTER_BASIC_BITWISE_OPERATORS(msys::source2::ShaderDecomposePBR::Flags)

#endif
