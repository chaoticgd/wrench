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

std::vector<u8> read_file(WrenchFileHandle* file, s64 offset, s64 size)
{
	s64 size_of_file = file_size(file);
	verify(file_seek(file, offset, WRENCH_FILE_ORIGIN_START) == 0, "Failed to seek.");
	if (offset + size > size_of_file && offset + size < size_of_file + SECTOR_SIZE) {
		// This happens if the last block in a file isn't padded to the sector size.
		size = size_of_file - offset;
	}
	std::vector<u8> buffer(size);
	if (buffer.size() > 0) {
		verify(file_read(buffer.data(), buffer.size(), file) == buffer.size(), "Failed to read file.");
	}
	return buffer;
}

std::vector<u8> read_file(fs::path path, bool text_mode)
{
	verify(!fs::is_directory(path), "Tried to open directory '%s' as regular file.", path.string().c_str());
	WrenchFileHandle* file = file_open(path.string().c_str(), WRENCH_FILE_MODE_READ);
	verify(file, "Failed to open file '%s' for reading (%s).", path.string().c_str(), FILEIO_ERROR_CONTEXT_STRING);
	defer([&]() { file_close(file); });
	std::vector<u8> buffer;
	if (text_mode) {
		buffer = std::vector<u8>(file_size(file) + 1);
		if (buffer.size() > 1) {
			size_t str_len = file_read_string((char*)buffer.data(), buffer.size(), file);
			verify(str_len > 0, "Failed to read file '%s' (%s).", path.string().c_str(), FILEIO_ERROR_CONTEXT_STRING);
			buffer.resize(str_len + 1);
		}
	} else {
		buffer = std::vector<u8>(file_size(file));
		if (buffer.size() > 0) {
			verify(file_read(buffer.data(), buffer.size(), file) == buffer.size(),
				"Failed to read file '%s' (%s).", path.string().c_str(), FILEIO_ERROR_CONTEXT_STRING);
		}
	}
	return buffer;
}

void write_file(const fs::path& path, Buffer buffer, bool text_mode)
{
	WrenchFileHandle* file = file_open(path.string().c_str(), WRENCH_FILE_MODE_WRITE);
	verify(file, "Failed to open file '%s' for writing (%s).", path.string().c_str(), FILEIO_ERROR_CONTEXT_STRING);
	defer([&]() { file_close(file); });
	if (buffer.size() > 0) {
		if (text_mode) {
			verify(file_write_string((char*)buffer.lo, file) > 0, "Failed to write file '%s' (%s).", path.string().c_str(), FILEIO_ERROR_CONTEXT_STRING);
		} else {
			verify(file_write(buffer.lo, buffer.size(), file) == buffer.size(), "Failed to write file '%s' (%s).", path.string().c_str(), FILEIO_ERROR_CONTEXT_STRING);
		}
	}
}

void strip_carriage_returns(std::vector<u8>& file)
{
	size_t new_size = 0;
	for (size_t i = 0; i < file.size(); i++) {
		if (file[i] != '\r') {
			file[new_size++] = file[i];
		}
	}
	file.resize(new_size);
}

void strip_carriage_returns_from_string(std::string& str)
{
	size_t new_size = 0;
	for (size_t i = 0; i < str.size(); i++) {
		if (str[i] != '\r') {
			str[new_size++] = str[i];
		}
	}
	str.resize(new_size);
}
