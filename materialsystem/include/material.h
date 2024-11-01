/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __MATERIAL_H__
#define __MATERIAL_H__

#include "matsysdefinitions.h"
#include "textureinfo.h"
#include <optional>
#include <sharedutils/util_path.hpp>
#include <sharedutils/util_weak_handle.hpp>
#include <sharedutils/def_handle.h>
#include <sharedutils/functioncallback.h>
#include <sharedutils/alpha_mode.hpp>
#include <mathutil/umath.h>
#include <mathutil/uvec.h>

class Material;
namespace msys {
	using MaterialHandle = std::shared_ptr<Material>;
	class MaterialManager;
};

using MaterialIndex = uint32_t;
namespace util {
	class ShaderInfo;
};
class VFilePtrInternalReal;
namespace ds {
	class Block;
};
namespace udm {
	struct AssetData;
};
#pragma warning(push)
#pragma warning(disable : 4251)
class DLLMATSYS Material : public std::enable_shared_from_this<Material> {
  public:
	static constexpr auto PMAT_IDENTIFIER = "PMAT";
	static constexpr uint32_t PMAT_VERSION = 1;

	static constexpr auto FORMAT_MATERIAL_BINARY = "pmat_b";
	static constexpr auto FORMAT_MATERIAL_ASCII = "pmat";
	static constexpr auto FORMAT_MATERIAL_LEGACY = "wmi";

	// inline static class-strings cause an internal compiler error with VS2019, so we can't use them for the time being
	// inline static const std::string DIFFUSE_MAP_IDENTIFIER = "diffuse_map";
	static const std::string DIFFUSE_MAP_IDENTIFIER;
	static const std::string ALBEDO_MAP_IDENTIFIER;
	static const std::string ALBEDO_MAP2_IDENTIFIER;
	static const std::string ALBEDO_MAP3_IDENTIFIER;
	static const std::string NORMAL_MAP_IDENTIFIER;
	static const std::string GLOW_MAP_IDENTIFIER;
	static const std::string EMISSION_MAP_IDENTIFIER;
	static const std::string PARALLAX_MAP_IDENTIFIER;
	static const std::string ALPHA_MAP_IDENTIFIER;
	static const std::string RMA_MAP_IDENTIFIER;
	static const std::string DUDV_MAP_IDENTIFIER;
	static const std::string WRINKLE_STRETCH_MAP_IDENTIFIER;
	static const std::string WRINKLE_COMPRESS_MAP_IDENTIFIER;
	static const std::string EXPONENT_MAP_IDENTIFIER;

	enum class StateFlags : uint32_t { None = 0u, Loaded = 1u, ExecutingOnLoadCallbacks = Loaded << 1u, Error = ExecutingOnLoadCallbacks << 1u, TexturesUpdated = Error << 1u };

	static std::shared_ptr<Material> Create(msys::MaterialManager &manager);
	static std::shared_ptr<Material> Create(msys::MaterialManager &manager, const util::WeakHandle<util::ShaderInfo> &shaderInfo, const std::shared_ptr<ds::Block> &data);
	static std::shared_ptr<Material> Create(msys::MaterialManager &manager, const std::string &shader, const std::shared_ptr<ds::Block> &data);
	Material(const Material &) = delete;
	virtual ~Material();
	msys::MaterialHandle GetHandle();
	virtual void SetShaderInfo(const util::WeakHandle<util::ShaderInfo> &shaderInfo);
	const util::ShaderInfo *GetShaderInfo() const;
	void UpdateTextures(bool forceUpdate = false);
	const std::string &GetShaderIdentifier() const;
	void SetName(const std::string &name);
	const std::string &GetName();
	bool IsTranslucent() const;
	bool IsError() const;
	void SetErrorFlag(bool set);
	virtual TextureInfo *GetTextureInfo(const std::string &key);
	const TextureInfo *GetTextureInfo(const std::string &key) const;

	const TextureInfo *GetDiffuseMap() const;
	TextureInfo *GetDiffuseMap();

	const TextureInfo *GetAlbedoMap() const;
	TextureInfo *GetAlbedoMap();

