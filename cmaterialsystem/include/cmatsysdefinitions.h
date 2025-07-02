// SPDX-FileCopyrightText: © 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __CMATSYSDEFINITIONS_H__
#define __CMATSYSDEFINITIONS_H__

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

#endif
