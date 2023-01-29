/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

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
