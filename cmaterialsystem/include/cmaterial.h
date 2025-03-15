/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __CMATERIAL_H__
#define __CMATERIAL_H__

#include "cmatsysdefinitions.h"
#include "texturemanager/texture.h"
#include "texture_load_flags.hpp"
#include "sprite_sheet_animation.hpp"
#include <shader/prosper_shader.hpp>
#include <sharedutils/util_weak_handle.hpp>
#include <material.h>
#include <functional>

namespace prosper {
	class ISampler;
	class IBuffer;
};

namespace msys {
	class CMaterialManager;
};
class TextureManager;
namespace ds {
	class Texture;
};
#pragma warning(push)
#pragma warning(disable : 4251)
struct SpriteSheetAnimation;
namespace msys {
	class TextureManager;
};
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
	virtual std::shared_ptr<Material> Copy() const override;
	void SetDescriptorSetGroup(prosper::Shader &shader, const std::shared_ptr<prosper::IDescriptorSetGroup> &descSetGroup);
	prosper::Shader *GetPrimaryShader();
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
REGISTER_BASIC_BITWISE_OPERATORS(CMaterial::StateFlags)
#pragma warning(pop)

#endif
