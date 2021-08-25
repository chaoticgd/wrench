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
	ByteRange gs_ram;
	ByteRange hud_header;
	ByteRange hud_banks[5];
	ByteRange assets;
	Opt<ByteRange> moby8355_pvars;
	Opt<ByteRange> art_instances;
	Opt<ByteRange> gameplay_core;
	Opt<ByteRange> global_nav_data;
};

packed_struct(Rac123PrimaryHeader,
	/* 0x00 */ ByteRange code;
	/* 0x08 */ ByteRange asset_header;
	/* 0x10 */ ByteRange gs_ram;
	/* 0x18 */ ByteRange hud_header;
	/* 0x20 */ ByteRange hud_banks[5];
	/* 0x48 */ ByteRange assets;
	/* 0x50 */ ByteRange loading_screen_textures;
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

packed_struct(ArrayRange,
	s32 count;
	s32 offset;
)

packed_struct(AssetHeader,
	/* 0x00 */ ArrayRange gs_ram;
	/* 0x08 */ s32 tfrags;
	/* 0x0c */ s32 occlusion;
	/* 0x10 */ s32 sky;
	/* 0x14 */ s32 collision;
	/* 0x18 */ ArrayRange moby_classes;
	/* 0x20 */ ArrayRange tie_classes;
	/* 0x28 */ ArrayRange shrub_classes;
	/* 0x30 */ ArrayRange tfrag_textures;
	/* 0x38 */ ArrayRange moby_textures;
	/* 0x40 */ ArrayRange tie_textures;
	/* 0x48 */ ArrayRange shrub_textures;
	/* 0x50 */ ArrayRange part_textures;
	/* 0x58 */ ArrayRange fx_textures;
	/* 0x60 */ s32 textures_base_offset;
	/* 0x64 */ s32 part_bank_offset;
	/* 0x68 */ s32 fx_bank_offset;
	/* 0x6c */ s32 part_defs_offset;
	/* 0x70 */ s32 sound_remap_offset;
	/* 0x74 */ s32 assets_base_address;
	/* 0x78 */ s32 light_cuboids_offset;
	/* 0x7c */ s32 scene_view_size;
	/* 0x80 */ s32 index_into_some1_texs;
	/* 0x84 */ s32 moby_gs_stash_count;
	/* 0x88 */ s32 assets_compressed_size;
	/* 0x8c */ s32 assets_decompressed_size;
	/* 0x90 */ s32 chrome_map_texture;
	/* 0x94 */ s32 chrome_map_palette;
	/* 0x98 */ s32 glass_map_texture;
	/* 0x9c */ s32 glass_map_palette;
	/* 0xa0 */ s32 unknown_a0;
	/* 0xa4 */ s32 heightmap_offset;
	/* 0xa8 */ s32 occlusion_oct_offset;
	/* 0xac */ s32 moby_gs_stash_list;
	/* 0xb0 */ s32 occlusion_rad_offset;
	/* 0xb4 */ s32 moby_sound_remap_offset;
	/* 0xb8 */ s32 occlusion_rad2_offset;
)
static_assert(sizeof(AssetHeader) == 0xbc);

void read_primary(LevelWad& wad, Buffer src);
SectorRange write_primary(OutBuffer& dest, const LevelWad& wad);
void swap_primary_header(PrimaryHeader& l, std::vector<u8>& r, Game game);

#endif
