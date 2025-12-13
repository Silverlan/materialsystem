// SPDX-FileCopyrightText: (c) 2022 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#ifndef DISABLE_VMT_SUPPORT
#define ENABLE_VKV_PARSER
#endif

module pragma.materialsystem;

import :format_handlers.source_vmt;

#ifndef DISABLE_VMT_SUPPORT
#ifdef ENABLE_VKV_PARSER
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
			if(pragma::string::compare(name,candidate.c_str(),true,candidate.length()) == false)
				continue;
			op = static_cast<Operator>(opIdx);
		}
		if(op == Operator::None)
			dxLevelValue = name;
		else
			dxLevelValue = pragma::string::substr(std::string{name},acceptedOperators.at(pragma::math::to_integral(op)).length());
		pragma::string::to_lower(dxLevelValue);
		auto it = dxStringToEnum.find(dxLevelValue);
		if(it == dxStringToEnum.end())
			continue;
		if(pragma::math::to_integral(it->second) <= pragma::math::to_integral(bestDxVersion) && pragma::math::to_integral(op) <= pragma::math::to_integral(bestOperator))
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

static std::optional<pragma::util::LogSeverity> to_util_severity(ValveKeyValueFormat::LogLevel severity)
{
	switch(severity) {
	case ValveKeyValueFormat::LogLevel::ALL:
		return pragma::util::LogSeverity::Info;
	case ValveKeyValueFormat::LogLevel::TRACE:
		return pragma::util::LogSeverity::Trace;
	case ValveKeyValueFormat::LogLevel::DEBUG:
		return pragma::util::LogSeverity::Debug;
	case ValveKeyValueFormat::LogLevel::WARN:
		return pragma::util::LogSeverity::Warning;
	case ValveKeyValueFormat::LogLevel::ERR:
		return pragma::util::LogSeverity::Error;
	}
	return {};
}

pragma::material::SourceVmtFormatHandler2::SourceVmtFormatHandler2(pragma::util::IAssetManager &assetManager) : ISourceVmtFormatHandler {assetManager}
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
bool pragma::material::SourceVmtFormatHandler2::Import(const std::string &outputPath, std::string &outFilePath)
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
	m_rootNode = pragma::util::make_shared<VkvNode>(*kvNode);
	return LoadVMT(*m_rootNode, outputPath, outFilePath);
}

std::string pragma::material::SourceVmtFormatHandler2::GetShader() const
{
	auto &kvNode = GetVkvNode(*m_rootNode);
	return std::string {kvNode.get_key()};
}
std::shared_ptr<const pragma::material::IVmtNode> pragma::material::SourceVmtFormatHandler2::GetNode(const std::string &key, const IVmtNode *optParent) const
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
	return pragma::util::make_shared<VkvNode>(*it->second);
}
std::optional<std::string> pragma::material::SourceVmtFormatHandler2::GetStringValue(const IVmtNode &node) const
{
	auto &kvNode = GetVkvNode(node);
	if(kvNode.get_type() != ValveKeyValueFormat::KVNodeType::LEAF)
		return {};
	return std::string {static_cast<const ValveKeyValueFormat::KVLeaf &>(kvNode).value};
}
std::optional<bool> pragma::material::SourceVmtFormatHandler2::GetBooleanValue(const IVmtNode &node) const
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
std::optional<float> pragma::material::SourceVmtFormatHandler2::GetFloatValue(const IVmtNode &node) const
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
std::optional<Vector3> pragma::material::SourceVmtFormatHandler2::GetColorValue(const IVmtNode &node) const
{
	auto &kvNode = GetVkvNode(node);
	if(kvNode.get_type() != ValveKeyValueFormat::KVNodeType::LEAF)
		return {};
	return vmt_parameter_to_color(std::string {static_cast<const ValveKeyValueFormat::KVLeaf &>(kvNode).value});
}
std::optional<uint8_t> pragma::material::SourceVmtFormatHandler2::GetUint8Value(const IVmtNode &node) const
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
std::optional<int32_t> pragma::material::SourceVmtFormatHandler2::GetInt32Value(const IVmtNode &node) const
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
std::optional<std::array<float, 3>> pragma::material::SourceVmtFormatHandler2::GetMatrixValue(const IVmtNode &node) const
{
	auto &kvNode = GetVkvNode(node);
	if(kvNode.get_type() != ValveKeyValueFormat::KVNodeType::LEAF)
		return {};
	std::string strVal {static_cast<const ValveKeyValueFormat::KVLeaf &>(kvNode).value};
	int32_t value;
	return get_vmt_matrix(strVal);
}

const ValveKeyValueFormat::KVNode &pragma::material::SourceVmtFormatHandler2::GetVkvNode(const IVmtNode &vmtNode) const { return static_cast<const VkvNode &>(vmtNode).vkvNode; }
#endif
#endif
