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

#ifndef WAD_PRIMARY_H
#define WAD_PRIMARY_H

#include "../level.h"

struct PrimaryHeader {
	ByteRange code;
	ByteRange asset_header;
	ByteRange small_textures;
	ByteRange hud_header;
	ByteRange hud_banks[5];
	ByteRange assets;
	Opt<ByteRange> moby8355_pvars;
	Opt<ByteRange> art_instances;
	Opt<ByteRange> gameplay_core;
	Opt<ByteRange> global_nav_data;
};

void read_primary(LevelWad& wad, Buffer src);
SectorRange write_primary(OutBuffer& dest, const LevelWad& wad);
void swap_primary_header(PrimaryHeader& l, std::vector<u8>& r, Game game);

#endif
