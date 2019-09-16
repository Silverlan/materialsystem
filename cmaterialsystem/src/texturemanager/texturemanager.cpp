/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "texturemanager/texturemanager.h"
#include "texturemanager/texturequeue.h"
#include "texturemanager/loadimagedata.h"
#include "sharedutils/functioncallback.h"
#include "squish.h"
#include "textureinfo.h"
#include "texturemanager/texture.h"
#include <prosper_context.hpp>
#include <prosper_util.hpp>
#include <image/prosper_sampler.hpp>
#include <mathutil/glmutil.h>
#include <gli/gli.hpp>
#include <gli/texture2d.hpp>
#include <sharedutils/util_file.h>
#ifdef ENABLE_VTF_SUPPORT
#include <Proc.h>
#include "virtualfile.h"
#endif

decltype(TextureManager::MAX_TEXTURE_COUNT) TextureManager::MAX_TEXTURE_COUNT = 4096;

#pragma optimize("",off)
#ifdef ENABLE_VTF_SUPPORT
static vlVoid vtf_read_close() {}
static vlBool vtf_read_open() {return true;}
static vlUInt vtf_read_read(vlVoid *buf,vlUInt bytes,vlVoid *handle)
{
	if(handle == nullptr)
		return -1;
	auto &f = *static_cast<VirtualFile*>(handle);
	return static_cast<vlUInt>(f.Read(buf,bytes));
}
static vlUInt vtf_read_seek(vlLong offset,VLSeekMode whence,vlVoid *handle)
{
	if(handle == nullptr)
		return -1;
	auto &f = *static_cast<VirtualFile*>(handle);
	f.Seek(offset,static_cast<VirtualFile::SeekMode>(whence));
	return f.Tell();
}
static vlUInt vtf_read_size(vlVoid *handle)
{
	if(handle == nullptr)
		return 0;
	auto &f = *static_cast<VirtualFile*>(handle);
	return f.GetSize();
}
static vlUInt vtf_read_tell(vlVoid *handle)
{
	if(handle == nullptr)
		return -1;
	auto &f = *static_cast<VirtualFile*>(handle);
	return f.Tell();
}
#endif

TextureManager::LoadInfo::LoadInfo()
	: mipmapLoadMode(TextureMipmapMode::Load)
{}

TextureManager::TextureManager(prosper::Context &context)
	: m_wpContext(context.shared_from_this()),m_textureSampler(nullptr),
	m_textureSamplerNoMipmap(nullptr),m_bThreadActive(false)
{
	auto samplerCreateInfo = prosper::util::SamplerCreateInfo {};
	TextureManager::SetupSamplerMipmapMode(samplerCreateInfo,TextureMipmapMode::Load);
	m_textureSampler = prosper::util::create_sampler(context.GetDevice(),samplerCreateInfo);

	samplerCreateInfo = {};
	TextureManager::SetupSamplerMipmapMode(samplerCreateInfo,TextureMipmapMode::Ignore);
	m_textureSamplerNoMipmap = prosper::util::create_sampler(context.GetDevice(),samplerCreateInfo);
#ifdef ENABLE_VTF_SUPPORT
	vlSetProc(PROC_READ_CLOSE,reinterpret_cast<void*>(vtf_read_close));
	vlSetProc(PROC_READ_OPEN,reinterpret_cast<void*>(vtf_read_open));
	vlSetProc(PROC_READ_READ,reinterpret_cast<void*>(vtf_read_read));
	vlSetProc(PROC_READ_SEEK,reinterpret_cast<void*>(vtf_read_seek));
	vlSetProc(PROC_READ_SIZE,reinterpret_cast<void*>(vtf_read_size));
	vlSetProc(PROC_READ_TELL,reinterpret_cast<void*>(vtf_read_tell));
#endif
}

TextureManager::~TextureManager()
{
	Clear();
}

