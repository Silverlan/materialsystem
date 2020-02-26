#ifndef __UTIL_VMT_HPP__
#define __UTIL_VMT_HPP__

#include <sharedutils/util.h>

template<typename T>
	bool vmt_parameter_to_numeric_type(VTFLib::Nodes::CVMTNode *node,T &outResult)
{
	auto type = node->GetType();
	switch(type)
	{
	case VMTNodeType::NODE_TYPE_SINGLE:
	{
		auto *singleNode = static_cast<VTFLib::Nodes::CVMTSingleNode*>(node);
		auto v = singleNode->GetValue();
		outResult = static_cast<T>(v);
		return true;
	}
	case VMTNodeType::NODE_TYPE_INTEGER:
	{
		auto *integerNode = static_cast<VTFLib::Nodes::CVMTIntegerNode*>(node);
		auto v = static_cast<float>(integerNode->GetValue());
		outResult = static_cast<T>(v);
		return true;
	}
	case VMTNodeType::NODE_TYPE_STRING:
	{
		auto *stringNode = static_cast<VTFLib::Nodes::CVMTStringNode*>(node);
		auto v = ::util::to_float(stringNode->GetValue());
		outResult = static_cast<T>(v);
		return true;
	}
	}
	return false;
}

template<class TData,typename TInternal>
	void get_vmt_data(const std::shared_ptr<ds::Block> &root,ds::Settings &dataSettings,const std::string &key,VTFLib::Nodes::CVMTNode *node,const std::function<TInternal(TInternal)> &translate=nullptr)
{
	auto type = node->GetType();
	switch(type)
	{
	case VMTNodeType::NODE_TYPE_SINGLE:
	{
		auto *singleNode = static_cast<VTFLib::Nodes::CVMTSingleNode*>(node);
		auto v = singleNode->GetValue();
		if(translate != nullptr)
			v = translate(v);
		root->AddData(key,std::make_shared<TData>(dataSettings,std::to_string(v)));
		break;
	}
	case VMTNodeType::NODE_TYPE_INTEGER:
	{
		auto *integerNode = static_cast<VTFLib::Nodes::CVMTIntegerNode*>(node);
		auto v = static_cast<float>(integerNode->GetValue());
		if(translate != nullptr)
			v = translate(v);
		root->AddData(key,std::make_shared<TData>(dataSettings,std::to_string(v)));
		break;
	}
	case VMTNodeType::NODE_TYPE_STRING:
	{
		auto *stringNode = static_cast<VTFLib::Nodes::CVMTStringNode*>(node);
		auto v = ::util::to_float(stringNode->GetValue());
		if(translate != nullptr)
			v = translate(v);
		root->AddData(key,std::make_shared<TData>(dataSettings,std::to_string(v)));
		break;
	}
	}
}

#endif
