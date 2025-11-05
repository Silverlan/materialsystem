// SPDX-FileCopyrightText: (c) 2020 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "definitions.hpp"

export module pragma.cmaterialsystem:shaders.source2.decompose_pbr;

export import pragma.prosper;

export {
	namespace msys {
		namespace source2 {
			class DLLCMATSYS ShaderDecomposePBR : public prosper::ShaderBaseImageProcessing {
			  public:
				static prosper::DescriptorSetInfo DESCRIPTOR_SET_TEXTURE;

				enum class TextureBinding : uint32_t {
					AlbedoMap = 0u,
					NormalMap,
					AnisotropicGlossinessMap,
					AmbientOcclusionMap,

					Count
				};

				enum class Flags : uint32_t { None = 0, TreatAlphaAsTransparency = 1, SpecularWorkflow = TreatAlphaAsTransparency << 1u, TreatAlphaAsSSS = SpecularWorkflow << 1u };

	#pragma pack(push, 1)
				struct PushConstants {
					Flags flags = Flags::None;
				};
	#pragma pack(pop)

				struct DLLCMATSYS DecomposedImageSet {
					std::shared_ptr<prosper::IImage> rmaMap = nullptr;
					std::shared_ptr<prosper::IImage> albedoMap = nullptr;
				};

				ShaderDecomposePBR(prosper::IPrContext &context, const std::string &identifier);
				DecomposedImageSet DecomposePBR(prosper::IPrContext &context, prosper::Texture &albedoMap, prosper::Texture &normalMap, prosper::Texture &aoMap, Flags flags = Flags::None, prosper::Texture *optAniGlossMap = nullptr);
			  protected:
				virtual void InitializeGfxPipeline(prosper::GraphicsPipelineCreateInfo &pipelineInfo, uint32_t pipelineIdx) override;
				virtual void InitializeRenderPass(std::shared_ptr<prosper::IRenderPass> &outRenderPass, uint32_t pipelineIdx) override;
				virtual void InitializeShaderResources() override;
			};
			using namespace umath::scoped_enum::bitwise;
		};
	};
	namespace umath::scoped_enum::bitwise {
		template<>
		struct enable_bitwise_operators<msys::source2::ShaderDecomposePBR::Flags> : std::true_type {};
	}
}
