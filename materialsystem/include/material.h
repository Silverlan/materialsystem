/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __MATERIAL_H__
#define __MATERIAL_H__

#include "matsysdefinitions.h"
#include "textureinfo.h"
#include <optional>
#include <sharedutils/util_path.hpp>
#include <sharedutils/util_weak_handle.hpp>
#include <sharedutils/def_handle.h>
#include <sharedutils/functioncallback.h>
#include <sharedutils/alpha_mode.hpp>
#include <sharedutils/util_debug.h>
#include <mathutil/umath.h>
#include <mathutil/uvec.h>
#include <datasystem_color.h>
#include <datasystem_vector.h>
#include <udm.hpp>

class Material;
namespace msys {
	using MaterialHandle = std::shared_ptr<Material>;
	class MaterialManager;

	template<typename T>
	concept is_property_type = (udm::is_udm_type<T>() && (!udm::is_non_trivial_type(udm::type_to_enum<T>()) || std::is_same_v<T, udm::String> || std::is_same_v<T, udm::Utf8String>)) || std::is_enum_v<T>;

	enum class PropertyType : uint8_t {
		None = 0,
		Value,
		Texture,
		Block,
		Count,
	};

	template<typename T>
	concept is_underlying_property_udm_type = std::is_same_v<T, udm::String> || std::is_same_v<T, udm::Int32> || std::is_same_v<T, udm::Float> || std::is_same_v<T, udm::Boolean> || std::is_same_v<T, udm::Vector3> || std::is_same_v<T, udm::Vector2> || std::is_same_v<T, udm::Vector4>;
	constexpr udm::Type to_udm_type(ds::ValueType type)
	{
		switch(type) {
		case ds::ValueType::String:
		case ds::ValueType::Texture:
			return udm::Type::String;
		case ds::ValueType::Int:
			return udm::Type::Int32;
		case ds::ValueType::Float:
			return udm::Type::Float;
		case ds::ValueType::Bool:
			return udm::Type::Boolean;
		case ds::ValueType::Color:
			return udm::Type::Vector3;
		case ds::ValueType::Vector2:
			return udm::Type::Vector2;
		case ds::ValueType::Vector3:
			return udm::Type::Vector3;
		case ds::ValueType::Vector4:
			return udm::Type::Vector4;
		default:
			return udm::Type::Invalid;
		}
		static_assert(umath::to_integral(ds::ValueType::Count) == 11, "Update this function when new types are added!");
	}

	constexpr ds::ValueType to_ds_type(udm::Type type)
	{
		switch(type) {
		case udm::Type::String:
		case udm::Type::Utf8String:
			return ds::ValueType::String;
		case udm::Type::Int8:
		case udm::Type::UInt8:
		case udm::Type::Int16:
		case udm::Type::UInt16:
		case udm::Type::Int32:
		case udm::Type::UInt32:
		case udm::Type::Int64:
		case udm::Type::UInt64:
			return ds::ValueType::Int;
		case udm::Type::Half:
		case udm::Type::Float:
		case udm::Type::Double:
			return ds::ValueType::Float;
		case udm::Type::Boolean:
			return ds::ValueType::Bool;
		case udm::Type::Vector2:
		case udm::Type::Vector2i:
			return ds::ValueType::Vector2;
		case udm::Type::Vector3:
		case udm::Type::Vector3i:
			return ds::ValueType::Vector3;
		case udm::Type::Vector4:
		case udm::Type::Vector4i:
			return ds::ValueType::Vector4;
		case udm::Type::Quaternion:
		case udm::Type::EulerAngles:
			return ds::ValueType::Vector3;
		case udm::Type::Srgba:
		case udm::Type::HdrColor:
			return ds::ValueType::Vector4;
		}
		static_assert(umath::to_integral(ds::ValueType::Count) == 11, "Update this function when new types are added!");
		return ds::ValueType::Invalid;
	}
};

