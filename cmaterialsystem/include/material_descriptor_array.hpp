// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __MATERIAL_DESCRIPTOR_ARRAY_HPP__
#define __MATERIAL_DESCRIPTOR_ARRAY_HPP__

#include "cmatsysdefinitions.h"
#include <shader/prosper_descriptor_array_manager.hpp>
#include <buffers/prosper_buffer.hpp>

class Material;
class Texture;
namespace prosper {
	class IPrContext;
	class IUniformResizableBuffer;
};
class DLLCMATSYS MaterialDescriptorArrayManager : public prosper::DescriptorArrayManager {
  public:
#pragma pack(push, 1)
	struct MaterialRenderInfoBufferData {
		// Indices into global texture array
		ArrayIndex albedoTextureArrayIndex = INVALID_ARRAY_INDEX;
		ArrayIndex normalTextureArrayIndex = INVALID_ARRAY_INDEX;
		ArrayIndex rmaTextureArrayIndex = INVALID_ARRAY_INDEX;

		std::array<ArrayIndex, 2> padding;
	};
#pragma pack(pop)

	using DescriptorArrayManager::DescriptorArrayManager;
	virtual ~MaterialDescriptorArrayManager() override;
	std::optional<prosper::IBuffer::SubBufferIndex> RegisterMaterial(const Material &mat, bool reInitialize = false);
	const std::shared_ptr<prosper::IUniformResizableBuffer> &GetMaterialInfoBuffer() const;
	friend DescriptorArrayManager;
  private:
	struct TextureData {
		DescriptorArrayManager::ArrayIndex arrayIndex;
		CallbackHandle onRemoveCallback;
	};
	virtual void Initialize(prosper::IPrContext &context) override;
	std::optional<ArrayIndex> AddItem(Texture &tex);
	void RemoveItem(const Texture &tex);
	std::unordered_map<const Texture *, TextureData> m_texData {};

	std::unordered_map<const Material *, std::shared_ptr<prosper::IBuffer>> m_materialRenderBuffers = {};
	std::shared_ptr<prosper::IUniformResizableBuffer> m_materialInfoBuffer = nullptr;
};

#endif
