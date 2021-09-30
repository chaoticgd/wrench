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

#include "stdarg.h"

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

std::vector<u8> Buffer::read_bytes(s64 offset, s64 size, const char* subject) const {
	return read_multiple<u8>(offset, size, subject).copy();
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

std::string Buffer::read_fixed_string(s64 offset, s64 size) const {
	verify(offset > 0, "Failed to read string: Offset cannot be negative.");
	verify(lo + offset + size <= hi, "Failed to read string: Attempted to read past end of buffer.");
	std::string string;
	string.resize(size);
	memcpy(string.data(), lo + offset, size);
	return string;
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

bool diff_buffers(Buffer lhs, Buffer rhs, s64 offset, const char* subject, s64 zero) {
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
		printf("%08x: ", (s32) (zero + offset + i));
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

void OutBuffer::pad(s64 align, u8 padding) {
	if(vec.size() % align != 0) {
		s64 pad_size = align - (vec.size() % align);
		vec.resize(vec.size() + pad_size, padding);
	}
}

void OutBuffer::writesf(const char* format, ...) {
	static char temp[16 * 1024];
	
	va_list args;
	va_start(args, format);
	int count = vsnprintf(temp, 16 * 1024, format, args);
	assert(count >= 0);
	va_end(args);
	
	size_t write_ofs = vec.size();
	vec.resize(vec.size() + count);
	memcpy(&vec[write_ofs], temp, count);
}

void OutBuffer::writelf(const char* format, ...) {
	static char temp[16 * 1024];
	
	va_list args;
	va_start(args, format);
	int count = vsnprintf(temp, 16 * 1024, format, args);
	assert(count >= 0);
	va_end(args);
	
	size_t write_ofs = vec.size();
	vec.resize(write_ofs + count + 1);
	memcpy(&vec[write_ofs], temp, count);
	vec[write_ofs + count] = '\n';
}

s64 file_size_in_bytes(FILE* file) {
	long whence_you_came = ftell(file);
	fseek(file, 0, SEEK_END);
	s64 ofs = ftell(file);
	fseek(file, whence_you_came, SEEK_SET);
	return ofs;
}

std::vector<u8> read_file(FILE* file, s64 offset, s64 size) {
	std::vector<u8> buffer(size);
	verify(fseek(file, offset, SEEK_SET) == 0, "Failed to seek.");
	if(buffer.size() > 0) {
		verify(fread(buffer.data(), buffer.size(), 1, file) == 1, "Failed to read file.");
	}
	return buffer;
}

std::vector<u8> read_file(fs::path path) {
	verify(!fs::is_directory(path), "Tried to open directory '%s' as regular file.", path.string().c_str());
	FILE* file = fopen(path.string().c_str(), "rb");
	verify(file, "Failed to open file '%s' for reading.", path.string().c_str());
	std::vector<u8> buffer(file_size_in_bytes(file));
	if(buffer.size() > 0) {
		verify(fread(buffer.data(), buffer.size(), 1, file) == 1, "Failed to read file '%s'.", path.string().c_str());
	}
	fclose(file);
	return buffer;
}

std::string write_file(fs::path dest_dir, fs::path rel_path, Buffer buffer) {
	fs::path dest_path = dest_dir/rel_path;
	FILE* file = fopen(dest_path.string().c_str(), "wb");
	verify(file, "Failed to open file '%s' for writing.", dest_path.string().c_str());
	if(buffer.size() > 0) {
		verify(fwrite(buffer.lo, buffer.size(), 1, file) == 1, "Failed to write output file '%s'.", dest_path.string().c_str());
	}
	fclose(file);
	if(buffer.size() < 1024) {
		//printf("Wrote %s (%ld bytes)\n", dest_path.string().c_str(), buffer.size());
	} else {
		//printf("Wrote %s (%ld KiB)\n", dest_path.string().c_str(), buffer.size() / 1024);
	}
	return rel_path.string();
}

void extract_file(fs::path dest_path, FILE* dest, FILE* src, s64 offset, s64 size) {
	static const s32 BUFFER_SIZE = 1024 * 1024;
	static std::vector<u8> copy_buffer(BUFFER_SIZE);
	verify(fseek(src, offset, SEEK_SET) == 0, "Failed to seek while extracting '%s'.", dest_path.string().c_str());
	for(s64 i = 0; i < size / BUFFER_SIZE; i++) {
		verify(fread(copy_buffer.data(), BUFFER_SIZE, 1, src) == 1,
			"Failed to read source file while extracting '%s'.", dest_path.string().c_str());
		verify(fwrite(copy_buffer.data(), BUFFER_SIZE, 1, dest) == 1,
			"Failed to write to file '%s'.", dest_path.string().c_str());

	}
	if(size % BUFFER_SIZE != 0) {
		verify(fread(copy_buffer.data(), size % BUFFER_SIZE, 1, src) == 1,
			"Failed to read source file while extracting '%s'.", dest_path.string().c_str());
		verify(fwrite(copy_buffer.data(), size % BUFFER_SIZE, 1, dest) == 1,
			"Failed to write to file '%s'.", dest_path.string().c_str());
	}
}
