// SPDX-FileCopyrightText: © 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __TEXTURE_H__
#define __TEXTURE_H__

#include "cmatsysdefinitions.h"
#include <image/prosper_texture.hpp>
#include <sharedutils/functioncallback.h>
#include <sharedutils/util_path.hpp>
#include <queue>

class TextureManager;
#pragma warning(push)
#pragma warning(disable : 4251)
enum class TextureType : uint32_t;
class DLLCMATSYS Texture final : public std::enable_shared_from_this<Texture> {
  public:
	friend TextureManager;
	enum class Flags : uint32_t { None = 0u, Indexed = 1u, Loaded = Indexed << 1u, Error = Loaded << 1u, SRGB = Error << 1u, NormalMap = SRGB << 1u };
	Texture(prosper::IPrContext &context, std::shared_ptr<prosper::Texture> texture = nullptr);
	~Texture();
	void Reset();
	CallbackHandle CallOnLoaded(const std::function<void(std::shared_ptr<Texture>)> &callback);
	CallbackHandle CallOnLoaded(const CallbackHandle &callback);
	CallbackHandle CallOnVkTextureChanged(const std::function<void()> &callback);
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

	uint32_t GetUpdateCount() const { return m_updateCount; }

	TextureType GetFileFormatType() const { return m_fileFormatType; }
	const std::optional<util::Path> &GetFilePath() { return m_filePath; }
	void SetFileInfo(const util::Path &path, TextureType type);

	bool HasFlag(Flags flag) const;
	bool IsIndexed() const;
	bool IsLoaded() const;
	bool IsError() const;
	void AddFlags(Flags flags);
	Flags GetFlags() const;
	void SetFlags(Flags flags);
  private:
	std::vector<CallbackHandle> m_onVkTextureChanged;
	std::queue<CallbackHandle> m_onLoadCallbacks;
	std::queue<CallbackHandle> m_onRemoveCallbacks;
	Flags m_flags = Flags::Error;
	TextureType m_fileFormatType;
	std::optional<util::Path> m_filePath {};
	prosper::IPrContext &m_context;
	std::shared_ptr<prosper::Texture> m_texture = nullptr;
	std::string m_name;
	uint32_t m_updateCount = 0;
};
REGISTER_BASIC_BITWISE_OPERATORS(Texture::Flags);
#pragma warning(pop)

#endif
