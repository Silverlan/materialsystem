// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __TEXTURE_LOAD_FLAGS_HPP__
#define __TEXTURE_LOAD_FLAGS_HPP__

#include <mathutil/umath.h>

enum class TextureLoadFlags : uint32_t { None = 0u, LoadInstantly = 1u, Reload = LoadInstantly << 1u, DontCache = Reload << 1u };
REGISTER_BASIC_BITWISE_OPERATORS(TextureLoadFlags);

#endif