using MaterialIndex = uint32_t;
namespace util {
	class ShaderInfo;
};
class VFilePtrInternalReal;
namespace ds {
	class Block;
};
namespace udm {
	struct AssetData;
};
#pragma warning(push)
#pragma warning(disable : 4251)
class DLLMATSYS Material : public std::enable_shared_from_this<Material> {
  public:
	static constexpr auto PMAT_IDENTIFIER = "PMAT";
	static constexpr uint32_t PMAT_VERSION = 1;

	static constexpr auto FORMAT_MATERIAL_BINARY = "pmat_b";
	static constexpr auto FORMAT_MATERIAL_ASCII = "pmat";
	static constexpr auto FORMAT_MATERIAL_LEGACY = "wmi";

	// inline static class-strings cause an internal compiler error with VS2019, so we can't use them for the time being
	// inline static const std::string DIFFUSE_MAP_IDENTIFIER = "diffuse_map";
	static const std::string DIFFUSE_MAP_IDENTIFIER;
	static const std::string ALBEDO_MAP_IDENTIFIER;
	static const std::string ALBEDO_MAP2_IDENTIFIER;
	static const std::string ALBEDO_MAP3_IDENTIFIER;
	static const std::string NORMAL_MAP_IDENTIFIER;
	static const std::string GLOW_MAP_IDENTIFIER;
	static const std::string EMISSION_MAP_IDENTIFIER;
	static const std::string PARALLAX_MAP_IDENTIFIER;
	static const std::string ALPHA_MAP_IDENTIFIER;
	static const std::string RMA_MAP_IDENTIFIER;
	static const std::string DUDV_MAP_IDENTIFIER;
	static const std::string WRINKLE_STRETCH_MAP_IDENTIFIER;
	static const std::string WRINKLE_COMPRESS_MAP_IDENTIFIER;
	static const std::string EXPONENT_MAP_IDENTIFIER;

	enum class StateFlags : uint32_t {
		None = 0u,
		Loaded = 1u,
		ExecutingOnLoadCallbacks = Loaded << 1u,
		Error = ExecutingOnLoadCallbacks << 1u,
		TexturesUpdated = Error << 1u,
	};

	enum class Event : uint8_t {
		OnTexturesUpdated = 0,
		Count,
	};

	static std::shared_ptr<Material> Create(msys::MaterialManager &manager);
	static std::shared_ptr<Material> Create(msys::MaterialManager &manager, const util::WeakHandle<util::ShaderInfo> &shaderInfo, const std::shared_ptr<ds::Block> &data);
	static std::shared_ptr<Material> Create(msys::MaterialManager &manager, const std::string &shader, const std::shared_ptr<ds::Block> &data);
	Material(const Material &) = delete;
	virtual ~Material();
	msys::MaterialHandle GetHandle();
	virtual void SetShaderInfo(const util::WeakHandle<util::ShaderInfo> &shaderInfo);
	const util::ShaderInfo *GetShaderInfo() const;
	void UpdateTextures(bool forceUpdate = false);
	const std::string &GetShaderIdentifier() const;
	void SetName(const std::string &name);
	const std::string &GetName();
	bool IsTranslucent() const;
	bool IsError() const;
	void SetErrorFlag(bool set);
	virtual TextureInfo *GetTextureInfo(const std::string_view &key);
	const TextureInfo *GetTextureInfo(const std::string_view &key) const;

	const Material *GetBaseMaterial() const;
	Material *GetBaseMaterial();
	void SetBaseMaterial(Material *baseMaterial);
	void SetBaseMaterial(const std::string &baseMaterial);

	const TextureInfo *GetDiffuseMap() const;
	TextureInfo *GetDiffuseMap();

	const TextureInfo *GetAlbedoMap() const;
	TextureInfo *GetAlbedoMap();

	const TextureInfo *GetNormalMap() const;
	TextureInfo *GetNormalMap();

