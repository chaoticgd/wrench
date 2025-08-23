/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2024 chaoticgd

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include "stream.h"

Stream::~Stream() {}

void Stream::copy(OutputStream& dest, InputStream& src, s64 size)
{
	static const s64 chunk_size = 64 * 1024;
	static std::vector<u8> buffer(chunk_size);
	for (s64 i = 0; i < size / chunk_size; i++) {
		src.read_n(buffer.data(), chunk_size);
		dest.write_n(buffer.data(), chunk_size);
	}
	s64 last_chunk_size = size % chunk_size;
	src.read_n(buffer.data(), last_chunk_size);
	dest.write_n(buffer.data(), last_chunk_size);
}

// *****************************************************************************

void OutputStream::pad(s64 alignment, u8 padding)
{
	s64 pos = tell();
	if (pos % alignment != 0) {
		for (s64 i = 0; i < alignment - (pos % alignment); i++) {
			write<u8>(padding);
		}
	}
}

// *****************************************************************************

BlackHoleOutputStream::BlackHoleOutputStream() {}

bool BlackHoleOutputStream::seek(s64 offset)
{
	m_ofs = offset;
	last_error.clear();
	return true;
}

s64 BlackHoleOutputStream::tell() const
{
	last_error.clear();
	return m_ofs;
}

s64 BlackHoleOutputStream::size() const
{
	last_error.clear();
	return m_top;
}

bool BlackHoleOutputStream::write_n(const u8*, s64 size)
{
	m_ofs += size;
	m_top = std::max(m_top, m_ofs);
	last_error.clear();
	return true;
}

// *****************************************************************************

MemoryInputStream::MemoryInputStream(const u8* begin, const u8* end)
	: m_begin(begin)
	, m_end(end) {}

MemoryInputStream::MemoryInputStream(const std::vector<u8>& bytes)
	: m_begin(bytes.data())
	, m_end(bytes.data() + bytes.size()) {}

bool MemoryInputStream::seek(s64 offset)
{
	m_ofs = offset;
	last_error.clear();
	return true;
}

s64 MemoryInputStream::tell() const
{
	last_error.clear();
	return m_ofs;
}
s64 MemoryInputStream::size() const
{
	last_error.clear();
	return m_end - m_begin;
}

bool MemoryInputStream::read_n(u8* dest, s64 size)
{
	verify(m_ofs + size <= m_end - m_begin, "Tried to read past end of memory input stream.");
	memcpy(dest, m_begin + m_ofs, size);
	m_ofs += size;
	last_error.clear();
	return true;
}

// *****************************************************************************

MemoryOutputStream::MemoryOutputStream(std::vector<u8>& backing_)
	: m_backing(backing_) {}

bool MemoryOutputStream::seek(s64 offset)
{
	m_ofs = offset;
	last_error.clear();
	return true;
}

s64 MemoryOutputStream::tell() const
{
	last_error.clear();
	return m_ofs;
}

s64 MemoryOutputStream::size() const
{
	last_error.clear();
	return m_backing.size();
}

bool MemoryOutputStream::write_n(const u8* src, s64 size)
{
	if (m_ofs + size > m_backing.size()) {
		m_backing.resize(m_ofs + size);
	}
	memcpy(m_backing.data() + m_ofs, src, size);
	m_ofs += size;
	last_error.clear();
	return true;
}

// *****************************************************************************

FileInputStream::FileInputStream() {}

FileInputStream::~FileInputStream()
{
	if (m_file != nullptr) {
		file_close(m_file);
	}
}

bool FileInputStream::open(const fs::path& path)
{
	if (m_file != nullptr) {
		if (file_close(m_file) == EOF) {
			last_error = FILEIO_ERROR_CONTEXT_STRING;
			return false;
		}
	}
	m_file = file_open(path.string().c_str(), WRENCH_FILE_MODE_READ);
	last_error = FILEIO_ERROR_CONTEXT_STRING;
	return m_file != nullptr;
}

