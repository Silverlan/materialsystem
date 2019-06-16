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
class DLLCMATSYS Texture
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
		Cubemap = Error<<1u
	};
	Texture();
	~Texture();
	std::shared_ptr<prosper::Texture> texture = nullptr;
	Texture(std::shared_ptr<prosper::Texture> &texture);
	Anvil::Format internalFormat = Anvil::Format::UNKNOWN;
	std::string name;
	unsigned int width;
	unsigned int height;
	void Reset();
	void CallOnLoaded(const std::function<void(std::shared_ptr<Texture>)> &callback);
	CallbackHandle CallOnRemove(const std::function<void()> &callback);
	void RunOnLoadedCallbacks();

	bool IsIndexed() const;
	bool IsLoaded() const;
	bool IsError() const;
	bool IsCubemap() const;
private:
	Flags GetFlags() const;
	void SetFlags(Flags flags);
	std::queue<std::function<void(std::shared_ptr<Texture>)>> m_onLoadCallbacks;
	std::queue<CallbackHandle> m_onRemoveCallbacks;
	Flags m_flags = Flags::None;
};
REGISTER_BASIC_BITWISE_OPERATORS(Texture::Flags);
#pragma warning(pop)

#endif