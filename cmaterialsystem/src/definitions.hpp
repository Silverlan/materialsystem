// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#pragma once

#ifdef DLLCMATSYS_EX
#ifdef __linux__
#define DLLCMATSYS __attribute__((visibility("default")))
#else
#define DLLCMATSYS __declspec(dllexport)
#endif
#else
#ifdef __linux__
#define DLLCMATSYS
#else
#define DLLCMATSYS __declspec(dllimport)
#endif
#endif
