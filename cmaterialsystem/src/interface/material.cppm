// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "cmatsysdefinitions.hpp"
#include <functional>
#include <memory>
#include <string_view>

#include <optional>

export module pragma.cmaterialsystem:material;

export import :sprite_sheet_animation;
export import :texture_manager;
export import pragma.materialsystem;

export {
	namespace msys {
		#pragma warning(push)
		#pragma warning(disable : 4251)
		class CMaterialManager;
		class DLLCMATSYS CMaterial : public Material {
		public:
			struct DLLCMATSYS CallbackInfo {
				CallbackInfo(const std::function<void(std::shared_ptr<Texture>)> &onload = nullptr);
				uint32_t count;
				CallbackHandle onload {};
			};
		public:
			enum class StateFlags : uint8_t { None = 0, TexturesInitialized = 1u, TexturesLoaded = TexturesInitialized << 1u, TexturesPrecached = TexturesLoaded << 1u };
			static std::shared_ptr<CMaterial> Create(msys::MaterialManager &manager);
			static std::shared_ptr<CMaterial> Create(msys::MaterialManager &manager, const util::WeakHandle<util::ShaderInfo> &shader, const std::shared_ptr<ds::Block> &data);
			static std::shared_ptr<CMaterial> Create(msys::MaterialManager &manager, const std::string &shader, const std::shared_ptr<ds::Block> &data);
			virtual ~CMaterial() override;
			void SetOnLoadedCallback(const std::function<void(void)> &f);
			void SetTexture(const std::string &identifier, Texture *texture);
			void SetTexture(const std::string &identifier, const std::string &texture);
			void SetTexture(const std::string &identifier, prosper::Texture &texture);
			const std::shared_ptr<prosper::IDescriptorSetGroup> &GetDescriptorSetGroup(prosper::Shader &shader) const;
			virtual TextureInfo *GetTextureInfo(const std::string_view &key) override;
			bool IsInitialized() const;
			virtual std::shared_ptr<Material> Copy(bool copyData = true) const override;
			void SetDescriptorSetGroup(prosper::Shader &shader, const std::shared_ptr<prosper::IDescriptorSetGroup> &descSetGroup);
			prosper::Shader *GetPrimaryShader();
			void SetPrimaryShader(prosper::Shader &shader);
			std::shared_ptr<prosper::ISampler> GetSampler();
			void SetSettingsBuffer(prosper::IBuffer &buffer);
			prosper::IBuffer *GetSettingsBuffer();
			virtual void SetLoaded(bool b) override;
			virtual void SetShaderInfo(const util::WeakHandle<util::ShaderInfo> &shaderInfo) override;
			virtual void Reset() override;
			virtual void Assign(const Material &other) override;
			void ClearDescriptorSets();
			using Material::Initialize;

			void SetSpriteSheetAnimation(const SpriteSheetAnimation &animInfo);
			void ClearSpriteSheetAnimation();
			const SpriteSheetAnimation *GetSpriteSheetAnimation() const;
			SpriteSheetAnimation *GetSpriteSheetAnimation();

			void LoadTextures(bool precache, bool force = false);
		protected:
			CMaterial(msys::MaterialManager &manager);
			CMaterial(msys::MaterialManager &manager, const util::WeakHandle<util::ShaderInfo> &shader, const std::shared_ptr<ds::Block> &data);
			CMaterial(msys::MaterialManager &manager, const std::string &shader, const std::shared_ptr<ds::Block> &data);
			void ResetRenderResources();
			virtual void OnBaseMaterialChanged() override;
			virtual void Initialize(const std::shared_ptr<ds::Block> &data) override;
			virtual void OnTexturesUpdated() override;
			void LoadTexture(const std::shared_ptr<ds::Block> &data, TextureInfo &texInfo, TextureLoadFlags flags = TextureLoadFlags::None, const std::shared_ptr<CallbackInfo> &callbackInfo = nullptr);
			void LoadTexture(TextureMipmapMode mipmapMode, TextureInfo &texInfo, TextureLoadFlags flags = TextureLoadFlags::None, const std::shared_ptr<CallbackInfo> &callbackInfo = nullptr);
			void InitializeTextures(const std::shared_ptr<ds::Block> &data, const std::function<void(void)> &onAllTexturesLoaded = nullptr, const std::function<void(std::shared_ptr<Texture>)> &onTextureLoaded = nullptr, TextureLoadFlags loadFlags = TextureLoadFlags::None);
			friend msys::CMaterialManager;
			msys::TextureManager &GetTextureManager();

			void LoadTextures(ds::Block &data, bool precache, bool force);
			void LoadTexture(TextureInfo &texInfo, bool precache);

			bool HaveTexturesBeenInitialized() const;
		private:
			void UpdatePrimaryShader();
			struct ShaderHash {
				std::size_t operator()(const util::WeakHandle<prosper::Shader> &whShader) const;
			};
			struct ShaderEqualFn {
				bool operator()(const util::WeakHandle<prosper::Shader> &whShader0, const util::WeakHandle<prosper::Shader> &whShader1) const;
			};

			std::function<void(void)> m_fOnLoaded;
			prosper::Shader *m_primaryShader = nullptr;
			std::vector<CallbackHandle> m_onVkTexturesChanged;
			std::unordered_map<util::WeakHandle<prosper::Shader>, std::shared_ptr<prosper::IDescriptorSetGroup>, ShaderHash, ShaderEqualFn> m_descriptorSetGroups;
			std::shared_ptr<prosper::ISampler> m_sampler = nullptr;
			std::shared_ptr<prosper::IBuffer> m_settingsBuffer = nullptr;
			std::shared_ptr<CallbackInfo> m_callbackInfo;
			std::optional<SpriteSheetAnimation> m_spriteSheetAnimation {};
			StateFlags m_stateFlags = StateFlags::None;
			std::unordered_map<util::WeakHandle<prosper::Shader>, std::shared_ptr<prosper::IDescriptorSetGroup>, ShaderHash, ShaderEqualFn>::iterator FindShaderDescriptorSetGroup(prosper::Shader &shader);
			std::unordered_map<util::WeakHandle<prosper::Shader>, std::shared_ptr<prosper::IDescriptorSetGroup>, ShaderHash, ShaderEqualFn>::const_iterator FindShaderDescriptorSetGroup(prosper::Shader &shader) const;
			std::shared_ptr<CallbackInfo> InitializeCallbackInfo(const std::function<void(void)> &onAllTexturesLoaded, const std::function<void(std::shared_ptr<Texture>)> &onTextureLoaded);

			uint32_t GetMipmapMode(const ds::Block &block) const;
			prosper::IPrContext &GetContext();
			void LoadTexture(const std::shared_ptr<ds::Block> &data, const std::shared_ptr<ds::Texture> &texture, TextureLoadFlags flags = TextureLoadFlags::None, const std::shared_ptr<CallbackInfo> &callbackInfo = nullptr);
			void InitializeSampler();
			void InitializeTextures(const std::shared_ptr<ds::Block> &data, const std::shared_ptr<CallbackInfo> &info, TextureLoadFlags loadFlags);
		};
		#pragma warning(pop)
		using namespace umath::scoped_enum::bitwise;
	}
	namespace umath::scoped_enum::bitwise {
		template<>
		struct enable_bitwise_operators<msys::CMaterial::StateFlags> : std::true_type {};
	}
}
