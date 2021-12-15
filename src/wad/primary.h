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

#include "../core/level.h"

struct PrimaryHeader {
	ByteRange code;
	ByteRange asset_header;
	ByteRange gs_ram;
	ByteRange hud_header;
	ByteRange hud_banks[5];
	ByteRange assets;
	Opt<ByteRange> core_bank;
	Opt<ByteRange> transition_textures;
	Opt<ByteRange> moby8355_pvars;
	Opt<ByteRange> art_instances;
	Opt<ByteRange> gameplay_core;
	Opt<ByteRange> global_nav_data;
};

packed_struct(Rac1PrimaryHeader,
	/* 0x00 */ ByteRange code;
	/* 0x08 */ ByteRange core_bank;
	/* 0x10 */ ByteRange asset_header;
	/* 0x18 */ ByteRange gs_ram;
	/* 0x20 */ ByteRange hud_header;
	/* 0x28 */ ByteRange hud_banks[5];
	/* 0x50 */ ByteRange assets;
)

packed_struct(Rac23PrimaryHeader,
	/* 0x00 */ ByteRange code;
	/* 0x08 */ ByteRange asset_header;
	/* 0x10 */ ByteRange gs_ram;
	/* 0x18 */ ByteRange hud_header;
	/* 0x20 */ ByteRange hud_banks[5];
	/* 0x48 */ ByteRange assets;
	/* 0x50 */ ByteRange transition_textures;
)

packed_struct(DeadlockedPrimaryHeader,
	/* 0x00 */ ByteRange moby8355_pvars;
	/* 0x08 */ ByteRange code;
	/* 0x10 */ ByteRange asset_header;
	/* 0x18 */ ByteRange gs_ram;
	/* 0x20 */ ByteRange hud_header;
	/* 0x28 */ ByteRange hud_banks[5];
	/* 0x50 */ ByteRange assets;
	/* 0x58 */ ByteRange art_instances;
	/* 0x60 */ ByteRange gameplay_core;
	/* 0x68 */ ByteRange global_nav_data;
)

void read_primary(LevelWad& wad, Buffer src);
SectorRange write_primary(OutBuffer dest, LevelWad& wad);
PrimaryHeader read_primary_header(Buffer src, Game game);
void write_primary_header(OutBuffer dest, s64 header_ofs, const PrimaryHeader& src, Game game);

#endif
