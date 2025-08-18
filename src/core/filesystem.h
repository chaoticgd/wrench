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

#ifndef CORE_FILESYSTEM_H
#define CORE_FILESYSTEM_H

#include <filesystem>
namespace fs = std::filesystem;

#include "buffer.h"
#include <platform/fileio.h>

// These functions all throw on error.
std::vector<u8> read_file(WrenchFileHandle* file, s64 offset, s64 size);
std::vector<u8> read_file(fs::path path, bool text_mode = false);
void write_file(const fs::path& path, Buffer buffer, bool text_mode = false);

void strip_carriage_returns(std::vector<u8>& file);
void strip_carriage_returns_from_string(std::string& str);

#endif
