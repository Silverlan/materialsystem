/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __CMATERIAL_H__
#define __CMATERIAL_H__

#include "cmatsysdefinitions.h"
#include "texturemanager/texture.h"
#include "texture_load_flags.hpp"
#include <shader/prosper_shader.hpp>
#include <sharedutils/util_weak_handle.hpp>
#include <material.h>
#include <functional>

namespace prosper {class Sampler;};

class CMaterialManager;
class TextureManager;
namespace ds
{
	class Texture;
	class Cubemap;
};
#pragma warning(push)
#pragma warning(disable : 4251)
class DLLCMATSYS CMaterial
	: public Material
{
public:
	struct DLLCMATSYS CallbackInfo
	{
		CallbackInfo(const std::function<void(std::shared_ptr<Texture>)> &onload=nullptr);
		uint32_t count;
		std::function<void(std::shared_ptr<Texture>)> onload;
	};
public:
	CMaterial(MaterialManager *manager);
	CMaterial(MaterialManager *manager,const util::WeakHandle<ShaderInfo> &shader,const std::shared_ptr<ds::Block> &data);
	CMaterial(MaterialManager *manager,const std::string &shader,const std::shared_ptr<ds::Block> &data);
	void SetOnLoadedCallback(const std::function<void(void)> &f);
	void SetTexture(const std::string &identifier,Texture *texture);
	void SetTexture(const std::string &identifier,const std::string &texture);
	void SetTexture(const std::string &identifier,prosper::Texture &texture);
	virtual Material *Copy() const override;
	const std::shared_ptr<prosper::DescriptorSetGroup> &GetDescriptorSetGroup(prosper::Shader &shader) const;
	bool IsInitialized() const;
	void SetDescriptorSetGroup(prosper::Shader &shader,const std::shared_ptr<prosper::DescriptorSetGroup> &descSetGroup);
	util::WeakHandle<prosper::Shader> GetPrimaryShader();
	std::shared_ptr<prosper::Sampler> GetSampler();
protected:
	void LoadTexture(const std::shared_ptr<ds::Block> &data,TextureInfo &texInfo,TextureLoadFlags flags=TextureLoadFlags::None,const std::shared_ptr<CallbackInfo> &callbackInfo=nullptr);
	void InitializeTextures(const std::shared_ptr<ds::Block> &data,const std::function<void(void)> &onAllTexturesLoaded=nullptr,const std::function<void(std::shared_ptr<Texture>)> &onTextureLoaded=nullptr,bool bLoadInstantly=false);
	friend CMaterialManager;
	TextureManager &GetTextureManager();
private:
	struct ShaderHash
	{
		std::size_t operator()(const util::WeakHandle<prosper::Shader> &whShader) const;
	};
	struct ShaderEqualFn
	{
		bool operator()(const util::WeakHandle<prosper::Shader> &whShader0,const util::WeakHandle<prosper::Shader> &whShader1) const;
	};

	virtual ~CMaterial() override;
	std::function<void(void)> m_fOnLoaded;
	std::unordered_map<util::WeakHandle<prosper::Shader>,std::shared_ptr<prosper::DescriptorSetGroup>,ShaderHash,ShaderEqualFn> m_descriptorSetGroups;
	std::shared_ptr<prosper::Sampler> m_sampler = nullptr;
	std::shared_ptr<CallbackInfo> m_callbackInfo;
	std::unordered_map<util::WeakHandle<prosper::Shader>,std::shared_ptr<prosper::DescriptorSetGroup>,ShaderHash,ShaderEqualFn>::iterator FindShaderDescriptorSetGroup(prosper::Shader &shader);
	std::unordered_map<util::WeakHandle<prosper::Shader>,std::shared_ptr<prosper::DescriptorSetGroup>,ShaderHash,ShaderEqualFn>::const_iterator FindShaderDescriptorSetGroup(prosper::Shader &shader) const;
	std::shared_ptr<CallbackInfo> InitializeCallbackInfo(const std::function<void(void)> &onAllTexturesLoaded,const std::function<void(std::shared_ptr<Texture>)> &onTextureLoaded);

	uint32_t GetMipmapMode(const std::shared_ptr<ds::Block> &data) const;
	prosper::Context &GetContext();
	void LoadTexture(const std::shared_ptr<ds::Block> &data,const std::shared_ptr<ds::Texture> &texture,TextureLoadFlags flags=TextureLoadFlags::None,const std::shared_ptr<CallbackInfo> &callbackInfo=nullptr);
	void LoadTexture(const std::shared_ptr<ds::Block> &data,const std::shared_ptr<ds::Cubemap> &texture,TextureLoadFlags flags=TextureLoadFlags::None,const std::shared_ptr<CallbackInfo> &callbackInfo=nullptr);
	void InitializeSampler();
	void InitializeTextures(const std::shared_ptr<ds::Block> &data,const std::shared_ptr<CallbackInfo> &info,bool bLoadInstantly);
};
#pragma warning(pop)

#endif
