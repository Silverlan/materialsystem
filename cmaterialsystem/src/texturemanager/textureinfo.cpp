/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "texturemanager/texture.h"
#include <image/prosper_texture.hpp>
#include <prosper_context.hpp>

Texture::Texture(prosper::IPrContext &context,std::shared_ptr<prosper::Texture> tex)
	: m_context{context},m_texture(tex)
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
	ClearVkTexture();
}

bool Texture::HasFlag(Flags flag) const {return umath::is_flag_set(m_flags,flag);}
bool Texture::IsIndexed() const {return (m_flags &Flags::Indexed) != Flags::None;}
bool Texture::IsLoaded() const {return (m_flags &Flags::Loaded) != Flags::None;}
bool Texture::IsError() const {return (m_flags &Flags::Error) != Flags::None;}

void Texture::SetName(const std::string &name) {m_name = name;}
const std::string &Texture::GetName() const {return m_name;}
uint32_t Texture::GetWidth() const {return m_texture ? m_texture->GetImage().GetWidth() : 0u;}
uint32_t Texture::GetHeight() const {return m_texture ? m_texture->GetImage().GetHeight() : 0u;}
const std::shared_ptr<prosper::Texture> &Texture::GetVkTexture() const {return m_texture;}
void Texture::SetVkTexture(prosper::Texture &texture) {SetVkTexture(texture.shared_from_this());}
void Texture::ClearVkTexture() {SetVkTexture(nullptr);}
void Texture::SetVkTexture(std::shared_ptr<prosper::Texture> texture)
{
	if(m_texture)
	{
		// Make sure the old texture/image/image view/sampler are kept alive until rendering is complete
		m_context.KeepResourceAliveUntilPresentationComplete(m_texture);
	}
	m_texture = texture;
	++m_updateCount;
	if(umath::is_flag_set(m_flags,Texture::Flags::Loaded))
		RunOnLoadedCallbacks();
}
bool Texture::HasValidVkTexture() const {return m_texture != nullptr;}

Texture::Flags Texture::GetFlags() const {return m_flags;}
void Texture::SetFlags(Flags flags) {m_flags = flags;}
void Texture::AddFlags(Flags flags) {m_flags |= flags;}

CallbackHandle Texture::CallOnLoaded(const CallbackHandle &callback)
{
	if(IsLoaded())
	{
		const_cast<CallbackHandle&>(callback)(shared_from_this());
		return {};
	}
	m_onLoadCallbacks.push(callback);
	return callback;
}
CallbackHandle Texture::CallOnLoaded(const std::function<void(std::shared_ptr<Texture>)> &callback)
{
	return CallOnLoaded(FunctionCallback<void,std::shared_ptr<Texture>>::Create(callback));
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
		auto callback = m_onLoadCallbacks.front();
		m_onLoadCallbacks.pop();
		callback(ptr);
	}
}

void Texture::Reset()
{
	if(m_name == "error") // Never remove error texture! TODO: Do this properly
		return;
	ClearVkTexture();
}