	const TextureInfo *GetNormalMap() const;
	TextureInfo *GetNormalMap();

	const TextureInfo *GetGlowMap() const;
	TextureInfo *GetGlowMap();

	const TextureInfo *GetAlphaMap() const;
	TextureInfo *GetAlphaMap();

	const TextureInfo *GetParallaxMap() const;
	TextureInfo *GetParallaxMap();

	const TextureInfo *GetRMAMap() const;
	TextureInfo *GetRMAMap();

	AlphaMode GetAlphaMode() const;
	float GetAlphaCutoff() const;

	void SetColorFactor(const Vector4 &colorFactor);
	Vector4 GetColorFactor() const;
	void SetBloomColorFactor(const Vector4 &bloomColorFactor);
	std::optional<Vector4> GetBloomColorFactor() const;

	const std::shared_ptr<ds::Block> &GetDataBlock() const;
	virtual void SetLoaded(bool b);
	CallbackHandle CallOnLoaded(const std::function<void(void)> &f) const;
	bool IsValid() const;
	msys::MaterialManager &GetManager() const;
	std::optional<std::string> GetAbsolutePath() const;

	bool Save(udm::AssetData outData, std::string &outErr);
	bool Save(const std::string &fileName, std::string &outErr, bool absolutePath = false);
	bool Save(std::string &outErr);

	bool SaveLegacy(std::shared_ptr<VFilePtrInternalReal> f) const;
	bool SaveLegacy(const std::string &fileName, const std::string &rootPath = "") const;
	bool SaveLegacy() const;

	MaterialIndex GetIndex() const { return m_index; }
	uint32_t GetUpdateIndex() const { return m_updateIndex; }

	virtual void Assign(const Material &other);

	virtual std::shared_ptr<Material> Copy() const;

	// Returns true if all textures associated with this material have been fully loaded
	bool IsLoaded() const;
	void *GetUserData();
	void SetUserData(void *data);
	void *GetUserData2() { return m_userData2; }
	void SetUserData2(void *data) { m_userData2 = data; }
	virtual void Reset();
	void Initialize(const util::WeakHandle<util::ShaderInfo> &shaderInfo, const std::shared_ptr<ds::Block> &data);
	void Initialize(const std::string &shader, const std::shared_ptr<ds::Block> &data);
  protected:
	friend msys::MaterialManager;
	Material(msys::MaterialManager &manager);
	Material(msys::MaterialManager &manager, const util::WeakHandle<util::ShaderInfo> &shaderInfo, const std::shared_ptr<ds::Block> &data);
	Material(msys::MaterialManager &manager, const std::string &shader, const std::shared_ptr<ds::Block> &data);
	virtual void Initialize(const std::shared_ptr<ds::Block> &data);
	virtual void OnTexturesUpdated();
	void SetIndex(MaterialIndex index) { m_index = index; }
	uint32_t m_updateIndex = 0;
	util::WeakHandle<util::ShaderInfo> m_shaderInfo = {};
	std::unique_ptr<std::string> m_shader;
	std::string m_name;
	std::shared_ptr<ds::Block> m_data;
	StateFlags m_stateFlags = StateFlags::None;
	mutable std::vector<CallbackHandle> m_callOnLoaded;
	msys::MaterialManager &m_manager;
	TextureInfo *m_texDiffuse = nullptr;
	TextureInfo *m_texNormal = nullptr;
	TextureInfo *m_texGlow = nullptr;
	TextureInfo *m_texAlpha = nullptr;
	TextureInfo *m_texParallax = nullptr;
	TextureInfo *m_texRma = nullptr;
	void *m_userData;
	void *m_userData2 = nullptr;
	AlphaMode m_alphaMode = AlphaMode::Opaque;
	MaterialIndex m_index = std::numeric_limits<MaterialIndex>::max();
};
REGISTER_BASIC_ARITHMETIC_OPERATORS(Material::StateFlags)
#pragma warning(pop)

DLLMATSYS std::ostream &operator<<(std::ostream &out, const Material &o);

#endif
