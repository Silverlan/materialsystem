/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __MSYS_MATERIAL_PROPERTY_BLOCK_VIEW_HPP__
#define __MSYS_MATERIAL_PROPERTY_BLOCK_VIEW_HPP__

#include "material.h"
#include <unordered_map>
#include <unordered_set>
#include <string>
#include <string_view>
#include <sharedutils/util_path.hpp>

namespace msys {
	struct DLLMATSYS MaterialPropertyBlockView {
		struct DLLMATSYS IterationData {
			IterationData(MaterialPropertyBlockView &blockView);
			MaterialPropertyBlockView *blockView;
			std::unordered_map<std::string_view, ds::Base *> map;
		};

		MaterialPropertyBlockView(Material &mat, const util::Path &path = {});

		// Nested iterator class that iterates only over the keys.
		class DLLMATSYS Iterator {
		  public:
			// Underlying map iterator type.
			using map_iterator = std::unordered_map<std::string_view, ds::Base *>::const_iterator;
			using iterator_category = std::forward_iterator_tag;
			using value_type = std::string_view;
			using difference_type = std::ptrdiff_t;
			using pointer = const std::string_view *;
			using reference = const std::string_view;

			explicit Iterator(map_iterator it) : it_(it) {}

			// Dereference returns the key (std::string_view)
			reference operator*() const { return it_->first; }
			pointer operator->() const { return &(it_->first); }

			// Pre-increment operator.
			Iterator &operator++()
			{
				++it_;
				return *this;
			}

			// Post-increment operator.
			Iterator operator++(int)
			{
				Iterator tmp(*this);
				++(*this);
				return tmp;
			}

			friend bool operator==(const Iterator &a, const Iterator &b) { return a.it_ == b.it_; }
			friend bool operator!=(const Iterator &a, const Iterator &b) { return a.it_ != b.it_; }
		  private:
			map_iterator it_;
		};

		// Begin and end functions for range-based for loop.
		Iterator begin() const { return Iterator(iterationData->map.cbegin()); }
		Iterator end() const { return Iterator(iterationData->map.cend()); }
	  private:
		std::vector<ds::Block *> blocks;
		std::unique_ptr<IterationData> iterationData;
	};
};

#endif
