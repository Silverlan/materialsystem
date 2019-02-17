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
#include <VTFLib/Proc.h>
#include "virtualfile.h"
#endif

decltype(TextureManager::MAX_TEXTURE_COUNT) TextureManager::MAX_TEXTURE_COUNT = 4096;

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
	: mipmapLoadMode(TextureMipmapMode::LoadOrGenerate)
{}

TextureManager::TextureManager(prosper::Context &context)
	: m_wpContext(context.shared_from_this()),m_textureSampler(nullptr),
	m_textureSamplerNoMipmap(nullptr),m_bThreadActive(false)
{
	auto samplerCreateInfo = prosper::util::SamplerCreateInfo {};
	TextureManager::SetupSamplerMipmapMode(samplerCreateInfo,TextureMipmapMode::LoadOrGenerate);
	m_textureSampler = prosper::util::create_sampler(context.GetDevice(),samplerCreateInfo);

	samplerCreateInfo = {};
	TextureManager::SetupSamplerMipmapMode(samplerCreateInfo,TextureMipmapMode::Ignore);
	m_textureSamplerNoMipmap = prosper::util::create_sampler(context.GetDevice(),samplerCreateInfo);
#ifdef ENABLE_VTF_SUPPORT
	vlSetProc(PROC_READ_CLOSE,vtf_read_close);
	vlSetProc(PROC_READ_OPEN,vtf_read_open);
	vlSetProc(PROC_READ_READ,vtf_read_read);
	vlSetProc(PROC_READ_SEEK,vtf_read_seek);
	vlSetProc(PROC_READ_SIZE,vtf_read_size);
	vlSetProc(PROC_READ_TELL,vtf_read_tell);
#endif
}

