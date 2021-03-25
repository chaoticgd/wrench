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

#include "../stream.h"

struct iso_file_record {
	std::string name;
	sector32 lba;
	uint32_t size;
};

// Read an ISO filesystem and output a map (dest) of the files in the root
// directory. Return true on success, false on failure.
bool read_iso_filesystem(std::vector<iso_file_record>& dest, stream& iso);

// Given a list of files of a specific size, write and ISO filesystem (not the
// files themselves) and return a map specifying where each file should go.
std::map<std::string, size_t> write_iso_filesystem(stream& dest, const std::vector<iso_file_record>& files);

#endif
