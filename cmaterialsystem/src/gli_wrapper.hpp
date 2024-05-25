/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __GLI_WRAPPER_HPP__
#define __GLI_WRAPPER_HPP__

#include <cinttypes>
#include <memory>
#include <string>

namespace prosper {
	enum class Format : uint32_t;
};

// We have to separate gli as a wrapper because we can't
// include both glm and gli at the same time.
// (Because we're using glm as a module and gli is not compatible with that)
namespace gli_wrapper {
	uint32_t get_block_size(prosper::Format format);
	uint32_t get_component_count(prosper::Format format);
	struct GliTexture;
	struct GliTextureWrapper {
		GliTextureWrapper();
		GliTextureWrapper(uint32_t width, uint32_t height, prosper::Format format, uint32_t numLevels);
		~GliTextureWrapper();
		std::shared_ptr<GliTexture> texture;

		bool load(const char *data, size_t size);
		size_t size() const;
		size_t size(uint32_t imimap) const;
		uint32_t layers() const;
		uint32_t faces() const;
		uint32_t levels() const;
		prosper::Format format() const;
		void extent(uint32_t layer, uint32_t &outWidth, uint32_t &outHeight) const;
		void extent(uint32_t &outWidth, uint32_t &outHeight) const;
		void *data(uint32_t layer, uint32_t face, uint32_t mipmap);
		const void *data(uint32_t layer, uint32_t face, uint32_t mipmap) const;

		bool save_dds(const std::string &fileName) const;
		bool save_ktx(const std::string &fileName) const;
	};
};

#endif
