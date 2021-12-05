/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __CMATERIALMANAGER_H__
#define __CMATERIALMANAGER_H__

#include "cmatsysdefinitions.h"
#include <materialmanager.h>
#include "texturemanager/texture_manager2.hpp"
#include <functional>
#include <queue>
#include <prosper_context_object.hpp>

class Texture;
#pragma warning(push)
#pragma warning(disable : 4251)
class CMaterial;
namespace msys {class TextureManager;};
class DLLCMATSYS CMaterialManager
	: public MaterialManager,
	public prosper::ContextObject
{
private:
	std::function<void(Material*)> m_shaderHandler;
	using MaterialManager::CreateMaterial;
	std::unique_ptr<msys::TextureManager> m_textureManager = nullptr;
	std::queue<MaterialHandle> m_reloadShaderQueue;
	virtual Material *CreateMaterial(const std::string *identifier,const std::string &shader,std::shared_ptr<ds::Block> root=nullptr) override;
#ifndef DISABLE_VMT_SUPPORT
	virtual bool InitializeVMTData(VTFLib::CVMTFile &vmt,LoadInfo &info,ds::Block &rootData,ds::Settings &settings,const std::string &shader) override;
#endif
#ifndef DISABLE_VMAT_SUPPORT
	virtual bool InitializeVMatData(
		source2::resource::Resource &resource,source2::resource::Material &vmat,LoadInfo &info,ds::Block &rootData,ds::Settings &settings,const std::string &shader,
		VMatOrigin origin
	) override;
#endif
public:
	CMaterialManager(prosper::IPrContext &context);
	virtual ~CMaterialManager() override;
	Material *CreateMaterial(const std::string &identifier,const std::string &shader,const std::shared_ptr<ds::Block> &root=nullptr);
	Material *CreateMaterial(const std::string &shader,const std::shared_ptr<ds::Block> &root=nullptr);
	msys::TextureManager &GetTextureManager();
	void Update();
	virtual void SetErrorMaterial(Material *mat) override;
	void SetShaderHandler(const std::function<void(Material*)> &handler);
	std::function<void(Material*)> GetShaderHandler() const;
	Material *Load(const std::string &path,const std::function<void(Material*)> &onMaterialLoaded,const std::function<void(std::shared_ptr<Texture>)> &onTextureLoaded=nullptr,bool bReload=false,bool *bFirstTimeError=nullptr,bool bLoadInstantly=false);
	virtual Material *Load(const std::string &path,bool bReload=false,bool *bFirstTimeError=nullptr) override;
	void ReloadMaterialShaders();
	void MarkForReload(CMaterial &mat);

	void SetDownscaleImportedRMATextures(bool downscale);
};
#pragma warning(pop)

#endif
