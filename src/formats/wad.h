/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019 chaoticgd

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

#include "../stream.h"

#include <cstring>

/*
	A container for data stored on disc using sliding window compression.
	Used to store textures, level data, etc. Not to be confused with the *.WAD
	files in the game's filesystem. Those contain multiple different segments,
	some of which may be WAD compressed.
*/
packed_struct(wad_header,
	char magic[3]; // "WAD"
	uint32_t total_size;
	uint8_t pad[9];
	uint8_t data[0 /* total_size - sizeof(wad_header) */];
)

// Check the magic bytes.
bool validate_wad(wad_header header);

// Throws stream_io_error, stream_format_error.
void decompress_wad(stream& dest, stream& src);
