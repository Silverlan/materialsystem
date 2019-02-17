/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __MATERIAL_H__
#define __MATERIAL_H__

#include "matsysdefinitions.h"
#include "textureinfo.h"
#include <sharedutils/util_weak_handle.hpp>
#include <sharedutils/def_handle.h>
#include <sharedutils/functioncallback.h>

class Material;
DECLARE_BASE_HANDLE(DLLMATSYS,Material,Material);

class ShaderInfo;
class MaterialManager;
class MaterialHandle;
namespace ds {class Block;};
#pragma warning(push)
#pragma warning(disable : 4251)
class DLLMATSYS Material
{
protected:
	virtual ~Material();
	MaterialHandle m_handle;
	util::WeakHandle<ShaderInfo> m_shaderInfo = {};
	std::unique_ptr<std::string> m_shader;
	std::string m_name;
	std::shared_ptr<ds::Block> m_data;
	bool m_bLoaded;
	std::vector<CallbackHandle> m_callOnLoaded;
	MaterialManager *m_manager;
	bool m_bTranslucent;
	TextureInfo *m_texDiffuse;
	TextureInfo *m_texNormal;
	TextureInfo *m_texSpecular;
	TextureInfo *m_texGlow;
	TextureInfo *m_texAlpha;
	TextureInfo *m_texParallax;
	void *m_userData;
	template<class TMaterial>
		TMaterial *Copy() const;
public:
	Material(MaterialManager *manager);
	Material(MaterialManager *manager,const util::WeakHandle<ShaderInfo> &shaderInfo,const std::shared_ptr<ds::Block> &data);
	Material(MaterialManager *manager,const std::string &shader,const std::shared_ptr<ds::Block> &data);
	Material(const Material&)=delete;
	void Remove();
	MaterialHandle GetHandle();
	void SetShaderInfo(const util::WeakHandle<ShaderInfo> &shaderInfo);
	const ShaderInfo *GetShaderInfo() const;
	void UpdateTextures();
	const std::string &GetShaderIdentifier() const;
	virtual Material *Copy() const;
	void SetName(const std::string &name);
	const std::string &GetName();
	bool IsTranslucent() const;
	virtual TextureInfo *GetTextureInfo(const std::string &key);
	TextureInfo *GetDiffuseMap();
	TextureInfo *GetNormalMap();
	TextureInfo *GetSpecularMap();
	TextureInfo *GetGlowMap();
	TextureInfo *GetAlphaMap();
	TextureInfo *GetParallaxMap();
	const std::shared_ptr<ds::Block> &GetDataBlock();
	void SetLoaded(bool b);
	CallbackHandle CallOnLoaded(const std::function<void(void)> &f);
	bool IsValid() const;
	MaterialManager *GetManager() const;

	// Returns true if all textures associated with this material have been fully loaded
	bool IsLoaded() const;
	void *GetUserData();
	void SetUserData(void *data);
	void Reset();
	void Initialize(const util::WeakHandle<ShaderInfo> &shaderInfo,const std::shared_ptr<ds::Block> &data);
	void Initialize(const std::string &shader,const std::shared_ptr<ds::Block> &data);
};
#pragma warning(pop)

template<class TMaterial>
	TMaterial *Material::Copy() const
{
	TMaterial *r = nullptr;
	if(!IsValid())
		r = new TMaterial{GetManager()};
	else if(m_shaderInfo.expired() == false)
		r = new TMaterial{GetManager(),m_shaderInfo,std::shared_ptr<ds::Block>(m_data->Copy())};
	else
		r = new TMaterial{GetManager(),*m_shader,std::shared_ptr<ds::Block>(m_data->Copy())};
	if(IsLoaded())
		r->SetLoaded(true);
	return r;
}


#endif