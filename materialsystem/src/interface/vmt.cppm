// SPDX-FileCopyrightText: (c) 2020 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include <VMTFile.h>
#include <VTFLib.h>
#ifdef ENABLE_VKV_PARSER
#include <VKVParser/library.h>
#endif

export module pragma.materialsystem:vmt;

export import pragma.datasystem;
import pragma.util;

#undef max
#undef min

export {
	template<typename T>
	bool vmt_parameter_to_numeric_type(const VTFLib::Nodes::CVMTNode *node, T &outResult)
	{
		auto type = node->GetType();
		switch(type) {
		case VMTNodeType::NODE_TYPE_SINGLE:
			{
				auto *singleNode = static_cast<const VTFLib::Nodes::CVMTSingleNode *>(node);
				auto v = singleNode->GetValue();
				outResult = static_cast<T>(v);
				return true;
			}
		case VMTNodeType::NODE_TYPE_INTEGER:
			{
				auto *integerNode = static_cast<const VTFLib::Nodes::CVMTIntegerNode *>(node);
				auto v = static_cast<float>(integerNode->GetValue());
				outResult = static_cast<T>(v);
				return true;
			}
		case VMTNodeType::NODE_TYPE_STRING:
			{
				auto *stringNode = static_cast<const VTFLib::Nodes::CVMTStringNode *>(node);
				auto v = ::util::to_float(stringNode->GetValue());
				outResult = static_cast<T>(v);
				return true;
			}
		default:
			break;
		}
		return false;
	}

	template<typename T>
	bool vmt_parameter_to_numeric_type(const std::string &value, T &outResult)
	{
		if constexpr(std::is_same_v<T, bool>) {
			outResult = util::to_boolean(value);
			return true;
		}
		else if constexpr(std::is_integral_v<T>) {
			outResult = util::to_int(value);
			return true;
		}
		else if constexpr(std::is_floating_point_v<T>) {
			outResult = util::to_float(value);
			return true;
		}
		return false;
	}

	std::optional<Vector3> vmt_parameter_to_color(std::string value)
	{
		auto start = value.find_first_of("[{");
		auto end = value.find_first_of("]}", start);
		if(end == std::string::npos)
			return {};
		auto colorValue = (value.at(start) == '{');
		value = value.substr(start + 1, end - start - 1);
		ustring::remove_whitespace(value);
		auto color = uvec::create(value);
		if(colorValue)
			color /= static_cast<float>(std::numeric_limits<uint8_t>::max());
		return color;
	}

	std::optional<Vector3> vmt_parameter_to_color(const VTFLib::Nodes::CVMTNode &node)
	{
		if(node.GetType() != VMTNodeType::NODE_TYPE_STRING)
			return {};
		auto &stringNode = static_cast<const VTFLib::Nodes::CVMTStringNode &>(node);
		return vmt_parameter_to_color(std::string {stringNode.GetValue()});
	}

	template<class TData, typename TInternal>
	void get_vmt_data(const std::shared_ptr<ds::Block> &root, ds::Settings &dataSettings, const std::string &key, VTFLib::Nodes::CVMTNode *node, const std::function<TInternal(TInternal)> &translate = nullptr)
	{
		auto type = node->GetType();
		switch(type) {
		case VMTNodeType::NODE_TYPE_SINGLE:
			{
				auto *singleNode = static_cast<VTFLib::Nodes::CVMTSingleNode *>(node);
				auto v = singleNode->GetValue();
				if(translate != nullptr)
					v = translate(v);
				root->AddData(key, std::make_shared<TData>(dataSettings, std::to_string(v)));
				break;
			}
		case VMTNodeType::NODE_TYPE_INTEGER:
			{
				auto *integerNode = static_cast<VTFLib::Nodes::CVMTIntegerNode *>(node);
				auto v = static_cast<float>(integerNode->GetValue());
				if(translate != nullptr)
					v = translate(v);
				root->AddData(key, std::make_shared<TData>(dataSettings, std::to_string(v)));
				break;
			}
		case VMTNodeType::NODE_TYPE_STRING:
			{
				auto *stringNode = static_cast<VTFLib::Nodes::CVMTStringNode *>(node);
				auto v = ::util::to_float(stringNode->GetValue());
				if(translate != nullptr)
					v = translate(v);
				root->AddData(key, std::make_shared<TData>(dataSettings, std::to_string(v)));
				break;
			}
		default:
			break;
		}
	}

	template<class TData, typename TInternal>
	void get_vmt_data(const std::shared_ptr<ds::Block> &root, ds::Settings &dataSettings, const std::string &key, std::string value, const std::function<TInternal(TInternal)> &translate = nullptr)
	{
		if(translate != nullptr) {
			if constexpr(std::is_same_v<TInternal, float>)
				value = std::to_string(translate(util::to_float(value)));
			else if constexpr(std::is_same_v<TInternal, int>)
				value = std::to_string(translate(util::to_int(value)));
			else {
				[]<bool flag = false>() { static_assert(flag, "Unsupported type"); }();
			}
		}
		root->AddData(key, std::make_shared<TData>(dataSettings, value));
	}

	std::array<float, 3> get_vmt_matrix(std::string value)
	{
		if(value.front() == '[')
			value = value.substr(1);
		if(value.back() == ']')
			value = value.substr(0, value.length() - 1);
		std::vector<std::string> substrings {};
		ustring::explode_whitespace(value, substrings);

		std::array<float, 3> data {0.f, 0.f, 0.f};
		for(uint8_t i = 0; i < umath::min(substrings.size(), static_cast<size_t>(3)); ++i) {
			auto val = util::to_float(substrings.at(i));
			for(auto j = i; j < 3; ++j)
				data[j] = val;
		}
		return data;
	}
}
