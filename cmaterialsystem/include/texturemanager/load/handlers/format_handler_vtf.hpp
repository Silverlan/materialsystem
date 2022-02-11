/* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __MSYS_TEXTURE_FORMAT_HANDLER_VTF_HPP__
#define __MSYS_TEXTURE_FORMAT_HANDLER_VTF_HPP__

#include "texturemanager/load/texture_format_handler.hpp"
#include <prosper_enums.hpp>
#include <optional>
#include <array>

#ifndef DISABLE_VTF_SUPPORT
namespace VTFLib {class CVTFFile;};

namespace msys
{
	namespace detail
	{
		struct VulkanImageData
		{
			// Original image format
			prosper::Format format = prosper::Format::R8G8B8A8_UNorm;

			// Some formats may not be supported in optimal layout by common GPUs, in which case we need to convert it
			std::optional<prosper::Format> conversionFormat = {};

			std::array<prosper::ComponentSwizzle,4> swizzle = {prosper::ComponentSwizzle::R,prosper::ComponentSwizzle::G,prosper::ComponentSwizzle::B,prosper::ComponentSwizzle::A};
		};
	};
	class DLLCMATSYS TextureFormatHandlerVtf
		: public ITextureFormatHandler
	{
	public:
		TextureFormatHandlerVtf(util::IAssetManager &assetManager);
		virtual bool GetDataPtr(uint32_t layer,uint32_t mipmapIdx,void **outPtr,size_t &outSize) override;
	protected:
		virtual bool LoadData(InputTextureInfo &texInfo) override;
	private:
		std::shared_ptr<VTFLib::CVTFFile> m_texture = nullptr;
	};
};
#endif

#endif
