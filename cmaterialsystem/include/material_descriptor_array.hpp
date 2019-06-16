#ifndef __MATERIAL_DESCRIPTOR_ARRAY_HPP__
#define __MATERIAL_DESCRIPTOR_ARRAY_HPP__

#include "cmatsysdefinitions.h"
#include <shader/prosper_descriptor_array_manager.hpp>
#include <buffers/prosper_buffer.hpp>

class Material;
class Texture;
namespace prosper {class Context; class UniformResizableBuffer;};
class DLLCMATSYS MaterialDescriptorArrayManager
	: public prosper::DescriptorArrayManager
{
public:
	#pragma pack(push,1)
	struct MaterialRenderInfoBufferData
	{
		ArrayIndex diffuseTextureArrayIndex = INVALID_ARRAY_INDEX; // Index into global texture array
		ArrayIndex specularTextureArrayIndex = INVALID_ARRAY_INDEX; // Index into global texture array
		ArrayIndex normalTextureArrayIndex = INVALID_ARRAY_INDEX; // Index into global texture array
		// TODO: Material parameters required for raytracing
	};
	#pragma pack(pop)

	using DescriptorArrayManager::DescriptorArrayManager;
	virtual ~MaterialDescriptorArrayManager() override;
	std::optional<prosper::Buffer::SubBufferIndex> RegisterMaterial(const Material &mat);
	const std::shared_ptr<prosper::UniformResizableBuffer> &GetMaterialInfoBuffer() const;
	friend DescriptorArrayManager;
private:
	struct TextureData
	{
		DescriptorArrayManager::ArrayIndex arrayIndex;
		CallbackHandle onRemoveCallback;
	};
	virtual void Initialize(prosper::Context &context) override;
	std::optional<ArrayIndex> AddItem(Texture &tex);
	void RemoveItem(const Texture &tex);
	std::unordered_map<const Texture*,TextureData> m_texData {};

	std::unordered_map<const Material*,std::shared_ptr<prosper::Buffer>> m_materialRenderBuffers = {};
	std::shared_ptr<prosper::UniformResizableBuffer> m_materialInfoBuffer = nullptr;
};

#endif
