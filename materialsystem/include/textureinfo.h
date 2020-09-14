/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __TEXTUREINFO_H__
#define __TEXTUREINFO_H__

#include "matsysdefinitions.h"
#include <datasystem.h>

#pragma warning(push)
#pragma warning(disable : 4251)
struct DLLMATSYS TextureInfo
{
	TextureInfo();
	TextureInfo(const TextureInfo &other);
	std::string name;
	unsigned int width;
	unsigned int height;
	std::shared_ptr<void> texture;
};

namespace ds
{
	class DLLMATSYS Texture
		: public Value
	{
	public:
		Texture(ds::Settings &dataSettings,const std::string &value);
		Texture(ds::Settings &dataSettings,const TextureInfo &value);
		virtual Texture *Copy() override;
		const TextureInfo &GetValue() const;
		TextureInfo &GetValue();

		virtual std::string GetString() const override;
		virtual std::string GetTypeString() const override;
		virtual int GetInt() const override;
		virtual float GetFloat() const override;
		virtual bool GetBool() const override;
		virtual ::Color GetColor() const override;
		virtual ::Vector3 GetVector() const override;
		virtual ::Vector4 GetVector4() const override;
	protected:
		Texture(ds::Settings &dataSettings,const std::string &value,bool bCubemap);
	private:
		TextureInfo m_value;
	};

	/*class DLLMATSYS Cubemap
		: public Texture
	{
	public:
		using Texture::Texture;
		Cubemap(ds::Settings &dataSettings,const std::string &value);
		virtual std::string GetTypeString() const override;
	};*/
};
#pragma warning(pop)

#endif