bool FileInputStream::seek(s64 offset)
{
	int result = file_seek(m_file, offset, WRENCH_FILE_ORIGIN_START);
	last_error = FILEIO_ERROR_CONTEXT_STRING;
	return result == 0;
}

s64 FileInputStream::tell() const
{
	size_t pos = file_tell(m_file);
	last_error = FILEIO_ERROR_CONTEXT_STRING;
	return pos;
}

s64 FileInputStream::size() const
{
	size_t result = file_size(m_file);
	last_error = FILEIO_ERROR_CONTEXT_STRING;
	return result;
}

bool FileInputStream::read_n(u8* dest, s64 size)
{
	size_t result = file_read(dest, size, m_file);
	last_error = FILEIO_ERROR_CONTEXT_STRING;
	return result == size;
}

// *****************************************************************************

FileOutputStream::FileOutputStream() {}

FileOutputStream::~FileOutputStream()
{
	if (m_file != nullptr) {
		file_close(m_file);
	}
}

bool FileOutputStream::open(const fs::path& path)
{
	if (m_file != nullptr) {
		file_close(m_file);
	}
	m_file = file_open(path.string().c_str(), WRENCH_FILE_MODE_WRITE);
	last_error = FILEIO_ERROR_CONTEXT_STRING;
	return m_file != nullptr;
}

bool FileOutputStream::seek(s64 offset)
{
	int result = file_seek(m_file, offset, WRENCH_FILE_ORIGIN_START);
	last_error = FILEIO_ERROR_CONTEXT_STRING;
	return result == 0;
}

s64 FileOutputStream::tell() const
{
	size_t result = file_tell(m_file);
	last_error = FILEIO_ERROR_CONTEXT_STRING;
	return result;
}

s64 FileOutputStream::size() const
{
	size_t result = file_size(m_file);
	last_error = FILEIO_ERROR_CONTEXT_STRING;
	return result;
}

bool FileOutputStream::write_n(const u8* src, s64 size)
{
	size_t result = file_write(src, size, m_file);
	last_error = FILEIO_ERROR_CONTEXT_STRING;
	return result == size;
}

// *****************************************************************************

SubInputStream::SubInputStream(InputStream& stream, ByteRange64 range)
	: m_stream(stream)
	, m_range(range) {}

SubInputStream::SubInputStream(InputStream& stream, s64 base, s64 bytes)
	: m_stream(stream)
	, m_range{base, bytes} {
	verify(m_range.offset + m_range.size <= stream.size(), "Tried to create out of range substream.");
}

bool SubInputStream::seek(s64 offset)
{
	return m_stream.seek(m_range.offset + offset);
}

s64 SubInputStream::tell() const
{
	return m_stream.tell() - m_range.offset;
}

s64 SubInputStream::size() const
{
	return m_range.size;
}

bool SubInputStream::read_n(u8* dest, s64 size)
{
	verify(m_stream.tell() + size <= m_range.offset + m_range.size,
		"Tried to read past end of substream of size %lx from suboffset %lx.",
		m_range.size, tell());
	return m_stream.read_n(dest, size);
}

s64 SubInputStream::offset_relative_to(InputStream* outer) const
{
	if (outer == &m_stream) {
		return m_range.offset;
	} else if (SubInputStream* sub_stream = dynamic_cast<SubInputStream*>(&m_stream)) {
		return m_range.offset + sub_stream->offset_relative_to(outer);
	} else {
		return 0;
	}
}

// *****************************************************************************

SubOutputStream::SubOutputStream(OutputStream& stream_, s64 zero_)
	: m_stream(stream_), m_zero(zero_) {}

bool SubOutputStream::seek(s64 offset)
{
	return m_stream.seek(m_zero + offset);
}

s64 SubOutputStream::tell() const
{
	return m_stream.tell() - m_zero;
}

s64 SubOutputStream::size() const
{
	return m_stream.size() - m_zero;
}

bool SubOutputStream::write_n(const u8* src, s64 size)
{
	return m_stream.write_n(src, size);
}
