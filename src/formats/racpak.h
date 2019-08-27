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

#ifndef FORMATS_RACPAK_H
#define FORMATS_RACPAK_H

#include <vector>

#include "../stream.h"
#include "wad.h"

# /*
#	Tools to open and modify racpak (*.WAD) archive files.
# */

struct racpak_entry {
	uint32_t offset;
	uint32_t size;
};

class racpak {
public:
	racpak(stream* backing, uint32_t offset, uint32_t size);

	uint32_t num_entries();
	racpak_entry entry(uint32_t index);
	stream* open(racpak_entry file);
	
	bool is_compressed(racpak_entry entry);
	stream* open_decompressed(racpak_entry entry);
	
	// Compress WAD segments and write them to the iso_stream.
	void commit();
	
private:

	proxy_stream _backing;
	std::vector<std::unique_ptr<proxy_stream>> _open_segments;
	std::vector<std::unique_ptr<wad_stream>> _wad_segments;
};

#endif
