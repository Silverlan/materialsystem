// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <VTFFile.h>

module pragma.cmaterialsystem;

import :texture_manager.load_image_data;
import :texture_manager.manager;
#ifndef DISABLE_VTEX_SUPPORT
import source2;
#endif

pragma::fs::VFilePtr TextureManager::OpenTextureFile(const std::string &fpath)
{
	if(m_texFileHandler != nullptr) {
		auto f = m_texFileHandler(fpath);
		if(f != nullptr)
			return f;
	}
	return pragma::fs::open_file(fpath.c_str(), pragma::fs::FileMode::Read | pragma::fs::FileMode::Binary);
}

void TextureManager::InitializeTextureData(pragma::material::TextureQueueItem &item)
{
	item.valid = false;
	auto *surface = dynamic_cast<pragma::material::TextureQueueItemSurface *>(&item);
	if(surface != nullptr) {
		auto f = item.file;
		if(f == nullptr)
			f = OpenTextureFile(item.path);
		if(f == nullptr)
			item.valid = false;
		else {
			auto sz = f->GetSize();
			std::vector<uint8_t> data(sz);
			f->Read(data.data(), sz);
			auto tex = gli::load(static_cast<char *>(static_cast<void *>(data.data())), data.size());
			if(tex.empty())
				item.valid = false;
			else {
				surface->texture = std::make_unique<gli::texture2d>(tex);
				surface->valid = surface->texture != nullptr;
			}
		}
	}
	else {
		auto *png = dynamic_cast<pragma::material::TextureQueueItemPNG *>(&item);
		if(png != nullptr) {
			pragma::fs::File f {item.file};
			png->pnginfo = item.file ? pragma::image::load_image(f) : pragma::image::load_image(png->path.c_str());
			if(png->cubemap || png->pnginfo == nullptr)
				png->valid = false;
			else
				png->valid = true;
		}
		else {
			auto *tga = dynamic_cast<pragma::material::TextureQueueItemStbi *>(&item);
			if(tga != nullptr) {
				pragma::fs::File f {item.file};
				tga->imageBuffer = item.file ? pragma::image::load_image(f) : pragma::image::load_image(tga->path.c_str());
				if(tga->cubemap || tga->imageBuffer == nullptr)
					tga->valid = false;
				else
					tga->valid = true;
			}
			else {
#ifndef DISABLE_VTF_SUPPORT
				auto *vtf = dynamic_cast<pragma::material::TextureQueueItemVTF *>(&item);
				if(vtf != nullptr) {
					auto fp = vtf->file;
					if(fp == nullptr)
						fp = OpenTextureFile(vtf->path);
					if(fp == nullptr)
						vtf->valid = false;
					else {
						pragma::fs::File f {fp};

						vtf->texture = std::make_unique<VTFLib::CVTFFile>();
						vtf->valid = vtf->texture->Load(&f, false);
						if(vtf->valid == true) {
							auto format = vtf->texture->GetFormat();
							switch(format) {
							case IMAGE_FORMAT_DXT1:
							case IMAGE_FORMAT_DXT3:
							case IMAGE_FORMAT_DXT5:
							case IMAGE_FORMAT_RGB888:
							case IMAGE_FORMAT_RGBA8888:
							case IMAGE_FORMAT_BGR888:
							case IMAGE_FORMAT_BGRA8888:
							case IMAGE_FORMAT_UV88:
							case IMAGE_FORMAT_RGBA16161616F:
							case IMAGE_FORMAT_RGBA32323232F:
							case IMAGE_FORMAT_ABGR8888:
							case IMAGE_FORMAT_BGRX8888:
								break; // Note: When adding new formats, make sure to also add them to texture_initialize.cpp:vtf_format_to_vulkan_format
							default:
								vtf->valid = false; // Unsupported format
							}
						}
					}
				}
#endif
#ifndef DISABLE_VTEX_SUPPORT
				auto *vtex = dynamic_cast<pragma::material::TextureQueueItemVTex *>(&item);
				if(vtex != nullptr) {
					auto fp = vtex->file;
					if(fp == nullptr)
						fp = OpenTextureFile(vtex->path);
					if(fp == nullptr)
						vtex->valid = false;
					else {
						vtex->fptr = std::make_unique<pragma::fs::File>(fp);
						auto resource = source2::load_resource(*vtex->fptr);
						auto *dataBlock = resource ? resource->FindBlock(source2::BlockType::DATA) : nullptr;
						if(dataBlock) {
							auto *texBlock = dynamic_cast<source2::resource::Texture *>(dataBlock);
							if(texBlock) {
								vtex->texture = std::static_pointer_cast<source2::resource::Texture>(texBlock->shared_from_this());
								vtex->valid = true;
								auto format = vtex->texture->GetFormat();
								switch(format) {
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
				static_assert(pragma::math::to_integral(pragma::material::TextureType::Count) == 13, "Update this implementation when new texture types have been added!");
			}
		}
	}
}
