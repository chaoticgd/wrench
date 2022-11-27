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

#include "filesystem.h"

static s64 file_size_in_bytes(FILE* file) {
	s64 last_ofs = ftell(file);
	fseek(file, 0, SEEK_END);
	s64 ofs = ftell(file);
	fseek(file, last_ofs, SEEK_SET);
	return ofs;
}

std::vector<u8> read_file(FILE* file, s64 offset, s64 size) {
	s64 file_size = file_size_in_bytes(file);
	verify(fseek(file, offset, SEEK_SET) == 0, "Failed to seek.");
	if(offset + size > file_size && offset + size < file_size + SECTOR_SIZE) {
		// This happens if the last block in a file isn't padded to the sector size.
		size = file_size - offset;
	}
	std::vector<u8> buffer(size);
	if(buffer.size() > 0) {
		verify(fread(buffer.data(), buffer.size(), 1, file) == 1, "Failed to read file.");
	}
	return buffer;
}

std::vector<u8> read_file(fs::path path, const char* open_mode) {
	verify(!fs::is_directory(path), "Tried to open directory '%s' as regular file.", path.string().c_str());
	FILE* file = fopen(path.string().c_str(), "rb");
	verify(file, "Failed to open file '%s' for reading.", path.string().c_str());
	std::vector<u8> buffer(fs::file_size(path));
	if(buffer.size() > 0) {
		verify(fread(buffer.data(), buffer.size(), 1, file) == 1, "Failed to read file '%s'.", path.string().c_str());
	}
	fclose(file);
	if(strcmp(open_mode, "r") == 0) {
		strip_carriage_returns(buffer);
	}
	return buffer;
}

void write_file(const fs::path& path, Buffer buffer, const char* open_mode) {
	FILE* file = fopen(path.string().c_str(), open_mode);
	verify(file, "Failed to open file '%s' for writing.", path.string().c_str());
	if(buffer.size() > 0) {
		verify(fwrite(buffer.lo, buffer.size(), 1, file) == 1, "Failed to write output file '%s'.", path.string().c_str());
	}
	fclose(file);
}

std::string write_file(fs::path dest_dir, fs::path rel_path, Buffer buffer, const char* open_mode) {
	fs::path dest_path = dest_dir/rel_path;
	FILE* file = fopen(dest_path.string().c_str(), open_mode);
	verify(file, "Failed to open file '%s' for writing.", dest_path.string().c_str());
	if(buffer.size() > 0) {
		verify(fwrite(buffer.lo, buffer.size(), 1, file) == 1, "Failed to write output file '%s'.", dest_path.string().c_str());
	}
	fclose(file);
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

void strip_carriage_returns(std::vector<u8>& file) {
	size_t new_size = 0;
	for(size_t i = 0; i < file.size(); i++) {
		if(file[i] != '\r') {
			file[new_size++] = file[i];
		}
	}
	file.resize(new_size);
}

void strip_carriage_returns_from_string(std::string& str) {
	size_t new_size = 0;
	for(size_t i = 0; i < str.size(); i++) {
		if(str[i] != '\r') {
			str[new_size++] = str[i];
		}
	}
	str.resize(new_size);
}
