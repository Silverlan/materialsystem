// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "definitions.hpp"

export module pragma.materialsystem:texture_info;

export import pragma.datasystem;

export {
	#pragma warning(push)
	#pragma warning(disable : 4251)
	struct DLLMATSYS TextureInfo {
		TextureInfo();
		TextureInfo(const TextureInfo &other);
		std::string name;
		unsigned int width;
		unsigned int height;
		std::shared_ptr<void> texture;
		std::shared_ptr<void> userData;
	};

	namespace ds {
		class DLLMATSYS Texture : public Value {
		public:
			Texture(ds::Settings &dataSettings, const std::string &value);
			Texture(ds::Settings &dataSettings, const TextureInfo &value);
			virtual Texture *Copy() override;
			const TextureInfo &GetValue() const;
			TextureInfo &GetValue();
			virtual ValueType GetType() const override;

			virtual std::string GetString() const override;
			virtual std::string GetTypeString() const override;
			virtual int GetInt() const override;
			virtual float GetFloat() const override;
			virtual bool GetBool() const override;
			virtual ::Color GetColor() const override;
			virtual ::Vector3 GetVector() const override;
			virtual ::Vector2 GetVector2() const override;
			virtual ::Vector4 GetVector4() const override;
		protected:
			Texture(ds::Settings &dataSettings, const std::string &value, bool bCubemap);
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
}
