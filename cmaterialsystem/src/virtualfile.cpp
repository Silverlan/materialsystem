/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "virtualfile.h"
#include <mathutil/umath.h>
#include <cstring>

VirtualFile::VirtualFile()
	: m_position(0)
{}

size_t VirtualFile::Read(void *buf,size_t bytes)
{
	auto pos = m_position;
	auto sz = GetSize();
	auto readBytes = umath::min(sz -pos,bytes);
	if(readBytes == 0)
		return 0;
	memcpy(buf,m_data.data() +m_position,readBytes);
	m_position = umath::min(m_position +readBytes,sz);
	return readBytes;
}
size_t VirtualFile::Seek(size_t offset,SeekMode mode)
{
	if(mode == SeekMode::Set)
		m_position = umath::min(offset,GetSize());
	else if(mode == SeekMode::End)
		m_position = GetSize();
	else
		m_position = umath::min(m_position +offset,GetSize());
	return m_position;
}
size_t VirtualFile::GetSize() const {return m_data.size();}
size_t VirtualFile::Tell() const {return m_position;}
std::vector<uint8_t> &VirtualFile::GetData() {return m_data;}
bool VirtualFile::Eof() const {return m_position >= m_data.size();}