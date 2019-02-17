/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "texturemanager/texturemanager.h"
#include "texturemanager/loadimagedata.h"
#include <fsys/filesystem.h>
#include "texturemanager/texturequeue.h"
#include "virtualfile.h"
#include <util_image.h>

VFilePtr TextureManager::OpenTextureFile(const std::string &fpath)
{
	if(m_texFileHandler != nullptr)
	{
		auto f = m_texFileHandler(fpath);
		if(f != nullptr)
			return f;
	}
	return FileManager::OpenFile(fpath.c_str(),"rb");
}

void TextureManager::InitializeTextureData(TextureQueueItem &item)
{
	const std::array<std::string,6> cubemapPostfixes = {"rt","lf","up","dn","bk","ft"};
	auto *surface = dynamic_cast<TextureQueueItemSurface*>(&item);
	if(surface != nullptr)
	{
		if(surface->cubemap == false)
		{
			auto f = OpenTextureFile(item.path);
			if(f == nullptr)
				item.valid = false;
			else
			{
				auto sz = f->GetSize();
				std::vector<uint8_t> data(sz);
				f->Read(data.data(),sz);
				surface->textures.push_back(std::make_unique<gli::texture2d>(gli::load(static_cast<char*>(static_cast<void*>(data.data())),data.size())));
				if(surface->textures.front()->empty())
				{
					surface->valid = false;
					surface->textures.clear();
				}
				else
					surface->valid = true;
			}
		}
		else
		{
			surface->textures.reserve(cubemapPostfixes.size());
			surface->valid = true;
			for(auto &p : cubemapPostfixes)
			{
				auto path = item.path +p;
				auto pathKtx = path +".ktx";
				auto f = OpenTextureFile(pathKtx);
				if(f == nullptr)
				{
					auto pathDds = path +".dds";
					f = OpenTextureFile(pathDds);
				}
				if(f == nullptr)
				{
					item.valid = false;
					surface->textures.clear();
					break;
				}
				else
				{
					auto sz = f->GetSize();
					std::vector<uint8_t> data(sz);
					f->Read(data.data(),sz);
					surface->textures.push_back(std::make_unique<gli::texture2d>(gli::load(static_cast<char*>(static_cast<void*>(data.data())),data.size())));
					if(surface->textures.front()->empty())
					{
						surface->valid = false;
						surface->textures.clear();
						break;
					}
				}
			}
		}
	}
	else
	{
		auto *png = dynamic_cast<TextureQueueItemPNG*>(&item);
		if(png != nullptr)
		{
			png->pnginfo = uimg::load_image(png->path.c_str());
			if(png->cubemap || png->pnginfo == nullptr)
				png->valid = false;
			else
				png->valid = true;
		}
		else
		{
			auto *tga = dynamic_cast<TextureQueueItemTGA*>(&item);
			if(tga != nullptr)
			{
				tga->tgainfo = uimg::load_image(tga->path.c_str());
				if(tga->cubemap || tga->tgainfo == nullptr)
					tga->valid = false;
				else
					tga->valid = true;
			}
			else
#ifdef ENABLE_VTF_SUPPORT
			{
				auto *vtf = dynamic_cast<TextureQueueItemVTF*>(&item);
/*
#ifdef ENABLE_VTF_SUPPORT
				if(f == nullptr)
				{
					auto pathVtf = path +".vtf";
					f = OpenTextureFile(pathVtf);
				}
#endif
*/
				if(vtf != nullptr)
				{
					if(item.cubemap == false)
					{
						auto fp = OpenTextureFile(vtf->path);
						if(fp == nullptr)
							vtf->valid = false;
						else
						{
							VirtualFile f{};
							auto &data = f.GetData();
							data.resize(fp->GetSize());
							fp->Read(data.data(),data.size());
							fp = nullptr;
							auto &vtfFile = std::make_shared<VTFLib::CVTFFile>();
							vtf->textures.push_back(vtfFile);
							vtf->valid = vtfFile->Load(&f,false);
							if(vtf->valid == true)
							{
								auto format = vtfFile->GetFormat();
								switch(format)
								{
									case VTFImageFormat::IMAGE_FORMAT_DXT1:
									case VTFImageFormat::IMAGE_FORMAT_DXT3:
									case VTFImageFormat::IMAGE_FORMAT_DXT5:
									case VTFImageFormat::IMAGE_FORMAT_RGB888:
									case VTFImageFormat::IMAGE_FORMAT_RGBA8888:
									case VTFImageFormat::IMAGE_FORMAT_BGR888:
									case VTFImageFormat::IMAGE_FORMAT_BGRA8888:
									case VTFImageFormat::IMAGE_FORMAT_UV88:
									case VTFImageFormat::IMAGE_FORMAT_RGBA16161616F:
									case VTFImageFormat::IMAGE_FORMAT_RGBA32323232F:
										break;
									default:
										vtf->valid = false; // Unsupported format
								}
							}
						}
					}
					else
					{
						vtf->textures.reserve(cubemapPostfixes.size());
						vtf->valid = true;
						for(auto &p : cubemapPostfixes)
						{
							auto path = item.path +p;
							auto pathVtf = path +".vtf";
							auto fp = OpenTextureFile(pathVtf);
							if(fp == nullptr)
							{
								vtf->valid = false;
								break;
							}
							VirtualFile f{};
							auto &data = f.GetData();
							data.resize(fp->GetSize());
							fp->Read(data.data(),data.size());
							fp = nullptr;
							auto &vtfFile = std::make_shared<VTFLib::CVTFFile>();
							vtf->textures.push_back(vtfFile);
							vtf->valid = vtfFile->Load(&f,false);
							if(vtf->valid == true)
							{
								auto format = vtfFile->GetFormat();
								switch(format)
								{
									case VTFImageFormat::IMAGE_FORMAT_DXT1:
									case VTFImageFormat::IMAGE_FORMAT_DXT3:
									case VTFImageFormat::IMAGE_FORMAT_DXT5:
									case VTFImageFormat::IMAGE_FORMAT_RGB888:
									case VTFImageFormat::IMAGE_FORMAT_RGBA8888:
									case VTFImageFormat::IMAGE_FORMAT_BGR888:
									case VTFImageFormat::IMAGE_FORMAT_BGRA8888:
									case VTFImageFormat::IMAGE_FORMAT_UV88:
									case VTFImageFormat::IMAGE_FORMAT_RGBA16161616F:
									case VTFImageFormat::IMAGE_FORMAT_RGBA32323232F:
										break;
									default:
										vtf->valid = false; // Unsupported format
								}
							}
							if(vtf->valid == false)
								break;
						}
					}
				}
				else
					item.valid = false;
			}
#else
				item->valid = false;
#endif
		}
	}
}
