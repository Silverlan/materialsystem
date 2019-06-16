/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "texturemanager/texture.h"
#include <image/prosper_texture.hpp>

Texture::Texture()
	: width(0),height(0),texture(nullptr),
	m_flags(Flags::Error)
{}

Texture::Texture(std::shared_ptr<prosper::Texture> &tex)
	: texture(tex),width((*tex->GetImage())->get_image_extent_2D(0u).width),height((*tex->GetImage())->get_image_extent_2D(0u).height)
{}

Texture::~Texture()
{
	while(m_onRemoveCallbacks.empty() == false)
	{
		auto &cb = m_onRemoveCallbacks.front();
		if(cb.IsValid())
			cb();
		m_onRemoveCallbacks.pop();
	}
}

bool Texture::IsIndexed() const {return (m_flags &Flags::Indexed) != Flags::None;}
bool Texture::IsLoaded() const {return (m_flags &Flags::Loaded) != Flags::None;}
bool Texture::IsError() const {return (m_flags &Flags::Error) != Flags::None;}
bool Texture::IsCubemap() const {return (m_flags &Flags::Cubemap) != Flags::None;}

Texture::Flags Texture::GetFlags() const {return m_flags;}
void Texture::SetFlags(Flags flags) {m_flags = flags;}

void Texture::CallOnLoaded(const std::function<void(std::shared_ptr<Texture>)> &callback)
{
	if(IsLoaded())
	{
		callback(shared_from_this());
		return;
	}
	m_onLoadCallbacks.push(callback);
}
CallbackHandle Texture::CallOnRemove(const std::function<void()> &callback)
{
	auto cb = FunctionCallback<void>::Create(callback);
	m_onRemoveCallbacks.push(cb);
	return cb;
}

void Texture::RunOnLoadedCallbacks()
{
	auto ptr = shared_from_this();
	while(!m_onLoadCallbacks.empty())
	{
		auto &callback = m_onLoadCallbacks.front();
		callback(ptr);
		m_onLoadCallbacks.pop();
	}
}

void Texture::Reset()
{
	if(name == "error") // Never remove error texture! TODO: Do this properly
		return;
	texture = nullptr;
}
