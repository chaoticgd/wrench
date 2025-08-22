/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2021 chaoticgd

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

#include "buffer.h"

#include <stdarg.h>

Buffer Buffer::subbuf(s64 offset) const
{
	verify(offset >= 0, "Failed to create buffer: Offset cannot be negative.");
	verify(lo + offset <= hi, "Failed to create buffer: Out of bounds.");
	return Buffer(lo + offset, hi);
}

Buffer Buffer::subbuf(s64 offset, s64 new_size) const
{
	verify(offset >= 0, "Failed to create buffer: Offset cannot be negative.");
	verify(lo + offset + new_size <= hi, "Failed to create buffer: Out of bounds.");
	return Buffer(lo + offset, lo + offset + new_size);
}

std::vector<u8> Buffer::read_bytes(s64 offset, s64 size, const char* subject) const
{
	return read_multiple<u8>(offset, size, subject).copy();
}

std::string Buffer::read_string(s64 offset, bool is_korean) const
{
	verify(offset >= 0, "Failed to read string: Offset cannot be negative.");
	verify(lo + offset <= hi, "Failed to read string: Attempted to read past end of buffer.");
	std::string result;
	if (!is_korean) {
		for (const u8* ptr = lo + offset; ptr < hi && *ptr != '\0'; ptr++) {
			result += *ptr;
		}
	} else {
		// HACK: I'm not sure what this character encoding is, but I'm
		// pretty sure this isn't the correct way to parse it. Have fun with
		// data corruption down the road thanks to this!
		for (const u8* ptr = lo + offset; ptr < hi && *ptr != '\0'; ptr++) {
			result += *ptr;
			if ((*ptr == 0x14 || *ptr == 0x38 || *ptr == 0x61)
				&& ptr + 2 < hi && ptr[1] == '\0' && ptr[2] == '\0') {
				result += *(++ptr);
				result += *(++ptr);
			}
		}
	}
	return result;
}

std::string Buffer::read_fixed_string(s64 offset, s64 size) const
{
	verify(offset >= 0, "Failed to read string: Offset cannot be negative.");
	verify(lo + offset + size <= hi, "Failed to read string: Attempted to read past end of buffer.");
	std::string string;
	string.resize(size);
	memcpy(string.data(), lo + offset, size);
	return string;
}

void Buffer::hexdump(FILE* file, s64 column, const char* ansi_colour_code) const
{
	fprintf(file, "\033[%sm", ansi_colour_code);
	for (s64 i = 0; i < size(); i++) {
		fprintf(file, "%02hhx", lo[i]);
		if ((i + column) % 0x10 == 0xf) {
			fprintf(file, "\n");
		}
	}
	fprintf(file, "\033[0m");
}

bool diff_buffers(Buffer lhs, Buffer rhs, s64 offset, s64 size, bool print_diff, const std::vector<ByteRange64>* ignore_list)
{
	if (size == DIFF_REST_OF_BUFFER) {
		lhs = lhs.subbuf(offset);
		rhs = rhs.subbuf(offset);
	} else {
		lhs = lhs.subbuf(offset, size);
		rhs = rhs.subbuf(offset, size);
	}
	
	s64 min_size = std::min(lhs.size(), rhs.size());
	s64 max_size = std::max(lhs.size(), rhs.size());
	
	// Determine which bytes should be ignored.
	std::vector<bool> ignore;
	if (ignore_list) {
		for (ByteRange64 range : *ignore_list) {
			range.offset -= offset;
			if (ignore.size() < range.offset + range.size) {
				ignore.resize(range.offset + range.size, false);
			}
			for (s64 i = 0; i < range.size; i++) {
				ignore[range.offset + i] = true;
			}
		}
	}
	
	// Figure out if the buffers are equal. If they are not, find the first byte
	// that doesn't match.
	s64 diff_pos = -1;
	for (s64 i = 0; i < min_size; i++) {
		if (lhs[i] != rhs[i]) {
			if (ignore.size() <= i || !ignore[i]) {
				diff_pos = i;
				break;
			}
		}
	}
	if (diff_pos == -1) {
		if (lhs.size() == rhs.size()) {
			return true;
		} else {
			diff_pos = min_size;
		}
	}
	
	if (!print_diff) {
		return false;
	}
	
	s64 row_start = (diff_pos / 0x10) * 0x10;
	s64 hexdump_begin = std::max((s64) 0, row_start - 0x50);
	s64 hexdump_end = max_size;
	for (s64 i = hexdump_begin; i < hexdump_end; i += 0x10) {
		printf("%08x: ", (s32) (offset + i));
		for (Buffer current : {lhs, rhs}) {
			for (s64 j = 0; j < 0x10; j++) {
				s64 pos = i + j;
				const char* colour = nullptr;
				if (lhs.in_bounds(pos) && rhs.in_bounds(pos)) {
					if (lhs[pos] == rhs[pos]) {
						colour = "32";
					} else if (ignore.size() > pos && ignore[pos]) {
						colour = "36";
					} else {
						colour = "31";
					}
				} else if (current.in_bounds(pos)) {
					colour = "33";
				} else {
					printf("   ");
				}
				if (colour != nullptr) {
					printf("\033[%sm%02x\033[0m ", colour, current[pos]);
				}
				if (j % 4 == 3 && j != 0xf) {
					printf(" ");
				}
			}
			printf("| ");
		}
		printf("\n");
	}
	printf("\n");
	
	return false;
}

void OutBuffer::pad(s64 align, u8 padding)
{
	if (vec.size() % align != 0) {
		s64 pad_size = align - (vec.size() % align);
		vec.resize(vec.size() + pad_size, padding);
	}
}

void OutBuffer::writesf(s32 indent_level, const char* format, va_list args)
{
	static char temp[16 * 1024];
	
	for (s32 i = 0; i < indent_level; i++) {
		temp[i] = '\t';
	}
	
	s32 count = vsnprintf(temp + indent_level, 16 * 1024 - indent_level, format, args);
	verify_fatal(count >= 0);
	count += indent_level;
	
	size_t write_ofs = vec.size();
	vec.resize(vec.size() + count);
	memcpy(vec.data() + write_ofs, temp, count);
}

void OutBuffer::writelf(s32 indent_level, const char* format, va_list args)
{
	static char temp[16 * 1024];
	
	for (s32 i = 0; i < indent_level; i++) {
		temp[i] = '\t';
	}
	
	s32 count = vsnprintf(temp + indent_level, 16 * 1024 - indent_level, format, args);
	verify_fatal(count >= 0);
	count += indent_level;
	
	size_t write_ofs = vec.size();
	vec.resize(write_ofs + count + 1);
	memcpy(vec.data() + write_ofs, temp, count);
	vec[write_ofs + count] = '\n';
}

void OutBuffer::writesf(s32 indent_level, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	writesf(indent_level, format, args);
	va_end(args);
}

void OutBuffer::writelf(s32 indent_level, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	writelf(indent_level, format, args);
	va_end(args);
}

void OutBuffer::writesf(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	writesf(0, format, args);
	va_end(args);
}

void OutBuffer::writelf(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	writelf(0, format, args);
	va_end(args);
}
