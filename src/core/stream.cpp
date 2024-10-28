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

void Stream::copy(OutputStream& dest, InputStream& src, s64 size) {
	static const s64 chunk_size = 64 * 1024;
	static std::vector<u8> buffer(chunk_size);
	for(s64 i = 0; i < size / chunk_size; i++) {
		src.read_n(buffer.data(), chunk_size);
		dest.write_n(buffer.data(), chunk_size);
	}
	s64 last_chunk_size = size % chunk_size;
	src.read_n(buffer.data(), last_chunk_size);
	dest.write_n(buffer.data(), last_chunk_size);
}

// *****************************************************************************

void OutputStream::pad(s64 alignment, u8 padding) {
	s64 pos = tell();
	if(pos % alignment != 0) {
		for(s64 i = 0; i < alignment - (pos % alignment); i++) {
			write<u8>(padding);
		}
	}
}

// *****************************************************************************

BlackHoleOutputStream::BlackHoleOutputStream() {}

bool BlackHoleOutputStream::seek(s64 offset) {
	ofs = offset;
	last_error.clear();
	return true;
}

s64 BlackHoleOutputStream::tell() const {
	last_error.clear();
	return ofs;
}

s64 BlackHoleOutputStream::size() const {
	last_error.clear();
	return top;
}

bool BlackHoleOutputStream::write_n(const u8*, s64 size) {
	ofs += size;
	top = std::max(top, ofs);
	last_error.clear();
	return true;
}

// *****************************************************************************

MemoryInputStream::MemoryInputStream(const u8* begin_, const u8* end_)
	: begin(begin_)
	, end(end_) {}

MemoryInputStream::MemoryInputStream(const std::vector<u8>& bytes)
	: begin(bytes.data())
	, end(bytes.data() + bytes.size()) {}

bool MemoryInputStream::seek(s64 offset) {
	ofs = offset;
	last_error.clear();
	return true;
}

s64 MemoryInputStream::tell() const {
	last_error.clear();
	return ofs;
}
s64 MemoryInputStream::size() const {
	last_error.clear();
	return end - begin;
}

bool MemoryInputStream::read_n(u8* dest, s64 size) {
	verify(ofs + size <= end - begin, "Tried to read past end of memory input stream.");
	memcpy(dest, begin + ofs, size);
	ofs += size;
	last_error.clear();
	return true;
}

// *****************************************************************************

MemoryOutputStream::MemoryOutputStream(std::vector<u8>& backing_)
	: backing(backing_) {}

bool MemoryOutputStream::seek(s64 offset) {
	ofs = offset;
	last_error.clear();
	return true;
}

s64 MemoryOutputStream::tell() const {
	last_error.clear();
	return ofs;
}

s64 MemoryOutputStream::size() const {
	last_error.clear();
	return backing.size();
}

bool MemoryOutputStream::write_n(const u8* src, s64 size) {
	if(ofs + size > backing.size()) {
		backing.resize(ofs + size);
	}
	memcpy(&backing[ofs], src, size);
	ofs += size;
	last_error.clear();
	return true;
}

// *****************************************************************************

FileInputStream::FileInputStream() {}

FileInputStream::~FileInputStream() {
	if(file != nullptr) {
		file_close(file);
	}
}

bool FileInputStream::open(const fs::path& path) {
	if(file != nullptr) {
		if(file_close(file) == EOF) {
			last_error = FILEIO_ERROR_CONTEXT_STRING;
			return false;
		}
	}
	file = file_open(path.string().c_str(), WRENCH_FILE_MODE_READ);
	last_error = FILEIO_ERROR_CONTEXT_STRING;
	return file != nullptr;
}

bool FileInputStream::seek(s64 offset) {
	int result = file_seek(file, offset, WRENCH_FILE_ORIGIN_START);
	last_error = FILEIO_ERROR_CONTEXT_STRING;
	return result == 0;
}

s64 FileInputStream::tell() const {
	size_t pos = file_tell(file);
	last_error = FILEIO_ERROR_CONTEXT_STRING;
	return pos;
}

s64 FileInputStream::size() const {
	size_t result = file_size(file);
	last_error = FILEIO_ERROR_CONTEXT_STRING;
	return result;
}

bool FileInputStream::read_n(u8* dest, s64 size) {
	size_t result = file_read(dest, size, file);
	last_error = FILEIO_ERROR_CONTEXT_STRING;
	return result == size;
}

// *****************************************************************************

FileOutputStream::FileOutputStream() {}

FileOutputStream::~FileOutputStream() {
	if(file != nullptr) {
		file_close(file);
	}
}

bool FileOutputStream::open(const fs::path& path) {
	if(file != nullptr) {
		file_close(file);
	}
	file = file_open(path.string().c_str(), WRENCH_FILE_MODE_WRITE);
	last_error = FILEIO_ERROR_CONTEXT_STRING;
	return file != nullptr;
}

bool FileOutputStream::seek(s64 offset) {
	int result = file_seek(file, offset, WRENCH_FILE_ORIGIN_START);
	last_error = FILEIO_ERROR_CONTEXT_STRING;
	return result == 0;
}

s64 FileOutputStream::tell() const {
	size_t result = file_tell(file);
	last_error = FILEIO_ERROR_CONTEXT_STRING;
	return result;
}

s64 FileOutputStream::size() const {
	size_t result = file_size(file);
	last_error = FILEIO_ERROR_CONTEXT_STRING;
	return result;
}

bool FileOutputStream::write_n(const u8* src, s64 size) {
	size_t result = file_write(src, size, file);
	last_error = FILEIO_ERROR_CONTEXT_STRING;
	return result == size;
}

// *****************************************************************************

SubInputStream::SubInputStream(InputStream& stream_, ByteRange64 range_)
	: stream(stream_)
	, range(range_) {}

SubInputStream::SubInputStream(InputStream& stream_, s64 base_, s64 bytes_)
	: stream(stream_)
	, range{base_, bytes_} {
	verify(range.offset + range.size <= stream.size(), "Tried to create out of range substream.");
}

bool SubInputStream::seek(s64 offset) {
	return stream.seek(range.offset + offset);
}

s64 SubInputStream::tell() const {
	return stream.tell() - range.offset;
}

s64 SubInputStream::size() const {
	return range.size;
}

bool SubInputStream::read_n(u8* dest, s64 size) {
	verify(stream.tell() + size <= range.offset + range.size,
		"Tried to read past end of substream of size %lx from suboffset %lx.",
		range.size, tell());
	return stream.read_n(dest, size);
}

s64 SubInputStream::offset_relative_to(InputStream* outer) const {
	if(outer == &stream) {
		return range.offset;
	} else if(SubInputStream* sub_stream = dynamic_cast<SubInputStream*>(&stream)) {
		return range.offset + sub_stream->offset_relative_to(outer);
	} else {
		return 0;
	}
}

// *****************************************************************************

SubOutputStream::SubOutputStream(OutputStream& stream_, s64 zero_)
	: stream(stream_), zero(zero_) {}

bool SubOutputStream::seek(s64 offset) {
	return stream.seek(zero + offset);
}

s64 SubOutputStream::tell() const {
	return stream.tell() - zero;
}

s64 SubOutputStream::size() const {
	return stream.size() - zero;
}

bool SubOutputStream::write_n(const u8* src, s64 size) {
	return stream.write_n(src, size);
}
