// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.materialsystem;

import :texture_info;
import pragma.image;

static bool read_image_size(std::string &imgFile, uint32_t &width, uint32_t &height)
{
	pragma::material::TextureType type;
	imgFile = translate_image_path(imgFile, type);
	auto r = pragma::image::read_image_size(imgFile, width, height);
	auto rootPath = MaterialManager::GetRootMaterialLocation() + "/";
	imgFile = imgFile.substr(rootPath.length());
	return r;
}

pragma::datasystem::Texture::Texture(Settings &dataSettings, const std::string &value, bool bCubemap) : Value(dataSettings)
{
	auto str = value;
	if(str.empty()) {
		m_value.texture = nullptr;
		m_value.width = 0;
		m_value.height = 0;
		return;
	}
	uint32_t width;
	uint32_t height;
	if(read_image_size(str, width, height) == true) {
		m_value.texture = nullptr;
		m_value.width = width;
		m_value.height = height;
	}
	else {
		m_value.texture = nullptr;
		m_value.width = 0;
		m_value.height = 0;
	}
	m_value.name = str;
}
pragma::datasystem::Texture::Texture(Settings &dataSettings, const std::string &value) : Texture(dataSettings, value, false) {}
pragma::datasystem::Texture::Texture(Settings &dataSettings, const TextureInfo &value) : Value(dataSettings), m_value(value) {}
pragma::datasystem::Texture *pragma::datasystem::Texture::Copy() { return new Texture(*m_dataSettings, m_value); }
const TextureInfo &pragma::datasystem::Texture::GetValue() const { return const_cast<Texture *>(this)->GetValue(); }
TextureInfo &pragma::datasystem::Texture::GetValue() { return m_value; }
pragma::datasystem::ValueType pragma::datasystem::Texture::GetType() const { return ValueType::Texture; }

std::string pragma::datasystem::Texture::GetString() const
{
	auto name = m_value.name;
	pragma::string::replace(name, "\\", "/");
	ufile::remove_extension_from_filename(name); // TODO: Allow manual extension if it was specified explicitly?
	return name;
}
int pragma::datasystem::Texture::GetInt() const { return 0; }
float pragma::datasystem::Texture::GetFloat() const { return 0.f; }
bool pragma::datasystem::Texture::GetBool() const { return true; }
Color pragma::datasystem::Texture::GetColor() const { return {}; }
Vector3 pragma::datasystem::Texture::GetVector() const { return {}; }
Vector2 pragma::datasystem::Texture::GetVector2() const { return {}; }
Vector4 pragma::datasystem::Texture::GetVector4() const { return {}; }
void pragma::datasystem::Texture::register_type() { pragma::datasystem::register_data_value_type<Texture>("texture"); }
std::string pragma::datasystem::Texture::GetTypeString() const { return "texture"; }

/*pragma::datasystem::Cubemap::Cubemap(pragma::datasystem::Settings &dataSettings,const std::string &value)
	: Texture(dataSettings,value,true)
{}
REGISTER_DATA_TYPE(pragma::datasystem::Cubemap,cubemap)*/

///////////////////////////

TextureInfo::TextureInfo() : name(), width(0), height(0), texture(nullptr) {}

TextureInfo::TextureInfo(const TextureInfo &other) : name(other.name), width(other.width), height(other.height), texture(other.texture) {}
