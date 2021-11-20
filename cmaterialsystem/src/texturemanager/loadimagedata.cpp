/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "texturemanager/texturemanager.h"
#include "texturemanager/loadimagedata.h"
#include <fsys/filesystem.h>
#include <fsys/ifile.hpp>
#include "texturemanager/texturequeue.h"
#include "virtualfile.h"
#include <util_image.hpp>
#ifndef DISABLE_VTEX_SUPPORT
#include <source2/resource.hpp>
#include <source2/resource_data.hpp>
#endif

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
	item.valid = false;
	auto *surface = dynamic_cast<TextureQueueItemSurface*>(&item);
	if(surface != nullptr)
	{
		auto f = item.file;
		if(f == nullptr)
			f = OpenTextureFile(item.path);
		if(f == nullptr)
			item.valid = false;
		else
		{
			auto sz = f->GetSize();
			std::vector<uint8_t> data(sz);
			f->Read(data.data(),sz);
			auto tex = gli::load(static_cast<char*>(static_cast<void*>(data.data())),data.size());
			if(tex.empty())
				item.valid = false;
			else
			{
				surface->texture = std::make_unique<gli::texture2d>(tex);
				surface->valid = surface->texture != nullptr;
			}
		}
	}
	else
	{
		auto *png = dynamic_cast<TextureQueueItemPNG*>(&item);
		if(png != nullptr)
		{
			fsys::File f {item.file};
			png->pnginfo = item.file ? uimg::load_image(f) : uimg::load_image(png->path.c_str());
			if(png->cubemap || png->pnginfo == nullptr)
				png->valid = false;
			else
				png->valid = true;
		}
		else
		{
			auto *tga = dynamic_cast<TextureQueueItemStbi*>(&item);
			if(tga != nullptr)
			{
				fsys::File f {item.file};
				tga->imageBuffer = item.file ? uimg::load_image(f) : uimg::load_image(tga->path.c_str());
				if(tga->cubemap || tga->imageBuffer == nullptr)
					tga->valid = false;
				else
					tga->valid = true;
			}
			else
			{
#ifndef DISABLE_VTF_SUPPORT
				auto *vtf = dynamic_cast<TextureQueueItemVTF*>(&item);
				if(vtf != nullptr)
				{
					auto fp = vtf->file;
					if(fp == nullptr)
						fp = OpenTextureFile(vtf->path);
					if(fp == nullptr)
						vtf->valid = false;
					else
					{
						VirtualFile f{};
						auto &data = f.GetData();
						data.resize(fp->GetSize());
						fp->Read(data.data(),data.size());
						fp = nullptr;
						vtf->texture = std::make_unique<VTFLib::CVTFFile>();
						vtf->valid = vtf->texture->Load(&f,false);
						if(vtf->valid == true)
						{
							auto format = vtf->texture->GetFormat();
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
								case VTFImageFormat::IMAGE_FORMAT_ABGR8888:
								case VTFImageFormat::IMAGE_FORMAT_BGRX8888:
									break; // Note: When adding new formats, make sure to also add them to texture_initialize.cpp:vtf_format_to_vulkan_format
								default:
									vtf->valid = false; // Unsupported format
							}
						}
					}
				}
#endif
#ifndef DISABLE_VTEX_SUPPORT
				auto *vtex = dynamic_cast<TextureQueueItemVTex*>(&item);
				if(vtex != nullptr)
				{
					auto fp = vtex->file;
					if(fp == nullptr)
						fp = OpenTextureFile(vtex->path);
					if(fp == nullptr)
						vtex->valid = false;
					else
					{
						fsys::File f {fp};
						auto resource = source2::load_resource(f);
						auto *dataBlock = resource ? resource->FindBlock(source2::BlockType::DATA) : nullptr;
						if(dataBlock)
						{
							auto *texBlock = dynamic_cast<source2::resource::Texture*>(dataBlock);
							if(texBlock)
							{
								vtex->texture = std::static_pointer_cast<source2::resource::Texture>(texBlock->shared_from_this());
								vtex->valid = true;
								auto format = vtex->texture->GetFormat();
								switch(format)
								{
								case source2::VTexFormat::DXT1:
								case source2::VTexFormat::DXT5:
								case source2::VTexFormat::RGBA8888:
								case source2::VTexFormat::RGBA16161616:
								case source2::VTexFormat::RGBA16161616F:
								case source2::VTexFormat::RGB323232F:
								case source2::VTexFormat::RGBA32323232F:
								case source2::VTexFormat::BC6H:
								case source2::VTexFormat::BC7:
								case source2::VTexFormat::BGRA8888:
								case source2::VTexFormat::ATI1N:
								case source2::VTexFormat::ATI2N:
									break; // Note: When adding new formats, make sure to also add them to texture_initialize.cpp:vtex_format_to_vulkan_format
								default:
									vtex->valid = false; // Unsupported format
								}
							}
						}
					}
				}
#endif
				static_assert(umath::to_integral(TextureType::Count) == 13,"Update this implementation when new texture types have been added!");
			}
		}
	}
}
