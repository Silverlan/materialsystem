/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __TEXTURE_LOAD_FLAGS_HPP__
#define __TEXTURE_LOAD_FLAGS_HPP__

#include <mathutil/umath.h>

enum class TextureLoadFlags : uint32_t
{
	None = 0u,
	LoadInstantly = 1u,
	Reload = LoadInstantly<<1u,
	DontCache = Reload<<1u
};
REGISTER_BASIC_BITWISE_OPERATORS(TextureLoadFlags);

#endif
