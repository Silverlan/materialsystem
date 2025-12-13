// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

module pragma.cmaterialsystem;

import :texture_manager.texture;

pragma::material::Texture::Texture(prosper::IPrContext &context, std::shared_ptr<prosper::Texture> tex) : m_context {context}, m_texture(tex), m_fileFormatType {TextureType::Invalid} {}

pragma::material::Texture::~Texture()
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

void pragma::material::Texture::SetFileInfo(const pragma::util::Path &path, TextureType type)
{
	m_fileFormatType = type;
	m_filePath = path;
}

bool pragma::material::Texture::HasFlag(Flags flag) const { return pragma::math::is_flag_set(m_flags, flag); }
bool pragma::material::Texture::IsIndexed() const { return (m_flags & Flags::Indexed) != Flags::None; }
bool pragma::material::Texture::IsLoaded() const { return (m_flags & Flags::Loaded) != Flags::None; }
bool pragma::material::Texture::IsError() const { return (m_flags & Flags::Error) != Flags::None; }

void pragma::material::Texture::SetName(const std::string &name) { m_name = name; }
const std::string &pragma::material::Texture::GetName() const { return m_name; }
uint32_t pragma::material::Texture::GetWidth() const { return m_texture ? m_texture->GetImage().GetWidth() : 0u; }
uint32_t pragma::material::Texture::GetHeight() const { return m_texture ? m_texture->GetImage().GetHeight() : 0u; }
const std::shared_ptr<prosper::Texture> &pragma::material::Texture::GetVkTexture() const { return m_texture; }
void pragma::material::Texture::SetVkTexture(prosper::Texture &texture) { SetVkTexture(texture.shared_from_this()); }
void pragma::material::Texture::ClearVkTexture() { SetVkTexture(nullptr); }
void pragma::material::Texture::SetVkTexture(std::shared_ptr<prosper::Texture> texture)
{
	if(texture.get() == m_texture.get())
		return;
	if(m_texture) {
		// Make sure the old texture/image/image view/sampler are kept alive until rendering is complete
		m_context.KeepResourceAliveUntilPresentationComplete(m_texture);
	}
	m_texture = texture;
	++m_updateCount;
	if(pragma::math::is_flag_set(m_flags, Flags::Loaded))
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
bool pragma::material::Texture::HasValidVkTexture() const { return m_texture != nullptr; }

pragma::material::Texture::Flags pragma::material::Texture::GetFlags() const { return m_flags; }
void pragma::material::Texture::SetFlags(Flags flags) { m_flags = flags; }
void pragma::material::Texture::AddFlags(Flags flags) { m_flags |= flags; }

CallbackHandle pragma::material::Texture::CallOnLoaded(const CallbackHandle &callback)
{
	if(IsLoaded()) {
		const_cast<CallbackHandle &>(callback)(shared_from_this());
		return {};
	}
	m_onLoadCallbacks.push(callback);
	return callback;
}
CallbackHandle pragma::material::Texture::CallOnVkTextureChanged(const std::function<void()> &callback)
{
	auto cb = FunctionCallback<void>::Create(callback);
	m_onVkTextureChanged.push_back(cb);
	return cb;
}
CallbackHandle pragma::material::Texture::CallOnLoaded(const std::function<void(std::shared_ptr<Texture>)> &callback) { return CallOnLoaded(FunctionCallback<void, std::shared_ptr<Texture>>::Create(callback)); }
CallbackHandle pragma::material::Texture::CallOnRemove(const std::function<void()> &callback)
{
	auto cb = FunctionCallback<void>::Create(callback);
	m_onRemoveCallbacks.push(cb);
	return cb;
}

void pragma::material::Texture::RunOnLoadedCallbacks()
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

void pragma::material::Texture::Reset()
{
	if(m_name == "error") // Never remove error texture! TODO: Do this properly
		return;
	ClearVkTexture();
}
