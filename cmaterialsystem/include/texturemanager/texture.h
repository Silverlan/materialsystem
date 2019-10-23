/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include "cmatsysdefinitions.h"
#include <image/prosper_texture.hpp>
#include <sharedutils/functioncallback.h>
#include <queue>

class TextureManager;
#pragma warning(push)
#pragma warning(disable : 4251)
class DLLCMATSYS Texture final
	: public std::enable_shared_from_this<Texture>
{
public:
	friend TextureManager;
	enum class Flags : uint32_t
	{
		None = 0u,
		Indexed = 1u,
		Loaded = Indexed<<1u,
		Error = Loaded<<1u,
		SRGB = Error<<1u,
		NormalMap = SRGB<<1u
	};
	Texture(prosper::Context &context,std::shared_ptr<prosper::Texture> texture=nullptr);
	~Texture();
	void Reset();
	void CallOnLoaded(const std::function<void(std::shared_ptr<Texture>)> &callback);
	CallbackHandle CallOnRemove(const std::function<void()> &callback);
	void RunOnLoadedCallbacks();

	void SetName(const std::string &name);
	const std::string &GetName() const;
	uint32_t GetWidth() const;
	uint32_t GetHeight() const;
	const std::shared_ptr<prosper::Texture> &GetVkTexture() const;
	void SetVkTexture(prosper::Texture &texture);
	void SetVkTexture(std::shared_ptr<prosper::Texture> texture);
	void ClearVkTexture();
	bool HasValidVkTexture() const;

	bool HasFlag(Flags flag) const;
	bool IsIndexed() const;
	bool IsLoaded() const;
	bool IsError() const;
	void AddFlags(Flags flags);
	Flags GetFlags() const;
	void SetFlags(Flags flags);
private:

	std::queue<std::function<void(std::shared_ptr<Texture>)>> m_onLoadCallbacks;
	std::queue<CallbackHandle> m_onRemoveCallbacks;
	Flags m_flags = Flags::Error;
	prosper::Context &m_context;
	std::shared_ptr<prosper::Texture> m_texture = nullptr;
	std::string m_name;
};
REGISTER_BASIC_BITWISE_OPERATORS(Texture::Flags);
#pragma warning(pop)

#endif