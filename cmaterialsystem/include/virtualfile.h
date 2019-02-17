/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __VIRTUALFILE_H__
#define __VIRTUALFILE_H__

#include "cmatsysdefinitions.h"
#include <vector>
#include <cinttypes>
#ifdef __linux__
#include <stdio.h>
#endif

class DLLCMATSYS VirtualFile
{
public:
	enum class SeekMode : uint32_t
	{
		Set = SEEK_SET,
		Cur = SEEK_CUR,
		End = SEEK_END
	};
private:
	std::vector<uint8_t> m_data;
	size_t m_position;
public:
	VirtualFile();
	size_t Read(void *buf,size_t bytes);
	size_t Seek(size_t offset,SeekMode mode=SeekMode::Set);
	size_t GetSize() const;
	size_t Tell() const;
	std::vector<uint8_t> &GetData();
	bool Eof() const;
};

#endif
