/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "material_descriptor_array.hpp"
#include "textureinfo.h"
#include "texturemanager/texture.h"
#include "cmaterial.h"
#include <buffers/prosper_uniform_resizable_buffer.hpp>
#include <prosper_util.hpp>
#include <datasystem.h>

#pragma optimize("",off)
MaterialDescriptorArrayManager::~MaterialDescriptorArrayManager()
{
	for(auto &pair : m_texData)
	{
		auto &cb = pair.second.onRemoveCallback;
		if(cb.IsValid())
			cb.Remove();
	}
}
void MaterialDescriptorArrayManager::Initialize(prosper::Context &context)
{
	auto instanceSize = sizeof(MaterialRenderInfoBufferData);
	auto instanceCount = 4'096;
	auto maxInstanceCount = instanceCount *10u;
	prosper::util::BufferCreateInfo createInfo {};
	createInfo.memoryFeatures = prosper::util::MemoryFeatureFlags::GPUBulk;
	createInfo.size = instanceSize *instanceCount;
	createInfo.usageFlags = Anvil::BufferUsageFlagBits::UNIFORM_BUFFER_BIT | Anvil::BufferUsageFlagBits::TRANSFER_SRC_BIT | Anvil::BufferUsageFlagBits::TRANSFER_DST_BIT;
	m_materialInfoBuffer = prosper::util::create_uniform_resizable_buffer(context,createInfo,instanceSize,instanceSize *maxInstanceCount,0.1f);
	m_materialInfoBuffer->SetDebugName("material_info_buf");
}
const std::shared_ptr<prosper::UniformResizableBuffer> &MaterialDescriptorArrayManager::GetMaterialInfoBuffer() const {return m_materialInfoBuffer;}
std::optional<prosper::Buffer::SubBufferIndex> MaterialDescriptorArrayManager::RegisterMaterial(const Material &mat)
{
	auto it = m_materialRenderBuffers.find(&mat);
	if(it != m_materialRenderBuffers.end())
		return it->second->GetBaseIndex();
	MaterialRenderInfoBufferData matRenderInfo {};

	auto *pDiffuseMap = mat.GetDiffuseMap();
	if(pDiffuseMap && pDiffuseMap->texture)
	{
		auto index = AddItem(*std::static_pointer_cast<Texture>(pDiffuseMap->texture));
		if(index.has_value())
			matRenderInfo.diffuseTextureArrayIndex = *index;
	}

	auto *pSpecularMap = mat.GetSpecularMap();
	if(pSpecularMap && pSpecularMap->texture)
	{
		auto index = AddItem(*std::static_pointer_cast<Texture>(pSpecularMap->texture));
		if(index.has_value())
			matRenderInfo.specularTextureArrayIndex = *index;
	}
	
	auto *pNormalMap = mat.GetNormalMap();
	if(pNormalMap && pNormalMap->texture)
	{
		auto index = AddItem(*std::static_pointer_cast<Texture>(pNormalMap->texture));
		if(index.has_value())
			matRenderInfo.normalTextureArrayIndex = *index;
	}

	auto matBuffer = m_materialInfoBuffer->AllocateBuffer(&matRenderInfo);
	if(matBuffer)
		m_materialRenderBuffers.insert(std::make_pair(&mat,matBuffer));
	return matBuffer->GetBaseIndex();

	// Add material textures to texture array
	/*std::function<void(ds::Block &dataBlock)> fIterateTextures = nullptr;
	fIterateTextures = [this,&fIterateTextures](ds::Block &dataBlock) {
		auto *data = dataBlock.GetData();
		if(data == nullptr)
			return;
		for(auto &pair : *data)
		{
			auto &dataVal = pair.second;
			if(dataVal == nullptr)
				continue;
			if(dataVal->IsBlock())
			{
				fIterateTextures(static_cast<ds::Block&>(*dataVal));
				continue;
			}
			auto *dsTex = dynamic_cast<ds::Texture*>(dataVal.get());
			if(dsTex == nullptr)
				continue;
			auto &texInfo = dsTex->GetValue();
			if(texInfo.texture == nullptr)
				continue;
			auto texture = std::static_pointer_cast<Texture>(texInfo.texture);
			auto it = m_texData.find(texture.get());
			if(it != m_texData.end())
				continue; // Texture is already in array?
			auto index = DescriptorArrayManager::AddItem([&texture](Anvil::DescriptorSet &ds,ArrayIndex index) -> bool {
				return prosper::util::set_descriptor_set_binding_array_texture(ds,*texture->texture,0,index);
			});
			if(index.has_value())
			{
				auto &texRef = *texture;
				auto cb = texture->CallOnRemove([this,&texRef]() {
					RemoveItem(texRef);
				});
				m_texData[texture.get()] = {*index,cb};
			}
		}
	};
	auto &rootDataBlock = mat.GetDataBlock();
	if(rootDataBlock)
		fIterateTextures(*rootDataBlock);*/
}
std::optional<prosper::DescriptorArrayManager::ArrayIndex> MaterialDescriptorArrayManager::AddItem(Texture &tex)
{
	auto it = m_texData.find(&tex);
	if(it != m_texData.end())
		return it->second.arrayIndex; // Texture is already in array?
	auto index = DescriptorArrayManager::AddItem([&tex](Anvil::DescriptorSet &ds,ArrayIndex index,uint32_t bindingIndex) -> bool {
		return prosper::util::set_descriptor_set_binding_array_texture(ds,*tex.texture,bindingIndex,index);
	});
	if(index.has_value() == false)
		return {};
	auto cb = tex.CallOnRemove([this,&tex]() {
		RemoveItem(tex);
	});
	m_texData[&tex] = {*index,cb};
	return index;
}
void MaterialDescriptorArrayManager::RemoveItem(const Texture &tex)
{
	auto it = m_texData.find(&tex);
	if(it == m_texData.end())
		return;
	auto &texData = it->second;
	DescriptorArrayManager::RemoveItem(texData.arrayIndex);
	if(texData.onRemoveCallback.IsValid())
		texData.onRemoveCallback.Remove();
	m_texData.erase(it);
}
#pragma optimize("",on)
