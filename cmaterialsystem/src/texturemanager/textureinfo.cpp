// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#include "texturemanager/texture.h"
#include "texture_type.h"
#include <image/prosper_texture.hpp>
#include <prosper_context.hpp>

Texture::Texture(prosper::IPrContext &context, std::shared_ptr<prosper::Texture> tex) : m_context {context}, m_texture(tex), m_fileFormatType {TextureType::Invalid} {}

Texture::~Texture()
{
	m_onLoadCallbacks = {};
	while(m_onRemoveCallbacks.empty() == false) {
		auto &cb = m_onRemoveCallbacks.front();
		if(cb.IsValid())
			cb();
		m_onRemoveCallbacks.pop();
	}
	ClearVkTexture();
}

void Texture::SetFileInfo(const util::Path &path, TextureType type)
{
	m_fileFormatType = type;
	m_filePath = path;
}

bool Texture::HasFlag(Flags flag) const { return umath::is_flag_set(m_flags, flag); }
bool Texture::IsIndexed() const { return (m_flags & Flags::Indexed) != Flags::None; }
bool Texture::IsLoaded() const { return (m_flags & Flags::Loaded) != Flags::None; }
bool Texture::IsError() const { return (m_flags & Flags::Error) != Flags::None; }

void Texture::SetName(const std::string &name) { m_name = name; }
const std::string &Texture::GetName() const { return m_name; }
uint32_t Texture::GetWidth() const { return m_texture ? m_texture->GetImage().GetWidth() : 0u; }
uint32_t Texture::GetHeight() const { return m_texture ? m_texture->GetImage().GetHeight() : 0u; }
const std::shared_ptr<prosper::Texture> &Texture::GetVkTexture() const { return m_texture; }
void Texture::SetVkTexture(prosper::Texture &texture) { SetVkTexture(texture.shared_from_this()); }
void Texture::ClearVkTexture() { SetVkTexture(nullptr); }
void Texture::SetVkTexture(std::shared_ptr<prosper::Texture> texture)
{
	if(texture.get() == m_texture.get())
		return;
	if(m_texture) {
		// Make sure the old texture/image/image view/sampler are kept alive until rendering is complete
		m_context.KeepResourceAliveUntilPresentationComplete(m_texture);
	}
	m_texture = texture;
	++m_updateCount;
	if(umath::is_flag_set(m_flags, Texture::Flags::Loaded))
		RunOnLoadedCallbacks();
	for(auto it = m_onVkTextureChanged.begin(); it != m_onVkTextureChanged.end();) {
		auto &cb = *it;
		if(!cb.IsValid()) {
			it = m_onVkTextureChanged.erase(it);
			continue;
		}
		cb();
		++it;
	}
}
bool Texture::HasValidVkTexture() const { return m_texture != nullptr; }

Texture::Flags Texture::GetFlags() const { return m_flags; }
void Texture::SetFlags(Flags flags) { m_flags = flags; }
void Texture::AddFlags(Flags flags) { m_flags |= flags; }

CallbackHandle Texture::CallOnLoaded(const CallbackHandle &callback)
{
	if(IsLoaded()) {
		const_cast<CallbackHandle &>(callback)(shared_from_this());
		return {};
	}
	m_onLoadCallbacks.push(callback);
	return callback;
}
CallbackHandle Texture::CallOnVkTextureChanged(const std::function<void()> &callback)
{
	auto cb = FunctionCallback<void>::Create(callback);
	m_onVkTextureChanged.push_back(cb);
	return cb;
}
CallbackHandle Texture::CallOnLoaded(const std::function<void(std::shared_ptr<Texture>)> &callback) { return CallOnLoaded(FunctionCallback<void, std::shared_ptr<Texture>>::Create(callback)); }
CallbackHandle Texture::CallOnRemove(const std::function<void()> &callback)
{
	auto cb = FunctionCallback<void>::Create(callback);
	m_onRemoveCallbacks.push(cb);
	return cb;
}

void Texture::RunOnLoadedCallbacks()
{
	if(m_onLoadCallbacks.empty())
		return;
	auto ptr = shared_from_this();
	while(!m_onLoadCallbacks.empty()) {
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