	const TextureInfo *GetGlowMap() const;
	TextureInfo *GetGlowMap();

	const TextureInfo *GetAlphaMap() const;
	TextureInfo *GetAlphaMap();

	const TextureInfo *GetParallaxMap() const;
	TextureInfo *GetParallaxMap();

	const TextureInfo *GetRMAMap() const;
	TextureInfo *GetRMAMap();

	AlphaMode GetAlphaMode() const;
	float GetAlphaCutoff() const;

	void SetColorFactor(const Vector4 &colorFactor);
	Vector4 GetColorFactor() const;
	void SetBloomColorFactor(const Vector4 &bloomColorFactor);
	std::optional<Vector4> GetBloomColorFactor() const;

	const std::shared_ptr<ds::Block> &GetPropertyDataBlock() const { return m_data; }
	bool HasPropertyBlock(const std::string_view &name) const;
	std::shared_ptr<ds::Block> GetPropertyBlock(const std::string_view &path) const;
	msys::PropertyType GetPropertyType(const std::string_view &key) const;

	void SetTextureProperty(const std::string_view &strPath, const std::string_view &tex);

	void ClearProperty(const std::string_view &key, bool clearBlocksIfEmpty = true);
	template<typename T>
	    requires(msys::is_property_type<T>)
	void SetProperty(const std::string_view &key, const T &value);
	template<typename TTarget>
	    requires(msys::is_property_type<TTarget>)
	bool GetProperty(const std::string_view &key, TTarget *outValue) const;
	template<typename TTarget>
	    requires(msys::is_property_type<TTarget>)
	TTarget GetProperty(const std::string_view &key, const TTarget &defVal) const;
	ds::ValueType GetPropertyValueType(const std::string_view &strPath) const;

	std::pair<std::shared_ptr<ds::Block>, std::string> ResolvePropertyPath(const std::string_view &strPath) const;

	virtual void SetLoaded(bool b);
	CallbackHandle CallOnLoaded(const std::function<void(void)> &f) const;
	CallbackHandle AddEventListener(Event event, const std::function<void(void)> &f);
	bool IsValid() const;
	msys::MaterialManager &GetManager() const;
	std::optional<std::string> GetAbsolutePath() const;

	bool Save(udm::AssetData outData, std::string &outErr);
	bool Save(const std::string &fileName, std::string &outErr, bool absolutePath = false);
	bool Save(std::string &outErr);

	bool SaveLegacy(std::shared_ptr<VFilePtrInternalReal> f) const;
	bool SaveLegacy(const std::string &fileName, const std::string &rootPath = "") const;
	bool SaveLegacy() const;

	MaterialIndex GetIndex() const { return m_index; }
	uint32_t GetUpdateIndex() const { return m_updateIndex; }

	virtual void Assign(const Material &other);

	virtual std::shared_ptr<Material> Copy(bool copyData = true) const;

	// Returns true if all textures associated with this material have been fully loaded
	bool IsLoaded() const;
	void *GetUserData();
	void SetUserData(void *data);
	void *GetUserData2() { return m_userData2; }
	void SetUserData2(void *data) { m_userData2 = data; }
	virtual void Reset();
	void Initialize(const util::WeakHandle<util::ShaderInfo> &shaderInfo, const std::shared_ptr<ds::Block> &data);
	void Initialize(const std::string &shader, const std::shared_ptr<ds::Block> &data);
  protected:
	friend msys::MaterialManager;
	Material(msys::MaterialManager &manager);
	Material(msys::MaterialManager &manager, const util::WeakHandle<util::ShaderInfo> &shaderInfo, const std::shared_ptr<ds::Block> &data);
	Material(msys::MaterialManager &manager, const std::string &shader, const std::shared_ptr<ds::Block> &data);
	virtual void OnBaseMaterialChanged();
	void CallEventListeners(Event event);
	void UpdateBaseMaterial();

