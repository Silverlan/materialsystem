/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __MATERIAL_H__
#define __MATERIAL_H__

#include "matsysdefinitions.h"
#include "textureinfo.h"
#include <optional>
#include <sharedutils/util_weak_handle.hpp>
#include <sharedutils/def_handle.h>
#include <sharedutils/functioncallback.h>
#include <mathutil/umath.h>
#include <mathutil/uvec.h>

class Material;
DECLARE_BASE_HANDLE(DLLMATSYS,Material,Material);

using MaterialIndex = uint32_t;
namespace util {class ShaderInfo;};
enum class AlphaMode : uint32_t;
class MaterialManager;
class MaterialHandle;
class VFilePtrInternalReal;
namespace ds {class Block;};
#pragma warning(push)
#pragma warning(disable : 4251)
class DLLMATSYS Material
{
public:
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

	enum class StateFlags : uint32_t
	{
		None = 0u,
		Translucent = 1u,
		Loaded = Translucent<<1u,
		ExecutingOnLoadCallbacks = Loaded<<1u,
		Error = ExecutingOnLoadCallbacks<<1u
	};
	
	Material(MaterialManager &manager);
	Material(MaterialManager &manager,const util::WeakHandle<util::ShaderInfo> &shaderInfo,const std::shared_ptr<ds::Block> &data);
	Material(MaterialManager &manager,const std::string &shader,const std::shared_ptr<ds::Block> &data);
	Material(const Material&)=delete;
	void Remove();
	MaterialHandle GetHandle();
	void SetShaderInfo(const util::WeakHandle<util::ShaderInfo> &shaderInfo);
	const util::ShaderInfo *GetShaderInfo() const;
	void UpdateTextures();
	const std::string &GetShaderIdentifier() const;
	virtual Material *Copy() const;
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
	MaterialManager &GetManager() const;
	std::optional<std::string> GetAbsolutePath() const;
	bool Save(std::shared_ptr<VFilePtrInternalReal> f) const;
	bool Save(const std::string &fileName,const std::string &rootPath="") const;
	bool Save() const;

	MaterialIndex GetIndex() const {return m_index;}

	// Returns true if all textures associated with this material have been fully loaded
	bool IsLoaded() const;
	void *GetUserData();
	void SetUserData(void *data);
	void Reset();
	void Initialize(const util::WeakHandle<util::ShaderInfo> &shaderInfo,const std::shared_ptr<ds::Block> &data);
	void Initialize(const std::string &shader,const std::shared_ptr<ds::Block> &data);
protected:
	friend MaterialManager;
	virtual ~Material();
	void SetIndex(MaterialIndex index) {m_index = index;}
	MaterialHandle m_handle;
	util::WeakHandle<util::ShaderInfo> m_shaderInfo = {};
	std::unique_ptr<std::string> m_shader;
	std::string m_name;
	std::shared_ptr<ds::Block> m_data;
	StateFlags m_stateFlags = StateFlags::None;
	mutable std::vector<CallbackHandle> m_callOnLoaded;
	MaterialManager &m_manager;
	TextureInfo *m_texDiffuse = nullptr;
	TextureInfo *m_texNormal = nullptr;
	TextureInfo *m_texGlow = nullptr;
	TextureInfo *m_texAlpha = nullptr;
	TextureInfo *m_texParallax = nullptr;
	TextureInfo *m_texRma = nullptr;
	void *m_userData;
	MaterialIndex m_index = std::numeric_limits<MaterialIndex>::max();
	template<class TMaterial>
	TMaterial *Copy() const;
};
REGISTER_BASIC_ARITHMETIC_OPERATORS(Material::StateFlags)
#pragma warning(pop)

template<class TMaterial>
	TMaterial *Material::Copy() const
{
	Material *r = nullptr;
	if(!IsValid())
		r = GetManager().CreateMaterial("pbr");
	else if(m_shaderInfo.expired() == false)
		r = GetManager().CreateMaterial(m_shaderInfo->GetIdentifier(),std::shared_ptr<ds::Block>(m_data->Copy()));
	else
		r = GetManager().CreateMaterial(*m_shader,std::shared_ptr<ds::Block>(m_data->Copy()));
	if(IsLoaded())
		r->SetLoaded(true);
	r->m_stateFlags = m_stateFlags;
	return static_cast<TMaterial*>(r);
}


#endif