/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __SOURCE_VMT_FORMAT_HANDLER_HPP__
#define __SOURCE_VMT_FORMAT_HANDLER_HPP__

#ifndef DISABLE_VMT_SUPPORT
#include "matsysdefinitions.h"
#include <sharedutils/asset_loader/asset_format_handler.hpp>
#include <mathutil/uvec.h>

#define ENABLE_VKV_PARSER

namespace VTFLib {
	class CVMTFile;
	namespace Nodes {
		class CVMTNode;
		class CVMTGroupNode;
	};
};
namespace ds {
	class Block;
};
namespace ValveKeyValueFormat {
	class KVNode;
	class KVBranch;
};
namespace msys {
	struct IVmtNode {
		virtual ~IVmtNode() = default;
	};
	struct VtfLibVmtNode : public IVmtNode {
		VtfLibVmtNode(const VTFLib::Nodes::CVMTNode &vtfLibNode) : vtfLibNode {vtfLibNode} {}
		const VTFLib::Nodes::CVMTNode &vtfLibNode;
	};
	class DLLMATSYS ISourceVmtFormatHandler : public util::IImportAssetFormatHandler {
	  public:
		ISourceVmtFormatHandler(util::IAssetManager &assetManager);
	  protected:
		virtual std::string GetShader() const = 0;
		virtual std::shared_ptr<const IVmtNode> GetNode(const std::string &key, const IVmtNode *optParent = nullptr) const = 0;
		virtual std::optional<std::string> GetStringValue(const IVmtNode &node) const = 0;
		virtual std::optional<bool> GetBooleanValue(const IVmtNode &node) const = 0;
		virtual std::optional<float> GetFloatValue(const IVmtNode &node) const = 0;
		virtual std::optional<Vector3> GetColorValue(const IVmtNode &node) const = 0;
		virtual std::optional<uint8_t> GetUint8Value(const IVmtNode &node) const = 0;
		virtual std::optional<uint8_t> GetInt32Value(const IVmtNode &node) const = 0;
		virtual std::optional<std::array<float, 3>> GetMatrixValue(const IVmtNode &node) const = 0;

		std::optional<std::string> GetStringValue(const std::string &key, const IVmtNode *optParent = nullptr, const std::optional<std::string> &defaultValue = {}) const;
		std::optional<bool> GetBooleanValue(const std::string &key, const IVmtNode *optParent = nullptr, const std::optional<bool> &defaultValue = {}) const;
		std::optional<float> GetFloatValue(const std::string &key, const IVmtNode *optParent = nullptr, const std::optional<float> &defaultValue = {}) const;
		std::optional<Vector3> GetColorValue(const std::string &key, const IVmtNode *optParent = nullptr, const std::optional<Vector3> &defaultValue = {}) const;
		std::optional<uint8_t> GetUint8Value(const std::string &key, const IVmtNode *optParent = nullptr, const std::optional<uint8_t> &defaultValue = {}) const;
		std::optional<uint8_t> GetInt32Value(const std::string &key, const IVmtNode *optParent = nullptr, const std::optional<uint8_t> &defaultValue = {}) const;

		bool AssignStringValue(ds::Block &dsData, const IVmtNode &vmtNode, const std::string &vmtKey, const std::string &pragmaKey) const;
		bool AssignTextureValue(ds::Block &dsData, const IVmtNode &vmtNode, const std::string &vmtKey, const std::string &pragmaKey) const;
		bool AssignBooleanValue(ds::Block &dsData, const IVmtNode &vmtNode, const std::string &vmtKey, const std::string &pragmaKey, std::optional<bool> defaultValue = std::nullopt) const;
		bool AssignFloatValue(ds::Block &dsData, const IVmtNode &vmtNode, const std::string &vmtKey, const std::string &pragmaKey, std::optional<float> defaultValue = std::nullopt) const;

		virtual bool LoadVmtData(const std::string &vmtShader, ds::Block &rootData, std::string &matShader) = 0;
		bool LoadVMT(const IVmtNode &rootNode, const std::string &outputPath, std::string &outFilePath);

		std::shared_ptr<IVmtNode> m_rootNode;
	};
	class DLLMATSYS SourceVmtFormatHandler : public ISourceVmtFormatHandler {
	  public:
		SourceVmtFormatHandler(util::IAssetManager &assetManager);
		virtual bool Import(const std::string &outputPath, std::string &outFilePath) override;
	  protected:
		const VTFLib::Nodes::CVMTNode &GetVtfLibNode(const IVmtNode &vmtNode) const;

		virtual std::string GetShader() const override;
		virtual std::shared_ptr<const IVmtNode> GetNode(const std::string &key, const IVmtNode *optParent = nullptr) const override;
		virtual std::optional<std::string> GetStringValue(const IVmtNode &node) const override;
		virtual std::optional<bool> GetBooleanValue(const IVmtNode &node) const override;
		virtual std::optional<float> GetFloatValue(const IVmtNode &node) const override;
		virtual std::optional<Vector3> GetColorValue(const IVmtNode &node) const override;
		virtual std::optional<uint8_t> GetUint8Value(const IVmtNode &node) const override;
		virtual std::optional<uint8_t> GetInt32Value(const IVmtNode &node) const override;
		virtual std::optional<std::array<float, 3>> GetMatrixValue(const IVmtNode &node) const override;

		using ISourceVmtFormatHandler::GetBooleanValue;
		using ISourceVmtFormatHandler::GetColorValue;
		using ISourceVmtFormatHandler::GetFloatValue;
		using ISourceVmtFormatHandler::GetInt32Value;
		using ISourceVmtFormatHandler::GetStringValue;
		using ISourceVmtFormatHandler::GetUint8Value;

		virtual bool LoadVmtData(const std::string &vmtShader, ds::Block &rootData, std::string &matShader) override { return true; }
	};
#ifdef ENABLE_VKV_PARSER
	struct VkvNode : public IVmtNode {
		VkvNode(const ValveKeyValueFormat::KVNode &vkvNode) : vkvNode {vkvNode} {}
		const ValveKeyValueFormat::KVNode &vkvNode;
	};
	class DLLMATSYS SourceVmtFormatHandler2 : public ISourceVmtFormatHandler {
	  public:
		SourceVmtFormatHandler2(util::IAssetManager &assetManager);
		virtual bool Import(const std::string &outputPath, std::string &outFilePath) override;
	  protected:
		static std::optional<std::string> GetStringValue(ValveKeyValueFormat::KVBranch &node, std::string key);

		using ISourceVmtFormatHandler::GetBooleanValue;
		using ISourceVmtFormatHandler::GetColorValue;
		using ISourceVmtFormatHandler::GetFloatValue;
		using ISourceVmtFormatHandler::GetInt32Value;
		using ISourceVmtFormatHandler::GetStringValue;
		using ISourceVmtFormatHandler::GetUint8Value;

		virtual bool LoadVmtData(const std::string &vmtShader, ds::Block &rootData, std::string &matShader) override { return true; }
	};
#endif
};
#endif

#endif
