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

#ifndef ISO_FILESYSTEM_H
#define ISO_FILESYSTEM_H

#include <map>

#include "../core/buffer.h"
#include "../editor/fs_includes.h"
#include "legacy_stream.h"

struct iso_file_record {
	std::string name;
	Sector32 lba;
	uint32_t size;
};

struct iso_directory {
	std::string name;
	std::vector<iso_file_record> files;
	std::vector<iso_directory> subdirs;
	
	// Fields below used internally by write_iso_filesystem.
	iso_directory* parent = nullptr;
	size_t index = 0;
	size_t parent_index = 0;
	Sector32 lba = {0};
	uint32_t size = 0;
};

// Read an ISO filesystem and output a map (dest) of the files in the root
// directory. Return true on success, false on failure.
bool read_iso_filesystem(iso_directory& dest, std::string& volume_id, Buffer src);

// Given a list of files including their LBA and size, write out an ISO
// filesystem. This function is "dumb" in that it doesn't work out any positions
// by itself.
void write_iso_filesystem(stream& dest, iso_directory* root_dir);

void print_file_record(iso_file_record& record, const char* row_format);

#endif
