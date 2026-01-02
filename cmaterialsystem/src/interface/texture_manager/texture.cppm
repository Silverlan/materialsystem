// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "util_enum_flags.hpp"

export module pragma.cmaterialsystem:texture_manager.texture;

export import pragma.materialsystem;
export import pragma.prosper;

export {
	class TextureManager;
	namespace pragma::material {
#pragma warning(push)
#pragma warning(disable : 4251)
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
			const std::optional<pragma::util::Path> &GetFilePath() { return m_filePath; }
			void SetFileInfo(const pragma::util::Path &path, TextureType type);

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
			std::optional<pragma::util::Path> m_filePath {};
			prosper::IPrContext &m_context;
			std::shared_ptr<prosper::Texture> m_texture = nullptr;
			std::string m_name;
			uint32_t m_updateCount = 0;
		};
#pragma warning(pop)
		using namespace pragma::math::scoped_enum::bitwise;
	}
	REGISTER_ENUM_FLAGS(pragma::material::Texture::Flags)
}
