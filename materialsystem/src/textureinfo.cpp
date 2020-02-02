/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "textureinfo.h"
#include "impl_texture_formats.h"
#include <util_image.h>
#include <sharedutils/util_file.h>
#include <cstring>

static bool read_image_size(std::string &imgFile,uint32_t &width,uint32_t &height)
{
	TextureType type;
	imgFile = translate_image_path(imgFile,type);
	auto r = uimg::read_image_size(imgFile,width,height);
	auto rootPath = MaterialManager::GetRootMaterialLocation() +"/";
	imgFile = imgFile.substr(rootPath.length());
	return r;
}

ds::Texture::Texture(ds::Settings &dataSettings,const std::string &value,bool bCubemap)
	: Value(dataSettings)
{
	auto str = value;
	if(str.empty())
	{
		m_value.texture = nullptr;
		m_value.width = 0;
		m_value.height = 0;
		return;
	}
	uint32_t width;
	uint32_t height;
	if(read_image_size(str,width,height) == true)
	{
		m_value.texture = NULL;
		m_value.width = width;
		m_value.height = height;
	}
	else
	{
		m_value.texture = NULL;
		m_value.width = 0;
		m_value.height = 0;
	}
	m_value.name = str;
}
ds::Texture::Texture(ds::Settings &dataSettings,const std::string &value)
	: Texture(dataSettings,value,false)
{}
ds::Texture::Texture(ds::Settings &dataSettings,const TextureInfo &value)
	: Value(dataSettings),m_value(value)
{}
ds::Texture *ds::Texture::Copy() {return new ds::Texture(*m_dataSettings,m_value);}
const TextureInfo &ds::Texture::GetValue() const {return const_cast<Texture*>(this)->GetValue();}
TextureInfo &ds::Texture::GetValue() {return m_value;}

std::string ds::Texture::GetString() const
{
	auto name = m_value.name;
	ustring::replace(name,"\\","/");
	ufile::remove_extension_from_filename(name); // TODO: Allow manual extension if it was specified explicitly?
	return name;
}
int ds::Texture::GetInt() const {return 0;}
float ds::Texture::GetFloat() const {return 0.f;}
bool ds::Texture::GetBool() const {return true;}
REGISTER_DATA_TYPE(ds::Texture,texture)

/*ds::Cubemap::Cubemap(ds::Settings &dataSettings,const std::string &value)
	: Texture(dataSettings,value,true)
{}
REGISTER_DATA_TYPE(ds::Cubemap,cubemap)*/

///////////////////////////

TextureInfo::TextureInfo()
	: name(),width(0),height(0),texture(nullptr)
{}

TextureInfo::TextureInfo(const TextureInfo &other)
	: name(other.name),width(other.width),height(other.height),texture(other.texture)
{}
