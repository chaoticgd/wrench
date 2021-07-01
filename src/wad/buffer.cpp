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

Buffer Buffer::subbuf(s64 offset) const {
	verify(offset >= 0, "Failed to create buffer: Offset cannot be negative.");
	verify(lo + offset <= hi, "Failed to create buffer: Out of bounds.");
	return Buffer(lo + offset, hi);
}

Buffer Buffer::subbuf(s64 offset, s64 new_size) const {
	verify(offset >= 0, "Failed to create buffer: Offset cannot be negative.");
	verify(lo + offset + new_size <= hi, "Failed to create buffer: Out of bounds.");
	return Buffer(lo + offset, lo + offset + new_size);
}

std::string Buffer::read_string(s64 offset, bool is_korean) const {
	verify(offset > 0, "Failed to read string: Offset cannot be negative.");
	verify(lo + offset <= hi, "Failed to read string: Attempted to read past end of buffer.");
	std::string result;
	if(!is_korean) {
		for(const uint8_t* ptr = lo + offset; ptr < hi && *ptr != '\0'; ptr++) {
			result += *ptr;
		}
	} else {
		// HACK: I'm not sure what this character encoding is, but I'm
		// pretty sure this isn't the correct way to parse it. Have fun with
		// data corruption down the road thanks to this!
		for(const uint8_t* ptr = lo + offset; ptr < hi && *ptr != '\0'; ptr++) {
			result += *ptr;
			if((*ptr == 0x14 || *ptr == 0x38 || *ptr == 0x61)
				&& ptr + 2 < hi && ptr[1] == '\0' && ptr[2] == '\0') {
				result += *(++ptr);
				result += *(++ptr);
			}
		}
	}
	return result;
}

void Buffer::hexdump(FILE* file, s64 column, const char* ansi_colour_code) const {
	fprintf(file, "\033[%sm", ansi_colour_code);
	for(s64 i = 0; i < size(); i++) {
		fprintf(file, "%02hhx", lo[i]);
		if((i + column) % 0x10 == 0xf) {
			fprintf(file, "\n");
		}
	}
	fprintf(file, "\033[0m");
}

bool diff_buffers(Buffer lhs, Buffer rhs, s64 offset, const char* subject) {
	s64 min_size = std::min(lhs.size(), rhs.size());
	s64 max_size = std::max(lhs.size(), rhs.size());
	s64 diff_pos = -1;
	for(s64 i = 0; i < min_size; i++) {
		if(lhs[i] != rhs[i]) {
			diff_pos = i;
			break;
		}
	}
	if(diff_pos == -1) {
		if(lhs.size() == rhs.size()) {
			printf("%s buffers equal.\n", subject);
			return true;
		} else {
			diff_pos = min_size;
		}
	}
	printf("%s buffers differ.\n", subject);
	s64 row_start = (diff_pos / 0x10) * 0x10;
	s64 hexdump_begin = std::max((s64) 0, row_start - 0x50);
	s64 hexdump_end = max_size;//std::min(max_size, row_start + 0xa0);
	for(s64 i = hexdump_begin; i < hexdump_end; i += 0x10) {
		printf("%08x: ", (s32) (offset + i));
		for(Buffer current : {lhs, rhs}) {
			for(s64 j = 0; j < 0x10; j++) {
				s64 pos = i + j;
				const char* colour = nullptr;
				if(lhs.in_bounds(pos) && rhs.in_bounds(pos)) {
					if(lhs[pos] == rhs[pos]) {
						colour = "32";
					} else {
						colour = "31";
					}
				} else if(current.in_bounds(pos)) {
					colour = "33";
				} else {
					printf("   ");
				}
				if(colour != nullptr) {
					printf("\033[%sm%02x\033[0m ", colour, current[pos]);
				}
				if(j % 4 == 3 && j != 0xf) {
					printf(" ");
				}
			}
			printf("| ");
		}
		printf("\n");
	}
	return false;
}