TextureManager::~TextureManager()
{
	Clear();
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
////////////
////////////const GLenum cubemapTargets[6] = {
////////////	GL_TEXTURE_CUBE_MAP_POSITIVE_Y, // Up
////////////	GL_TEXTURE_CUBE_MAP_NEGATIVE_X, // Left
////////////	GL_TEXTURE_CUBE_MAP_POSITIVE_Z, // Front
////////////	GL_TEXTURE_CUBE_MAP_POSITIVE_X, // Right
////////////	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, // Back
////////////	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y // Down
////////////};
////////////static void InitializeGLDDS(TextureQueueItem *item,Texture *texture,TextureQueueItemDDS *dds,nv_dds::CDDSImage *img)
////////////{
////////////	gli::texture2d tex2D(gli::load(fileName));
////////////	if(tex2D.empty())
////////////		return false;
////////////	auto &img = tex2D[0];
////////////	auto extent = img.extent();
////////////	auto width = extent.x;
////////////	auto height = extent.y;
////////////	auto mipmapLevels = tex2D.levels();
////////////	auto format = static_cast<vk::Format>(tex2D.format());
////////////	auto &context = *m_context.get();
////////////
////////////	std::vector<Vulkan::Image> mipLevels(mipmapLevels,nullptr);
////////////	for(auto level=decltype(mipmapLevels){0};level<mipmapLevels;++level)
////////////	{
////////////		auto img = Vulkan::Image::Create(context,tex2D[level].extent().x,tex2D[level].extent().y,static_cast<vk::Format>(format),true,[](vk::ImageCreateInfo &info,vk::MemoryPropertyFlags &flags) {
////////////			info.tiling(vk::ImageTiling::eLinear);
////////////			info.usage(vk::ImageUsageFlagBits::eTransferSrc);
////////////		});
////////////		vk::SubresourceLayout subResLayout {};
////////////		auto map = img->MapMemory(0,subResLayout);
////////////		auto *data = map->GetData();
////////////		memcpy(data,tex2D[level].data(),tex2D[level].size());
////////////		map = nullptr;
////////////		img->SetLayout(vk::ImageLayout::eTransferSrcOptimal);
////////////		mipLevels[level] = img;
////////////	}
////////////
////////////	texture = Vulkan::Image::Create(context,width,height,static_cast<vk::Format>(format),true,false,[](vk::ImageCreateInfo &info,vk::MemoryPropertyFlags &flags) {
////////////		info.tiling(vk::ImageTiling::eOptimal);
////////////		info.usage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled);
////////////	});
////////////	texture->SetLayout(vk::ImageLayout::eTransferDstOptimal);
////////////
////////////	for(auto level=decltype(mipmapLevels){0};level<mipmapLevels;++level)
////////////	{
////////////		mipLevels[level]->Copy(texture,tex2D[level].extent().x,tex2D[level].extent().y,[level](vk::ImageCopy &copy) {
////////////			copy.dstSubresource().mipLevel(level);
////////////		});
////////////	}
////////////	texture->SetLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
////////////
////////////
////////////////////////	vk::Format format;
////////////////////////	auto glFormat = img->get_format();
////////////////////////	switch(glFormat)
////////////////////////	{
////////////////////////	case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
////////////////////////		format = vk::Format::eBc1RgbaUnormBlock;
////////////////////////		break;
////////////////////////	case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
////////////////////////		format = vk::Format::eBc2UnormBlock;
////////////////////////		break;
////////////////////////	case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
////////////////////////		format = vk::Format::eBc3UnormBlock;
////////////////////////		break;
////////////////////////	};
////////////////////////	try
////////////////////////	{
////////////////////////		auto w = texture->width;
////////////////////////		auto h = texture->height;
////////////////////////		texture->vkTexture = Vulkan::Texture::Create(*item->context.get(),w,h,format,true,std::function<void(vk::ImageCreateInfo&,vk::MemoryPropertyFlags&)>([](vk::ImageCreateInfo &info,vk::MemoryPropertyFlags &flags) {
////////////////////////			//info.tiling(vk::ImageTiling::eLinear);
////////////////////////			info.usage(vk::ImageUsageFlagBits::eSampled);
////////////////////////		}));
////////////////////////
////////////////////////		auto &vkImg = texture->vkTexture->GetImage();
////////////////////////		//auto &mem = vkImg->GetMemory();
////////////////////////		//auto &vkImg = texture->vkTexture->GetImage();
////////////////////////
////////////////////////		unsigned int mipmap = 0;
////////////////////////		unsigned int xBlocks = 0;
////////////////////////		unsigned int yBlocks = 0;
////////////////////////		unsigned int blockSize = 0;
////////////////////////		unsigned int lineSize = 0;
////////////////////////		auto *dataold = static_cast<uint8_t*>(static_cast<void*>(img->get_block_data(mipmap,xBlocks,yBlocks,blockSize,lineSize)));
////////////////////////		//assert(data != nullptr);
////////////////////////
/////////////////////////*
////////////////////////        DXTColBlock *top;
////////////////////////        DXTColBlock *bottom;
////////////////////////    
////////////////////////        for (unsigned int j = 0; j < (yblocks >> 1); j++)
////////////////////////        {
////////////////////////            top = (DXTColBlock*)((unsigned char*)surface+ j * linesize);
////////////////////////            bottom = (DXTColBlock*)((unsigned char*)surface + (((yblocks-j)-1) * linesize));
////////////////////////
////////////////////////            (this->*flipblocks)(top, xblocks);
////////////////////////            (this->*flipblocks)(bottom, xblocks);
////////////////////////
////////////////////////            swap(bottom, top, linesize);
////////////////////////        }
////////////////////////*/
////////////////////////		////////////auto ddsTest = gli::load_dds("C:\\Users\\Florian\\Documents\\Projects\\vulkan\\x64\\Debug\\materials\\deathclaw.dds");
////////////////////////		////////////gli::storage storage {ddsTest.format(),ddsTest.dimensions(),ddsTest.layers(),ddsTest.faces(),ddsTest.levels()};
////////////////////////		////////////blockSize = storage.block_size();
////////////////////////		////////////auto blockCount = storage.block_count(0);
////////////////////////		////////////auto *data = static_cast<uint8_t*>(ddsTest.data());
////////////////////////		////////////auto &mem = vkImg->GetMemory();
////////////////////////		////////////auto layout = vkImg->GetResourceLayout();
////////////////////////		////////////auto map = mem->MapMemory();
////////////////////////		////////////auto *ptr = static_cast<uint8_t*>(map->GetData());
////////////////////////		////////////ptr += layout.offset();
////////////////////////		////////////assert((w %4) == 0);
////////////////////////		////////////assert((h %4) == 0);
////////////////////////		////////////assert(blockSize == 8);
////////////////////////		////////////auto wBlocks = blockCount.x;
////////////////////////		////////////auto hBlocks = blockCount.y;
////////////////////////		////////////for(auto y=decltype(hBlocks){0};y<hBlocks;++y)
////////////////////////		////////////{
////////////////////////		//////////////for(auto y=hBlocks;y>0;)
////////////////////////		//////////////{
////////////////////////		////////////	//--y;
////////////////////////		////////////	auto *rowDest = ptr +y *layout.rowPitch();
////////////////////////		////////////	auto *rowSrc = data +y *(wBlocks *blockSize);
////////////////////////		////////////	for(auto x=decltype(wBlocks){0};x<wBlocks;++x)
////////////////////////		////////////	{
////////////////////////		////////////		auto *pxDest = rowDest +x *blockSize;
////////////////////////		////////////		auto *pxSrc = rowSrc +x *blockSize;
////////////////////////		////////////		memcpy(pxDest,pxSrc,blockSize);
////////////////////////		////////////		//memset(pxDest,255,blockSize);
////////////////////////		////////////	}
////////////////////////		////////////}
////////////////////////		////////////map = nullptr;
////////////////////////		//address(x,y,z,layer) = layer*arrayPitch + z*depthPitch + y*rowPitch + x*blockSize + offset;
////////////////////////		//size_t pos = 0;
////////////////////////		/*for(auto y=decltype(yBlocks){0};y<yBlocks *2;++y)
////////////////////////		{
////////////////////////			auto *row = ptr +y *layout.rowPitch();
////////////////////////			auto *line = data +(y /2) *lineSize;
////////////////////////			for(auto x=decltype(yBlocks){0};x<yBlocks *2;++x)
////////////////////////			{
////////////////////////				memset(row,128,blockSize);
////////////////////////				//auto *block = line +(x /2) *blockSize;
////////////////////////				//memcpy(row,block,blockSize);
////////////////////////
////////////////////////				row += blockSize;
////////////////////////			}
////////////////////////			//memcpy(row,line,lineSize *2);
////////////////////////		}*/
////////////////////////		//for(auto y=decltype(yBlocks){0};y<yBlocks;++y)
////////////////////////		//{
////////////////////////		//	auto *row = ptr +y *layout.rowPitch();
////////////////////////		//	auto *line = data +y *lineSize;
////////////////////////		//	memcpy(row,line,lineSize);
////////////////////////		//	/*for(auto x=decltype(xBlocks){0};x<xBlocks;++x)
////////////////////////		//	{
////////////////////////		//		//nv_dds::DXTColBlock *xx = static_cast<nv_dds::DXTColBlock*>(static_cast<void*>(line));
////////////////////////		//		//xx->
////////////////////////		//		auto *block = line +x *blockSize;
////////////////////////		//		auto *px = row;
////////////////////////		//		memcpy(px,block,blockSize);
////////////////////////		//		row += blockSize;
////////////////////////		//	}*/
////////////////////////		//	//ptr += layout.rowPitch();
////////////////////////		//}
////////////////////////		/*for(auto y=decltype(texture->height){0};y<texture->height;++y)
////////////////////////		{
////////////////////////			auto *row = ptr;
////////////////////////			for(auto x=decltype(texture->width){0};x<texture->width;++x)
////////////////////////			{
////////////////////////				auto *px = row;
////////////////////////				auto *data = static_cast<const uint8_t*>(static_cast<const void*>(*img));
////////////////////////				px[0] = data[pos];
////////////////////////				px[1] = data[pos +1];
////////////////////////				px[2] = data[pos +2];
////////////////////////				px[3] = data[pos +3];
////////////////////////				pos += 4;
////////////////////////				row += 4;
////////////////////////			}
////////////////////////			ptr += layout.rowPitch();
////////////////////////		*/
////////////////////////		//map = nullptr;
////////////////////////		/*for(auto y=decltype(gslot->bitmap.rows){0};y<gslot->bitmap.rows;++y)
////////////////////////		{
////////////////////////			auto *row = ptr;
////////////////////////			for(auto x=decltype(gslot->bitmap.width){0};x<gslot->bitmap.width;++x)
////////////////////////			{
////////////////////////				auto *px = row;
////////////////////////				px[0] = gslot->bitmap.buffer[pos];
////////////////////////				++pos;
////////////////////////				++row;
////////////////////////			}
////////////////////////			ptr += layout.rowPitch();
////////////////////////		}*/
////////////////////////		//mem->MapMemory(*img);
////////////////////////	}//
////////////////////////	catch(Vulkan::Exception &e)
////////////////////////	{
////////////////////////		throw e;
////////////////////////	}
////////////////////////	//auto vkImg = Vulkan::Image::Create(context,width,height,format);
////////////	
////////////#ifdef GRAPHICS_API_USE_OPENGL
////////////	unsigned int w = img->get_width();
////////////	unsigned int h = img->get_height();
////////////	unsigned int format = img->get_format();
////////////	unsigned int size = img->get_size();
////////////	glCompressedTexImage2D(target,0,format,w,h,0,size,*img);
////////////	int numMipmaps = img->get_num_mipmaps();
////////////	if((w &(w -1)) == 0 && (h &(h -1)) == 0) // Power of 2
////////////	{
////////////		for(int i=0;i<numMipmaps;i++)
////////////		{
////////////			const nv_dds::CSurface &mipmap = img->get_mipmap(i);
////////////			glCompressedTexImage2D(target,i +1,format,mipmap.get_width(),mipmap.get_height(),0,mipmap.get_size(),mipmap);
////////////		}
////////////	}
////////////	else // The nv_dds library seems to have problems with textures that don't have powers of 2 as resolutions (Mipmaps are incorrect)
////////////		glGenerateMipmap(target);
////////////	glTexParameteri(target,GL_TEXTURE_MAX_LEVEL,numMipmaps);
////////////#endif
////////////}

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
