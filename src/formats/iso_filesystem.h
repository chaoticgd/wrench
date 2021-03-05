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

#ifndef FORMATS_ISOFS_H
#define FORMATS_ISOFS_H

#include <map>

#include "../stream.h"

// Read an ISO filesystem and output a map (dest) of the files in the root
// directory. Return true on success, false on failure.
bool read_iso_filesystem(std::map<std::string, std::pair<size_t, size_t>>& dest, stream& iso);

// Given a list of files of a specific size, write and ISO filesystem (not the
// files themselves) and return a map specifying where each file should go.
std::map<std::string, size_t> write_iso_filesystem(stream& dest, const std::map<std::string, size_t>& file_sizes);

#endif