bool TextureManager::HasWork()
{
	if(m_bThreadActive == false)
		return false;
	if(m_bBusy)
		return true;
	m_loadMutex->lock();
		auto hasLoadItem = (m_loadQueue.empty() == false);
	m_loadMutex->unlock();
	if(hasLoadItem)
		return true;
	m_queueMutex->lock();
		auto hasInitItem = (m_initQueue.empty() == false);
	m_queueMutex->unlock();
	return hasInitItem;
}

void TextureManager::WaitForTextures()
{
	while(HasWork())
		Update();
	Update();
}

void TextureManager::SetupSamplerMipmapMode(prosper::util::SamplerCreateInfo &createInfo,TextureMipmapMode mode)
{
	switch(mode)
	{
		case TextureMipmapMode::Ignore:
		{
			createInfo.minFilter = Anvil::Filter::NEAREST;
			createInfo.magFilter = Anvil::Filter::NEAREST;
			createInfo.mipmapMode = Anvil::SamplerMipmapMode::NEAREST;
			createInfo.minLod = 0.f;
			createInfo.maxLod = 0.f;
			break;
		}
		default:
		{
			createInfo.minFilter = Anvil::Filter::LINEAR;
			createInfo.magFilter = Anvil::Filter::LINEAR;
			createInfo.mipmapMode = Anvil::SamplerMipmapMode::LINEAR;
			break;
		}
	}
}

void TextureManager::RegisterCustomSampler(const std::shared_ptr<prosper::Sampler> &sampler) {m_customSamplers.push_back(sampler);}
const std::vector<std::weak_ptr<prosper::Sampler>> &TextureManager::GetCustomSamplers() const {return m_customSamplers;}

prosper::Context &TextureManager::GetContext() const {return *m_wpContext.lock();}

void TextureManager::SetTextureFileHandler(const std::function<VFilePtr(const std::string&)> &fileHandler) {m_texFileHandler = fileHandler;}

std::shared_ptr<Texture> TextureManager::CreateTexture(const std::string &name,prosper::Texture &texture)
{
	auto it = std::find_if(m_textures.begin(),m_textures.end(),[&name](std::shared_ptr<Texture> &tex) {
		return (tex->name == name) ? true : false;
	});
	if(it != m_textures.end())
		return *it;
	auto tex = std::make_shared<Texture>();
	tex->texture = texture.shared_from_this();
	tex->name = name;
	m_textures.push_back(tex);
	return tex;
}

std::shared_ptr<Texture> TextureManager::GetErrorTexture() {return m_error;}

/*std::shared_ptr<Texture> TextureManager::CreateTexture(prosper::Context &context,const std::string &name,unsigned int w,unsigned int h)
{
	auto it = std::find_if(m_textures.begin(),m_textures.end(),[&name](std::shared_ptr<Texture> &tex) {
		return (tex->name == name) ? true : false;
	});
	if(it != m_textures.end())
		return *it;
	auto dev = context.GetDevice()->shared_from_this();
	prosper::util::ImageCreateInfo imgCreateInfo {};
	imgCreateInfo.
	auto img = prosper::util::create_image(dev,imgCreateInfo);

	prosper::util::TextureCreateInfo texCreateInfo {};
	prosper::util::ImageViewCreateInfo imgViewCreateInfo {};
	prosper::util::SamplerCreateInfo samplerCreateInfo {};

	prosper::util::create_texture(dev,texCreateInfo,img,&imgViewCreateInfo,&samplerCreateInfo);
	//vk::Format::eR16G16B16A16Sfloat
	try
	{
		auto vkTex = Vulkan::Texture::Create(context,w,h);
		return CreateTexture(name,vkTex);
	}
	catch(Vulkan::Exception &e)
	{
		return nullptr;
	}
}*/

std::shared_ptr<Texture> TextureManager::GetTexture(const std::string &name)
{
	auto nameNoExt = name;
	ufile::remove_extension_from_filename(nameNoExt);
	auto it = std::find_if(m_textures.begin(),m_textures.end(),[&nameNoExt](const std::shared_ptr<Texture> &tex) {
		auto texNoExt = tex->name;
		ufile::remove_extension_from_filename(texNoExt);
		return FileManager::ComparePath(nameNoExt,texNoExt);
	});
	return (it != m_textures.end()) ? *it : nullptr;
}

