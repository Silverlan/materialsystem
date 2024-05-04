/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2021 Silverlan
 */

#include "matsysdefinitions.h"
#include "source_vmt_format_handler.hpp"
#include "material.h"
#include "material_manager2.hpp"
#include "textureinfo.h"
#include "detail_mode.hpp"
#include <sharedutils/util_string.h>
#include <datasystem.h>

#ifndef DISABLE_VMT_SUPPORT
#ifdef ENABLE_VKV_PARSER
#include "util_vmt.hpp"
#include <VKVParser/library.h>

// Find highest dx node version and merge its values with the specified node
static void merge_dx_node_values(ValveKeyValueFormat::KVNode &node)
{
#if 0
	enum class DXVersion : uint8_t
	{
		Undefined = 0,
		dx90,
		dx90_20b // dxlevel 95
	};
	const std::unordered_map<std::string,DXVersion> dxStringToEnum = {
		{"dx90",DXVersion::dx90},
		{"dx90_20b",DXVersion::dx90_20b}
	};
	enum class Operator : int8_t
	{
		None = -1,
		LessThan = 0,
		LessThanOrEqual,
		GreaterThanOrEqual,
		GreaterThan
	}; // Operators ordered by significance!
	const std::array<std::string,4> acceptedOperators = {"<","<=",">=",">"};
	auto numNodes = node.branches.size();
	KVBranch *dxNode = nullptr;
	auto bestDxVersion = DXVersion::Undefined;
	auto bestOperator = Operator::None;
	for(auto i=decltype(numNodes){0u};i<numNodes;++i)
	{
		auto *pNode = node.Get
		auto *name = pNode->GetName();
		std::string dxLevelValue {};
		auto op = Operator::None;
		for(auto opIdx : {2,3,1,0}) // Order is important! (Ordered by string length per operator type (e.g. '<=' has to come before '<'))
		{
			auto &candidate = acceptedOperators.at(opIdx);
			if(ustring::compare(name,candidate.c_str(),true,candidate.length()) == false)
				continue;
			op = static_cast<Operator>(opIdx);
		}
		if(op == Operator::None)
			dxLevelValue = name;
		else
			dxLevelValue = ustring::substr(std::string{name},acceptedOperators.at(umath::to_integral(op)).length());
		ustring::to_lower(dxLevelValue);
		auto it = dxStringToEnum.find(dxLevelValue);
		if(it == dxStringToEnum.end())
			continue;
		if(umath::to_integral(it->second) <= umath::to_integral(bestDxVersion) && umath::to_integral(op) <= umath::to_integral(bestOperator))
			continue;
		dxNode = pNode;
		bestDxVersion = it->second;
		bestOperator = op;
	}
	if(dxNode != nullptr && dxNode->GetType() == VMTNodeType::NODE_TYPE_GROUP)
	{
		auto *dxGroupNode = static_cast<VTFLib::Nodes::CVMTGroupNode*>(dxNode);
		auto numNodesDx = dxGroupNode->GetNodeCount();
		for(auto i=decltype(numNodesDx){0u};i<numNodesDx;++i)
			node.AddNode(dxGroupNode->GetNode(i)->Clone());
	}
#endif
}

msys::SourceVmtFormatHandler2::SourceVmtFormatHandler2(util::IAssetManager &assetManager) : ISourceVmtFormatHandler {assetManager} {}
bool msys::SourceVmtFormatHandler2::Import(const std::string &outputPath, std::string &outFilePath)
{
	auto size = m_file->GetSize();
	if(size == 0)
		return false;
	std::string data;
	data.resize(size);
	size = m_file->Read(data.data(), size);
	if(size == 0)
		return false;
	data.resize(size);

	ValveKeyValueFormat::setLogCallback([](const std::string &message, ValveKeyValueFormat::LogLevel severity) { std::cout << message << std::endl; });
	auto kvNode = ValveKeyValueFormat::parseKVBuffer(data);
	if(!kvNode) {
		m_error = "TODO";
		return false;
	}
	auto *vmtRoot = kvNode.get();
	merge_dx_node_values(*vmtRoot);
	m_rootNode = std::make_shared<VkvNode>(*kvNode);
	return LoadVMT(*m_rootNode, outputPath, outFilePath);
}

static std::vector<float> get_vmt_matrix(std::string value, bool *optOutIsMatrix = nullptr)
{
	if(optOutIsMatrix)
		*optOutIsMatrix = false;
	if(value.front() == '[') {
		value = value.substr(1);
		if(optOutIsMatrix)
			*optOutIsMatrix = true;
	}
	if(value.back() == ']')
		value = value.substr(0, value.length() - 1);
	std::vector<std::string> substrings {};
	ustring::explode_whitespace(value, substrings);

	std::vector<float> data;
	data.reserve(substrings.size());
	for(auto &str : substrings)
		data.push_back(util::to_float(str));
	return data;
}

std::optional<std::string> msys::SourceVmtFormatHandler2::GetStringValue(ValveKeyValueFormat::KVBranch &node, std::string key)
{
	for(auto &c : key)
		c = std::tolower(c);
	auto it = node.branches.find(key);
	if(it == node.branches.end())
		return {};
	auto *leaf = it->second->as_leaf();
	if(!leaf)
		return {};
	return std::string {leaf->value};
}
#endif
#endif
