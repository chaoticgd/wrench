/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2023 chaoticgd

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

#ifndef WRENCHBUILD_LEVEL_CORE_H
#define WRENCHBUILD_LEVEL_CORE_H

#include <instancemgr/gameplay.h>
#include <assetmgr/asset_types.h>
#include <wrenchbuild/level/level_chunks.h>
#include <wrenchbuild/level/level_textures.h>

packed_struct(LevelCoreHeader,
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
	/* 0x74 */ s32 unknown_74;
	packed_nested_anon_union(
		/* 0x78 */ s32 ratchet_seqs_rac123;
		/* 0x78 */ s32 light_cuboids_offset_dl;
	)
	/* 0x7c */ s32 scene_view_size;
	packed_nested_anon_union(
		/* 0x80 */ s32 gadget_count_rac1;
		/* 0x80 */ s32 index_into_some1_texs_rac2_maybe3;
	)
	packed_nested_anon_union(
		/* 0x84 */ s32 gadget_offset_rac1;
		/* 0x84 */ s32 moby_gs_stash_count_rac23dl;
	)
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
static_assert(sizeof(LevelCoreHeader) == 0xbc);

packed_struct(EndOfRacLevelCoreHeader,
	/* 0x00 */ u8 data[0x30];
)

packed_struct(MobyClassEntry,
	/* 0x00 */ s32 offset_in_asset_wad;
	/* 0x04 */ s32 o_class;
	/* 0x08 */ s32 unknown_8;
	/* 0x0c */ s32 unknown_c;
	/* 0x10 */ u8 textures[16];
)

packed_struct(TieClassEntry,
	/* 0x00 */ s32 offset_in_asset_wad;
	/* 0x04 */ s32 o_class;
	/* 0x08 */ s32 unknown_8;
	/* 0x0c */ s32 unknown_c;
	/* 0x10 */ u8 textures[16];
)

packed_struct(ShrubClassEntry,
	/* 0x00 */ s32 offset_in_asset_wad;
	/* 0x04 */ s32 o_class;
	/* 0x08 */ s32 pad_8;
	/* 0x0c */ s32 pad_c;
	/* 0x10 */ u8 textures[16];
	/* 0x20 */ ShrubBillboardInfo billboard;
)

packed_struct(RacGadgetHeader,
	/* 0x0 */ s32 offset_in_asset_wad;
	/* 0x4 */ s32 class_number;
	/* 0x8 */ s32 compressed_size;
	/* 0xc */ s32 pad;
)

packed_struct(SoundRemapHeader,
	/* 0x0 */ s16 second_part_ofs;
	/* 0x2 */ s16 second_part_size;
	/* 0x4 */ s16 third_part_ofs;
	/* 0x6 */ s16 third_part_count;
)

packed_struct(SoundRemapElement,
	/* 0x0 */ s16 offset;
	/* 0x2 */ s16 size;
)

packed_struct(MobySoundRemapHeader,
	/* 0x00 */ s32 size;
	/* 0x04 */ s32 second_part_ofs;
	/* 0x08 */ s32 third_part_ofs;
	/* 0x0c */ s32 fourth_part_ofs;
	/* 0x10 */ s32 unknown;
)

void unpack_level_core(
	LevelWadAsset& dest,
	InputStream& src,
	ByteRange index_range,
	ByteRange data_range,
	ByteRange gs_ram_range,
	BuildConfig config);
void pack_level_core(
	std::vector<u8>& index_dest,
	std::vector<u8>& data_dest,
	std::vector<u8>& gs_ram_dest,
	const std::vector<LevelChunk>& chunks,
	const LevelWadAsset& src,
	BuildConfig config);
BuildAsset& build_from_level_wad_asset(LevelWadAsset& core);
ByteRange level_core_block_range(s32 ofs, const std::vector<s64>& block_bounds);

#endif