void TextureManager::ReloadTextures(const LoadInfo &loadInfo)
{
	for(unsigned int i=0;i<m_textures.size();i++)
	{
		auto &texture = m_textures[i];
		ReloadTexture(*texture,loadInfo);
	}
}

void TextureManager::ReloadTexture(uint32_t texId,const LoadInfo &loadInfo)
{
	if(texId >= m_textures.size())
		return;
	auto &texture = m_textures[texId];
	if(texture->texture == nullptr)
		return;
	auto &tex = texture->texture;
	auto &context = tex->GetContext();
	auto sampler = tex->GetSampler();
	auto ptr = std::static_pointer_cast<void>(texture->shared_from_this());
	Load(context,texture->name,loadInfo,&ptr);
	texture = std::static_pointer_cast<Texture>(ptr);
}

void TextureManager::ReloadTexture(Texture &texture,const LoadInfo &loadInfo)
{
	auto it = std::find_if(m_textures.begin(),m_textures.end(),[&texture](const std::shared_ptr<Texture> &texOther) {
		return (texOther.get() == &texture) ? true : false;
	});
	if(it == m_textures.end())
		return;
	ReloadTexture(it -m_textures.begin(),loadInfo);
}

void TextureManager::SetErrorTexture(const std::shared_ptr<Texture> &tex)
{
	m_error = tex;
}

void TextureManager::Clear()
{
	if(m_threadLoad != nullptr)
	{
		m_activeMutex->lock();
			m_bThreadActive = false;
		m_activeMutex->unlock();
		m_queueVar->notify_one();
		m_threadLoad->join();
		m_threadLoad = nullptr;
		m_queueVar = nullptr;
		m_queueMutex = nullptr;
		m_lockMutex = nullptr;
		m_activeMutex = nullptr;
		m_loadMutex = nullptr;
	}
	while(!m_loadQueue.empty())
		m_loadQueue.pop();
	m_textures.clear();
	m_textureSampler = nullptr;
	m_textureSamplerNoMipmap = nullptr;
	m_error = nullptr;
}
void TextureManager::ClearUnused()
{
	for(auto &tex : m_textures)
	{
		if(tex.use_count() == 1)
			tex->Reset();
	}
}

std::shared_ptr<prosper::Sampler> &TextureManager::GetTextureSampler() {return m_textureSampler;}

std::shared_ptr<Texture> TextureManager::FindTexture(const std::string &imgFile,bool *bLoading)
{
	std::string cache;
	return FindTexture(imgFile,&cache,bLoading);
}
std::shared_ptr<Texture> TextureManager::FindTexture(const std::string &imgFile,bool bLoadedOnly)
{
	auto bLoading = false;
	auto r = FindTexture(imgFile,&bLoading);
	if(bLoadedOnly == true && bLoading == true)
		return nullptr;
	return r;
}

std::shared_ptr<Texture> TextureManager::FindTexture(const std::string &imgFile,std::string *cache,bool *bLoading)
{
	if(bLoading != nullptr)
		*bLoading = false;
	auto pathCache = FileManager::GetNormalizedPath(imgFile);
	auto rBr = pathCache.find_last_of('\\');
	if(rBr != std::string::npos)
	{
		auto ext = pathCache.find('.',rBr +1);
		if(ext != std::string::npos)
			pathCache = pathCache.substr(0,ext);
	}
	*cache = pathCache;
	const auto find = [&pathCache](decltype(m_textures.front()) &tex) {
		return (tex->name == pathCache) ? true : false;
	};
	auto itTex = std::find_if(m_textures.begin(),m_textures.end(),find);
	if(itTex != m_textures.end())
		return *itTex;
	auto itTmp = std::find_if(m_texturesTmp.begin(),m_texturesTmp.end(),find);
	if(itTmp != m_texturesTmp.end())
	{
		if(bLoading != nullptr)
			*bLoading = true;
		return *itTmp;
	}
	return nullptr;
}
#pragma optimize("",on)
