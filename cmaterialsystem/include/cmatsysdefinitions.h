/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __CMATSYSDEFINITIONS_H__
#define __CMATSYSDEFINITIONS_H__

#ifdef DLLCMATSYS_EX
	#ifdef __linux__
		#define DLLCMATSYS __attribute__((visibility("default")))
	#else
		#define DLLCMATSYS  __declspec(dllexport)
	#endif
#else
	#ifdef __linux__
		#define DLLCMATSYS
	#else
		#define DLLCMATSYS  __declspec(dllimport)
	#endif
#endif

#endif