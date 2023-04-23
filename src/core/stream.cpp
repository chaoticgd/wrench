
#include "stream.h"

#if defined(_WIN32) || defined(WIN32)
// std::max won't work otherwise because windows.h defines macros max and min
#ifndef NOMINMAX
#define NOMINMAX 1
#endif /* NOMINMAX */

#include <Windows.h>
#include <fileapi.h>
#include <io.h>
#endif

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
	return true;
}

s64 BlackHoleOutputStream::tell() const {
	return ofs;
}

s64 BlackHoleOutputStream::size() const {
	return top;
}

bool BlackHoleOutputStream::write_n(const u8*, s64 size) {
	ofs += size;
	top = std::max(top, ofs);
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
	return true;
}

s64 MemoryInputStream::tell() const {
	return ofs;
}
s64 MemoryInputStream::size() const {
	return end - begin;
}

bool MemoryInputStream::read_n(u8* dest, s64 size) {
	verify(ofs + size <= end - begin, "Tried to read past end of memory input stream.");
	memcpy(dest, begin + ofs, size);
	ofs += size;
	return true;
}

// *****************************************************************************

MemoryOutputStream::MemoryOutputStream(std::vector<u8>& backing_)
	: backing(backing_) {}

bool MemoryOutputStream::seek(s64 offset) {
	ofs = offset;
	return true;
}

s64 MemoryOutputStream::tell() const {
	return ofs;
}

s64 MemoryOutputStream::size() const {
	return backing.size();
}

bool MemoryOutputStream::write_n(const u8* src, s64 size) {
	if(ofs + size > backing.size()) {
		backing.resize(ofs + size);
	}
	memcpy(&backing[ofs], src, size);
	ofs += size;
	return true;
}

// *****************************************************************************

FileInputStream::FileInputStream() {}

FileInputStream::~FileInputStream() {
	if(file != nullptr) {
		fclose(file);
	}
}

bool FileInputStream::open(const fs::path& path) {
	if(file != nullptr) {
		fclose(file);
	}
	file = fopen(path.string().c_str(), "rb");
	return file != nullptr;
}

bool FileInputStream::seek(s64 offset) {
	return fseek(file, offset, SEEK_SET) == 0;
}

s64 FileInputStream::tell() const {
	return ftell(file);
}

s64 FileInputStream::size() const {
	s64 size_val;

	// SEEK_END is not required to be implemented in a meaningful away according to
	// the C90 standard. On Windows, the UCRT does not implement SEEK_END for the
	// standard fseek function but only for _fseeki64. This approach is not
	// necessary given that the correct fseek version is used but this approach
	// should always work on Windows and hence it is safer.
#if defined(_WIN32) || defined(WIN32)
	int fd = _fileno(file);
	HANDLE hfile = (HANDLE) _get_osfhandle(fd);
	FlushFileBuffers(hfile);

	LARGE_INTEGER size;
	bool success = GetFileSizeEx(hfile, &size);
	DWORD err = GetLastError();

	verify(success, "Failed to retrieve file size. WinAPI Error Code: '%d'.", err);

	size_val = (s64) size.QuadPart;
#else
	s64 offset = ftell(file);
	fseek(file, 0, SEEK_END);
	size_val = ftell(file);
	fseek(file, offset, SEEK_SET);
#endif
	return size_val;
}

bool FileInputStream::read_n(u8* dest, s64 size) {
	return fread(dest, size, 1, file) == 1;
}

// *****************************************************************************

FileOutputStream::FileOutputStream() {}

FileOutputStream::~FileOutputStream() {
	if(file != nullptr) {
		fclose(file);
	}
}

bool FileOutputStream::open(const fs::path& path) {
	if(file != nullptr) {
		fclose(file);
	}
	file = fopen(path.string().c_str(), "wb");
	return file != nullptr;
}

bool FileOutputStream::seek(s64 offset) {
	return fseek(file, offset, SEEK_SET) == 0;
}

s64 FileOutputStream::tell() const {
	return ftell(file);
}

s64 FileOutputStream::size() const {
	s64 size_val;

	// SEEK_END is not required to be implemented in a meaningful away according to
	// the C90 standard. On Windows, the UCRT does not implement SEEK_END for the
	// standard fseek function but only for _fseeki64. This approach is not
	// necessary given that the correct fseek version is used but this approach
	// should always work on Windows and hence it is safer.
#if defined(_WIN32) || defined(WIN32)
	int fd = _fileno(file);
	HANDLE hfile = (HANDLE) _get_osfhandle(fd);
	FlushFileBuffers(hfile);

	LARGE_INTEGER size;
	bool success = GetFileSizeEx(hfile, &size);
	DWORD err = GetLastError();

	verify(success, "Failed to retrieve file size. WinAPI Error Code: '%d'.", err);

	size_val = (s64) size.QuadPart;
#else
	s64 offset = ftell(file);
	fseek(file, 0, SEEK_END);
	size_val = ftell(file);
	fseek(file, offset, SEEK_SET);
#endif
	return size_val;
}

bool FileOutputStream::write_n(const u8* src, s64 size) {
	return fwrite(src, size, 1, file) == 1;
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