	void ClearProperty(ds::Block &block, const std::string_view &key, bool clearBlocksIfEmpty);
	template<typename T>
	    requires(msys::is_property_type<T>)
	void SetProperty(ds::Block &block, const std::string_view &key, const T &value);
	template<typename TTarget>
	    requires(msys::is_property_type<TTarget>)
	bool GetProperty(const ds::Block &block, const std::string_view &key, TTarget *outValue) const;
	template<typename TTarget>
	    requires(msys::is_property_type<TTarget>)
	TTarget GetProperty(const ds::Block &block, const std::string_view &key, const TTarget &defVal) const;

	virtual void Initialize(const std::shared_ptr<ds::Block> &data);
	virtual void OnTexturesUpdated();
	void SetIndex(MaterialIndex index) { m_index = index; }
	uint32_t m_updateIndex = 0;
	util::WeakHandle<util::ShaderInfo> m_shaderInfo = {};
	std::unique_ptr<std::string> m_shader;
	std::string m_name;
	std::shared_ptr<ds::Block> m_data;
	StateFlags m_stateFlags = StateFlags::None;
	mutable std::vector<CallbackHandle> m_callOnLoaded;
	std::unordered_map<Event, std::vector<CallbackHandle>> m_eventListeners;
	msys::MaterialManager &m_manager;
	TextureInfo *m_texDiffuse = nullptr;
	TextureInfo *m_texNormal = nullptr;
	TextureInfo *m_texGlow = nullptr;
	TextureInfo *m_texAlpha = nullptr;
	TextureInfo *m_texParallax = nullptr;
	TextureInfo *m_texRma = nullptr;
	void *m_userData;
	void *m_userData2 = nullptr;
	AlphaMode m_alphaMode = AlphaMode::Opaque;
	MaterialIndex m_index = std::numeric_limits<MaterialIndex>::max();

	struct BaseMaterial {
		~BaseMaterial();
		std::string name;
		std::shared_ptr<Material> material;
		CallbackHandle onBaseTexturesUpdated;
	};
	std::unique_ptr<BaseMaterial> m_baseMaterial;
};
REGISTER_BASIC_ARITHMETIC_OPERATORS(Material::StateFlags)
#pragma warning(pop)

DLLMATSYS std::ostream &operator<<(std::ostream &out, const Material &o);

template<typename T>
    requires(msys::is_property_type<T>)
void Material::SetProperty(const std::string_view &strPath, const T &value)
{
	auto [block, key] = ResolvePropertyPath(strPath);
	if(block == nullptr)
		return;
	SetProperty<T>(*block, key, value);
	return;
}
template<typename TTarget>
    requires(msys::is_property_type<TTarget>)
bool Material::GetProperty(const std::string_view &strPath, TTarget *outValue) const
{
	auto [block, key] = ResolvePropertyPath(strPath);
	if(block == nullptr)
		return false;
	if(GetProperty<TTarget>(*block, key, outValue))
		return true;
	auto *baseMaterial = GetBaseMaterial();
	if(baseMaterial)
		return baseMaterial->GetProperty(strPath, outValue);
	return false;
}
template<typename TTarget>
    requires(msys::is_property_type<TTarget>)
TTarget Material::GetProperty(const std::string_view &strPath, const TTarget &defVal) const
{
	TTarget val;
	if(GetProperty<TTarget>(strPath, &val))
		return val;
	return defVal;
}

template<typename T>
    requires(msys::is_property_type<T>)
void Material::SetProperty(ds::Block &block, const std::string_view &key, const T &value)
{
	if constexpr(std::is_same_v<T, Quat>)
		return SetProperty(block, key, EulerAngles {value});
	else {
		constexpr auto udmType = udm::type_to_enum<T>();
		constexpr auto dsType = msys::to_ds_type(udmType);
		constexpr auto normalizedUdmType = msys::to_udm_type(dsType);
		udm::visit(normalizedUdmType, [&](auto tag) {
			using TNorm = typename decltype(tag)::type;
			if constexpr(msys::is_underlying_property_udm_type<TNorm> && udm::is_convertible<T, TNorm>()) {
				auto normValue = udm::convert<T, TNorm>(value);
				block.AddValue(std::string {key}, normValue);
			}
		});
	}
}
template<typename TTarget>
    requires(msys::is_property_type<TTarget>)
