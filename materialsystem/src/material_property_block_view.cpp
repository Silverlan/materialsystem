// SPDX-FileCopyrightText: (c) 2025 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#include "material_property_block_view.hpp"

msys::MaterialPropertyBlockView::IterationData::IterationData(MaterialPropertyBlockView &blockView) : blockView {&blockView}
{
	std::unordered_set<std::string_view> subBlockNames;
	for(auto *block : blockView.blocks) {
		for(auto &[name, base] : *block->GetData()) {
			if(base->IsBlock()) {
				subBlockNames.insert(name);
				continue;
			}
			map[name] = block;
		}
	}
}

msys::MaterialPropertyBlockView::MaterialPropertyBlockView(Material &mat, const util::Path &path)
{
	std::vector<Material *> mats;
	mats.push_back(&mat);
	auto *matBase = mat.GetBaseMaterial();
	while(matBase) {
		mats.push_back(matBase);
		matBase = matBase->GetBaseMaterial();
	}

	blocks.reserve(mats.size());
	for(auto it = mats.rbegin(); it != mats.rend(); ++it) {
		auto *mat = *it;
		auto block = mat->GetPropertyBlock(path.GetString());
		if(!block)
			continue;
		blocks.push_back(block.get());
	}

	iterationData = std::make_unique<IterationData>(*this);
}
