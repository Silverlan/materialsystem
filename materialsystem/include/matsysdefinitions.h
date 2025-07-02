// SPDX-FileCopyrightText: (c) 2019 Silverlan <opensource@pragma-engine.com>
// SPDX-License-Identifier: MIT

#ifndef __MATSYSDEFINITIONS_H__
#define __MATSYSDEFINITIONS_H__

#ifdef MATSYS_DLL
#ifdef __linux__
#define DLLMATSYS __attribute__((visibility("default")))
#else
#define DLLMATSYS __declspec(dllexport)
#endif
#else
#ifndef MATSYS_EXE
#ifndef MATSYS_LIB
#ifdef __linux__
#define DLLMATSYS
#else
#define DLLMATSYS __declspec(dllimport)
#endif
#else
#define DLLMATSYS
#endif
#else
#define DLLMATSYS
#endif
#endif

#endif
