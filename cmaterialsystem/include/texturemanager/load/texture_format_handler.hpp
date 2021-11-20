/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __MSYS_TEXTURE_FORMAT_HANDLER_HPP__
#define __MSYS_TEXTURE_FORMAT_HANDLER_HPP__

#include "cmatsysdefinitions.h"
#include <cinttypes>
#include <prosper_structs.hpp>

#undef AddJob

namespace ufile {struct IFile;};
namespace msys
{
	class DLLCMATSYS ITextureFormatHandler
	{
	public:
		struct InputTextureInfo
		{
			enum class Flags : uint32_t
			{
				None = 0u,
				CubemapBit = 1u,
				SrgbBit = CubemapBit<<1u
			};
			uint32_t width = 0;
			uint32_t height = 0;
			prosper::Format format = prosper::Format::Unknown;
			uint32_t layerCount = 1;
			uint32_t mipmapCount = 1;
			Flags flags = Flags::None;
			std::array<prosper::ComponentSwizzle,4> swizzle = {
				prosper::ComponentSwizzle::R,prosper::ComponentSwizzle::G,
				prosper::ComponentSwizzle::B,prosper::ComponentSwizzle::A
			};

			std::optional<prosper::Format> conversionFormat {};
		};

		bool LoadData();
		virtual bool GetDataPtr(uint32_t layer,uint32_t mipmapIdx,void **outPtr,size_t &outSize)=0;
		const InputTextureInfo &GetInputTextureInfo() const {return m_inputTextureInfo;}
		void SetFile(const std::shared_ptr<ufile::IFile> &file) {m_file = file;}
	protected:
		ITextureFormatHandler();
		virtual bool LoadData(InputTextureInfo &texInfo)=0;
		std::shared_ptr<ufile::IFile> m_file = nullptr;
		InputTextureInfo m_inputTextureInfo;
	};
};
REGISTER_BASIC_BITWISE_OPERATORS(msys::ITextureFormatHandler::InputTextureInfo::Flags)

#endif