bool Material::GetProperty(const ds::Block &block, const std::string_view &key, TTarget *outValue) const
{
	auto &dsBase = block.GetValue(key);
	if(dsBase == nullptr || !dsBase->IsValue())
		return false;
	auto &dsVal = *static_cast<ds::Value *>(dsBase.get());
	constexpr auto targetType = udm::type_to_enum<TTarget>();
	auto sourceType = msys::to_udm_type(dsVal.GetType());
	auto res = udm::visit(sourceType, [&](auto tag) -> bool {
		using TSource = typename decltype(tag)::type;
		if constexpr(msys::is_property_type<TSource>) {
			if constexpr(udm::is_convertible<TSource, TTarget>()) {
				if constexpr(std::is_same_v<TSource, udm::String>) {
					UTIL_ASSERT(dsVal.GetType() == ds::ValueType::String || dsVal.GetType() == ds::ValueType::Texture);
					*outValue = udm::convert<TSource, TTarget>(static_cast<ds::String *>(&dsVal)->GetValue());
				}
				else if constexpr(std::is_same_v<TSource, udm::Int32>) {
					UTIL_ASSERT(dsVal.GetType() == ds::ValueType::Int);
					*outValue = udm::convert<TSource, TTarget>(static_cast<ds::Int *>(&dsVal)->GetValue());
				}
				else if constexpr(std::is_same_v<TSource, udm::Float>) {
					UTIL_ASSERT(dsVal.GetType() == ds::ValueType::Float);
					*outValue = udm::convert<TSource, TTarget>(static_cast<ds::Float *>(&dsVal)->GetValue());
				}
				else if constexpr(std::is_same_v<TSource, udm::Boolean>) {
					UTIL_ASSERT(dsVal.GetType() == ds::ValueType::Bool);
					*outValue = udm::convert<TSource, TTarget>(static_cast<ds::Bool *>(&dsVal)->GetValue());
				}
				else if constexpr(std::is_same_v<TSource, udm::Vector3>) {
					auto dsValType = dsVal.GetType();
					UTIL_ASSERT(dsValType == ds::ValueType::Color || dsValType == ds::ValueType::Vector3);
					if(dsValType == ds::ValueType::Color)
						*outValue = udm::convert<TSource, TTarget>(static_cast<ds::Color *>(&dsVal)->GetValue().ToVector3());
					else
						*outValue = udm::convert<TSource, TTarget>(static_cast<ds::Vector *>(&dsVal)->GetValue());
				}
				else if constexpr(std::is_same_v<TSource, udm::Vector2>) {
					UTIL_ASSERT(dsVal.GetType() == ds::ValueType::Vector2);
					*outValue = udm::convert<TSource, TTarget>(static_cast<ds::Vector2 *>(&dsVal)->GetValue());
				}
				else if constexpr(std::is_same_v<TSource, udm::Vector4>) {
					UTIL_ASSERT(dsVal.GetType() == ds::ValueType::Vector4);
					*outValue = udm::convert<TSource, TTarget>(static_cast<ds::Vector4 *>(&dsVal)->GetValue());
				}
				else
					return false;
				return true;
			}
		}
		return false;
	});
	return res;
}
template<typename TTarget>
    requires(msys::is_property_type<TTarget>)
TTarget Material::GetProperty(const ds::Block &block, const std::string_view &key, const TTarget &defVal) const
{
	TTarget val;
	if(GetProperty<TTarget>(block, key, &val))
		return val;
	return defVal;
}

#endif
