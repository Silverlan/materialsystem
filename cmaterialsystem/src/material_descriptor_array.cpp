/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <prosper_context.hpp>
#include <prosper_util.hpp>
#include "material_descriptor_array.hpp"
#include "cmaterial_manager2.hpp"
#include "textureinfo.h"
#include "texturemanager/texture.h"
#include "cmaterial.h"
#include <cmaterialmanager.h>
#include <buffers/prosper_uniform_resizable_buffer.hpp>
#include <prosper_descriptor_set_group.hpp>
#include <datasystem.h>

MaterialDescriptorArrayManager::~MaterialDescriptorArrayManager()
{
	for(auto &pair : m_texData) {
		auto &cb = pair.second.onRemoveCallback;
		if(cb.IsValid())
			cb.Remove();
	}
}
void MaterialDescriptorArrayManager::Initialize(prosper::IPrContext &context)
{
	auto instanceSize = sizeof(MaterialRenderInfoBufferData);
	auto instanceCount = 4'096;
	auto maxInstanceCount = instanceCount * 10u;
	prosper::util::BufferCreateInfo createInfo {};
	createInfo.memoryFeatures = prosper::MemoryFeatureFlags::GPUBulk;
	createInfo.size = instanceSize * instanceCount;
	createInfo.usageFlags = prosper::BufferUsageFlags::StorageBufferBit | prosper::BufferUsageFlags::UniformBufferBit | prosper::BufferUsageFlags::TransferSrcBit | prosper::BufferUsageFlags::TransferDstBit;
	m_materialInfoBuffer = context.CreateUniformResizableBuffer(createInfo, instanceSize, instanceSize * maxInstanceCount, 0.1f);
	m_materialInfoBuffer->SetDebugName("material_info_buf");
}
const std::shared_ptr<prosper::IUniformResizableBuffer> &MaterialDescriptorArrayManager::GetMaterialInfoBuffer() const { return m_materialInfoBuffer; }
std::optional<prosper::IBuffer::SubBufferIndex> MaterialDescriptorArrayManager::RegisterMaterial(const Material &mat, bool reInitialize)
{
	auto it = m_materialRenderBuffers.find(&mat);
	if(it != m_materialRenderBuffers.end()) {
		if(reInitialize == false)
			return it->second->GetBaseIndex();
	}
	else
		it = m_materialRenderBuffers.insert(std::make_pair(&mat, std::shared_ptr<prosper::IBuffer> {})).first;

	auto *errMat = mat.GetManager().GetErrorMaterial();
	if(errMat == nullptr)
		return {};
	auto *errTex = errMat->GetDiffuseMap();
	auto loaded = mat.IsLoaded();
	if(loaded == false) {
		std::weak_ptr<MaterialDescriptorArrayManager> wpThis = std::static_pointer_cast<MaterialDescriptorArrayManager>(shared_from_this());
		mat.CallOnLoaded([wpThis, &mat]() {
			if(wpThis.expired())
				return;
			wpThis.lock()->RegisterMaterial(mat, true); // Re-register material once all textures have been loaded!
		});
	}

	MaterialRenderInfoBufferData matRenderInfo {};
	auto *pDiffuseMap = loaded ? mat.GetDiffuseMap() : errTex;
	if(pDiffuseMap && pDiffuseMap->texture) {
		auto index = AddItem(*std::static_pointer_cast<Texture>(pDiffuseMap->texture));
		if(index.has_value())
			matRenderInfo.albedoTextureArrayIndex = *index;
	}

	auto *pAlbedoMap = loaded ? mat.GetTextureInfo("albedo_map") : errTex;
	if(pAlbedoMap && pAlbedoMap->texture) {
		auto index = AddItem(*std::static_pointer_cast<Texture>(pAlbedoMap->texture));
		if(index.has_value())
			matRenderInfo.albedoTextureArrayIndex = *index;
	}

	auto *pNormalMap = loaded ? mat.GetNormalMap() : errTex;
	if(pNormalMap && pNormalMap->texture) {
		auto index = AddItem(*std::static_pointer_cast<Texture>(pNormalMap->texture));
		if(index.has_value())
			matRenderInfo.normalTextureArrayIndex = *index;
	}

	pNormalMap = loaded ? mat.GetTextureInfo("normal_map") : errTex;
	if(pNormalMap && pNormalMap->texture) {
		auto index = AddItem(*std::static_pointer_cast<Texture>(pNormalMap->texture));
		if(index.has_value())
			matRenderInfo.normalTextureArrayIndex = *index;
	}

	auto *pRmaMap = loaded ? mat.GetTextureInfo("rma_map") : errTex;
	if(pRmaMap && pRmaMap->texture) {
		auto index = AddItem(*std::static_pointer_cast<Texture>(pRmaMap->texture));
		if(index.has_value())
			matRenderInfo.rmaTextureArrayIndex = *index;
	}

	auto &matBuffer = it->second;
	if(matBuffer == nullptr)
		matBuffer = m_materialInfoBuffer->AllocateBuffer(&matRenderInfo);
	else
		matBuffer->Write(0, sizeof(matRenderInfo), &matRenderInfo);
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
	auto index = DescriptorArrayManager::AddItem([&tex](prosper::IDescriptorSet &ds, ArrayIndex index, uint32_t bindingIndex) -> bool { return ds.SetBindingArrayTexture(*tex.GetVkTexture(), bindingIndex, index); });
	if(index.has_value() == false)
		return {};
	auto cb = tex.CallOnRemove([this, &tex]() { RemoveItem(tex); });
	m_texData[&tex] = {*index, cb};
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
