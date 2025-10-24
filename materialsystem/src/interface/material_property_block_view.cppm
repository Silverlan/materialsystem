// SPDX-FileCopyrightText: (c) 2025 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

module;

#include "matsysdefinitions.hpp"
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <string>
#include <string_view>

export module pragma.materialsystem:material_property_block_view;

export import :material;

export namespace msys {
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

			bool operator==(const Iterator &b) const { return it_ == b.it_; }
			bool operator!=(const Iterator &b) const { return it_ != b.it_; }
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
