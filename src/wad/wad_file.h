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

#ifndef WAD_WAD_FILE_H
#define WAD_WAD_FILE_H

#include <vector>

#include "util.h"
#include "level.h"
#include "buffer.h"
#include "gameplay.h"
#include "../lz/compression.h"

struct WadLumpDescription;

using ReadLumpFunc = std::function<void(WadLumpDescription desc, Wad& dest, std::vector<u8>& src)>;
using WriteLumpFunc = std::function<void(WadLumpDescription desc, s32 index, std::vector<u8>& dest, const Wad& src)>;

struct LumpFuncs {
	ReadLumpFunc read;
	WriteLumpFunc write;
};

struct WadLumpDescription {
	s32 offset;
	s32 count;
	LumpFuncs funcs;
	const char* name;
};

struct WadFileDescription {
	const char* name;
	u32 header_size;
	std::unique_ptr<Wad> (*create)();
	std::vector<WadLumpDescription> fields;
};

extern const std::vector<WadFileDescription> wad_files;

std::vector<u8> read_header(FILE* file);
WadFileDescription match_wad(FILE* file, const std::vector<u8>& header);
std::vector<u8> read_lump(FILE* file, Sector32 offset, Sector32 size);
void write_file(const char* path, Buffer buffer);

#endif
