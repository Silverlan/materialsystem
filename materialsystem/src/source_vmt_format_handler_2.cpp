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

static std::optional<util::LogSeverity> to_util_severity(ValveKeyValueFormat::LogLevel severity)
{
	switch(severity) {
	case ValveKeyValueFormat::LogLevel::ALL:
		return util::LogSeverity::Info;
	case ValveKeyValueFormat::LogLevel::TRACE:
		return util::LogSeverity::Trace;
	case ValveKeyValueFormat::LogLevel::DEBUG:
		return util::LogSeverity::Debug;
	case ValveKeyValueFormat::LogLevel::WARN:
		return util::LogSeverity::Warning;
	case ValveKeyValueFormat::LogLevel::ERR:
		return util::LogSeverity::Error;
	}
	return {};
}

msys::SourceVmtFormatHandler2::SourceVmtFormatHandler2(util::IAssetManager &assetManager) : ISourceVmtFormatHandler {assetManager}
{
	ValveKeyValueFormat::setLogCallback([&assetManager](const std::string &message, ValveKeyValueFormat::LogLevel severity) {
		if(!assetManager.ShouldLog())
			return;
		auto utilSeverity = to_util_severity(severity);
		if(!utilSeverity)
			return;
		assetManager.Log(message, *utilSeverity);
	});
}
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

	auto kvNode = ValveKeyValueFormat::parseKVBuffer(data);
	if(!kvNode) {
		auto fileName = m_file->GetFileName();
		if(!fileName)
			fileName = "UNKNOWN";
		m_error = "Failed to parse VMT file data for file '" + *fileName + "'!";
		return false;
	}
	auto *vmtRoot = kvNode.get();
	merge_dx_node_values(*vmtRoot);
	m_rootNode = std::make_shared<VkvNode>(*kvNode);
	return LoadVMT(*m_rootNode, outputPath, outFilePath);
}

std::string msys::SourceVmtFormatHandler2::GetShader() const
{
	auto &kvNode = GetVkvNode(*m_rootNode);
	return std::string {kvNode.get_key()};
}
std::shared_ptr<const msys::IVmtNode> msys::SourceVmtFormatHandler2::GetNode(const std::string &key, const IVmtNode *optParent) const
{
	if(!optParent)
		return GetNode(key, m_rootNode.get());
	auto &kvNode = GetVkvNode(*optParent);
	if(kvNode.get_type() != ValveKeyValueFormat::KVNodeType::BRANCH)
		return nullptr;
	auto &kvBranch = static_cast<const ValveKeyValueFormat::KVBranch &>(kvNode);
	auto it = kvBranch.branches.find(key);
	if(it == kvBranch.branches.end())
		return nullptr;
	return std::make_shared<VkvNode>(*it->second);
}
std::optional<std::string> msys::SourceVmtFormatHandler2::GetStringValue(const IVmtNode &node) const
{
	auto &kvNode = GetVkvNode(node);
	if(kvNode.get_type() != ValveKeyValueFormat::KVNodeType::LEAF)
		return {};
	return std::string {static_cast<const ValveKeyValueFormat::KVLeaf &>(kvNode).value};
}
std::optional<bool> msys::SourceVmtFormatHandler2::GetBooleanValue(const IVmtNode &node) const
{
	auto &kvNode = GetVkvNode(node);
	if(kvNode.get_type() != ValveKeyValueFormat::KVNodeType::LEAF)
		return {};
	std::string strVal {static_cast<const ValveKeyValueFormat::KVLeaf &>(kvNode).value};
	bool value;
	if(!vmt_parameter_to_numeric_type<bool>(strVal, value))
		return {};
	return value;
}
std::optional<float> msys::SourceVmtFormatHandler2::GetFloatValue(const IVmtNode &node) const
{
	auto &kvNode = GetVkvNode(node);
	if(kvNode.get_type() != ValveKeyValueFormat::KVNodeType::LEAF)
		return {};
	std::string strVal {static_cast<const ValveKeyValueFormat::KVLeaf &>(kvNode).value};
	float value;
	if(!vmt_parameter_to_numeric_type<float>(strVal, value))
		return {};
	return value;
}
std::optional<Vector3> msys::SourceVmtFormatHandler2::GetColorValue(const IVmtNode &node) const
{
	auto &kvNode = GetVkvNode(node);
	if(kvNode.get_type() != ValveKeyValueFormat::KVNodeType::LEAF)
		return {};
	return vmt_parameter_to_color(std::string {static_cast<const ValveKeyValueFormat::KVLeaf &>(kvNode).value});
}
std::optional<uint8_t> msys::SourceVmtFormatHandler2::GetUint8Value(const IVmtNode &node) const
{
	auto &kvNode = GetVkvNode(node);
	if(kvNode.get_type() != ValveKeyValueFormat::KVNodeType::LEAF)
		return {};
	std::string strVal {static_cast<const ValveKeyValueFormat::KVLeaf &>(kvNode).value};
	uint8_t value;
	if(!vmt_parameter_to_numeric_type<uint8_t>(strVal, value))
		return {};
	return value;
}
std::optional<int32_t> msys::SourceVmtFormatHandler2::GetInt32Value(const IVmtNode &node) const
{
	auto &kvNode = GetVkvNode(node);
	if(kvNode.get_type() != ValveKeyValueFormat::KVNodeType::LEAF)
		return {};
	std::string strVal {static_cast<const ValveKeyValueFormat::KVLeaf &>(kvNode).value};
	int32_t value;
	if(!vmt_parameter_to_numeric_type<int32_t>(strVal, value))
		return {};
	return value;
}
std::optional<std::array<float, 3>> msys::SourceVmtFormatHandler2::GetMatrixValue(const IVmtNode &node) const
{
	auto &kvNode = GetVkvNode(node);
	if(kvNode.get_type() != ValveKeyValueFormat::KVNodeType::LEAF)
		return {};
	std::string strVal {static_cast<const ValveKeyValueFormat::KVLeaf &>(kvNode).value};
	int32_t value;
	return get_vmt_matrix(strVal);
}

const ValveKeyValueFormat::KVNode &msys::SourceVmtFormatHandler2::GetVkvNode(const IVmtNode &vmtNode) const { return static_cast<const VkvNode &>(vmtNode).vkvNode; }
#endif
#endif
