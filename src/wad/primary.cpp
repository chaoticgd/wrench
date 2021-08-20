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

#include "primary.h"

#include "../lz/compression.h"
#include "collision.h"

packed_struct(Rac123PrimaryHeader,
	/* 0x00 */ ByteRange code;
	/* 0x08 */ ByteRange asset_header;
	/* 0x10 */ ByteRange small_textures;
	/* 0x18 */ ByteRange hud_header;
	/* 0x20 */ ByteRange hud_banks[5];
	/* 0x48 */ ByteRange assets;
	/* 0x50 */ ByteRange loading_screen_textures;
)

packed_struct(DeadlockedPrimaryHeader,
	/* 0x00 */ ByteRange moby8355_pvars;
	/* 0x08 */ ByteRange code;
	/* 0x10 */ ByteRange asset_header;
	/* 0x18 */ ByteRange small_textures;
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

packed_struct(DeadlockedAssetHeader,
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
static_assert(sizeof(DeadlockedAssetHeader) == 0xbc);

static void read_assets(LevelWad& wad, Buffer asset_header, Buffer assets);
static SectorRange write_assets(OutBuffer header_dest, OutBuffer data_dest, const LevelWad& wad);
static s32 next_asset_block(s32 ofs, const DeadlockedAssetHeader& header);

static WadBuffer wad_buffer(Buffer buf) {
	return {buf.lo, buf.hi};
}

void read_primary(LevelWad& wad, Buffer src) {
	std::vector<u8> header_bytes = src.read_bytes(0, sizeof(DeadlockedPrimaryHeader), "primary header");
	PrimaryHeader header = {0};
	swap_primary_header(header, header_bytes, wad.game);
	
	wad.code = src.read_bytes(header.code.offset, header.code.size, "code");
	wad.asset_header = src.read_bytes(header.asset_header.offset, header.asset_header.size, "asset_header");
	wad.small_textures = src.read_bytes(header.small_textures.offset, header.small_textures.size, "small_textures");
	wad.hud_header = src.read_bytes(header.hud_header.offset, header.hud_header.size, "hud_header");
	for(s32 i = 0; i < 5; i++) {
		if(header.hud_banks[i].offset > 0) {
			wad.hud_banks[i] = src.read_bytes(header.hud_banks[i].offset, header.hud_banks[i].size, "hud_banks");
		}
	}
	std::vector<u8> assets_vec;
	verify(decompress_wad(assets_vec, wad_buffer(src.subbuf(header.assets.offset))), "Failed to decompress assets.");
	Buffer assets(assets_vec);
	read_assets(wad, wad.asset_header, assets);
	if(header.moby8355_pvars.has_value()) {
		wad.moby8355_pvars = src.read_bytes(header.moby8355_pvars->offset, header.moby8355_pvars->size, "moby8355_pvars");
	}
	if(header.global_nav_data.has_value()) {
		wad.global_nav_data = src.read_bytes(header.global_nav_data->offset, header.global_nav_data->size, "global_nav_data");
	}
}

static ByteRange write_primary_block(OutBuffer& dest, const std::vector<u8>& bytes, s64 primary_ofs) {
	dest.pad(0x40);
	s64 block_ofs = dest.tell();
	dest.write_multiple(bytes);
	return {(s32) (block_ofs - primary_ofs), (s32) (dest.tell() - block_ofs)};
}

SectorRange write_primary(OutBuffer& dest, const LevelWad& wad) {
	dest.pad(SECTOR_SIZE, 0);
	
	PrimaryHeader header = {0};
	s64 header_ofs;
	if(wad.game == Game::DL) {
		header_ofs = dest.alloc<DeadlockedPrimaryHeader>();
	} else {
		header_ofs = dest.alloc<Rac123PrimaryHeader>();
	}
	
	if(wad.moby8355_pvars.has_value()) {
		header.moby8355_pvars = write_primary_block(dest, *wad.moby8355_pvars, header_ofs);
	}
	header.code = write_primary_block(dest, wad.code, header_ofs);
	std::vector<u8> asset_header;
	std::vector<u8> asset_data;
	write_assets(OutBuffer(asset_header), OutBuffer(asset_data), wad);
	header.asset_header = write_primary_block(dest, asset_header, header_ofs);
	std::vector<u8> compressed_asset_data;
	compress_wad(compressed_asset_data, asset_data, 8);
	header.asset_header = write_primary_block(dest, compressed_asset_data, header_ofs);
	
	header.small_textures = write_primary_block(dest, wad.small_textures, header_ofs);
	header.hud_header = write_primary_block(dest, wad.hud_header, header_ofs);
	for(s32 i = 0; i < 5; i++) {
		if(wad.hud_banks[i].size() > 0) {
			header.hud_banks[i] = write_primary_block(dest, wad.hud_banks[i], header_ofs);
		}
	}
	
	std::vector<u8> header_bytes;
	swap_primary_header(header, header_bytes, wad.game);
	dest.write_multiple(header_ofs, header_bytes);
	
	return {
		(s32) (header_ofs / SECTOR_SIZE),
		Sector32::size_from_bytes(dest.tell() - header_ofs)
	};
}

void swap_primary_header(PrimaryHeader& l, std::vector<u8>& r, Game game) {
	switch(game) {
		case Game::RAC1:
		case Game::RAC2:
		case Game::RAC3: {
			Rac123PrimaryHeader packed_header = {0};
			if(r.size() >= sizeof(Rac123PrimaryHeader)) {
				packed_header = Buffer(r).read<Rac123PrimaryHeader>(0, "primary header");
			}
			l.moby8355_pvars = {};
			SWAP_PACKED(l.code, packed_header.code);
			SWAP_PACKED(l.asset_header, packed_header.asset_header);
			SWAP_PACKED(l.small_textures, packed_header.small_textures);
			SWAP_PACKED(l.hud_header, packed_header.hud_header);
			for(s32 i = 0; i < 5; i++) {
				SWAP_PACKED(l.hud_banks[i], packed_header.hud_banks[i]);
			}
			SWAP_PACKED(l.assets, packed_header.assets);
			l.art_instances = {};
			l.gameplay_core = {};
			l.global_nav_data = {};
			if(r.size() == 0) {
				OutBuffer(r).write(packed_header);
			}
			break;
		}
		case Game::DL: {
			DeadlockedPrimaryHeader packed_header = {0};
			if(r.size() >= sizeof(DeadlockedPrimaryHeader)) {
				packed_header = Buffer(r).read<DeadlockedPrimaryHeader>(0, "primary header");
				l.moby8355_pvars = ByteRange {0, 0};
				l.art_instances = ByteRange {0, 0};
				l.gameplay_core = ByteRange {0, 0};
				l.global_nav_data = ByteRange {0, 0};
			}
			SWAP_PACKED(*l.moby8355_pvars, packed_header.moby8355_pvars);
			SWAP_PACKED(l.code, packed_header.code);
			SWAP_PACKED(l.asset_header, packed_header.asset_header);
			SWAP_PACKED(l.small_textures, packed_header.small_textures);
			SWAP_PACKED(l.hud_header, packed_header.hud_header);
			for(s32 i = 0; i < 5; i++) {
				SWAP_PACKED(l.hud_banks[i], packed_header.hud_banks[i]);
			}
			SWAP_PACKED(l.assets, packed_header.assets);
			SWAP_PACKED(*l.moby8355_pvars, packed_header.moby8355_pvars);
			SWAP_PACKED(*l.art_instances, packed_header.art_instances);
			SWAP_PACKED(*l.gameplay_core, packed_header.gameplay_core);
			SWAP_PACKED(*l.global_nav_data, packed_header.global_nav_data);
			if(r.size() == 0) {
				OutBuffer(r).write(packed_header);
			}
			break;
		}
	}
}

packed_struct(MobyClassEntry,
	s32 offset_in_asset_wad;
	s32 o_class;
	s32 unknown_8;
	s32 unknown_c;
	u8 textures[16];
)

packed_struct(TieClassEntry,
	s32 offset_in_asset_wad;
	s32 o_class;
	s32 unknown_8;
	s32 unknown_c;
	u8 textures[16];
)

packed_struct(ShrubClassEntry,
	uint32_t offset_in_asset_wad;
	uint32_t o_class;
	uint32_t unknown_8;
	uint32_t unknown_c;
	uint8_t textures[16];
	uint8_t unknown_20[16];
)

static void read_assets(LevelWad& wad, Buffer asset_header, Buffer assets) {
	auto header = asset_header.read<DeadlockedAssetHeader>(0, "asset header");
	
	s32 tfrags_size;
	if(header.occlusion != 0) {
		tfrags_size = header.occlusion;
	} else if(header.sky != 0) {
		tfrags_size = header.sky;
	} else if(header.collision != 0) {
		tfrags_size = header.collision;
	} else {
		verify_not_reached("Unable to determine size of tfrag block.");
	}
	wad.tfrags = assets.read_bytes(header.tfrags, tfrags_size, "tfrags");
	s32 occlusion_size = next_asset_block(header.occlusion, header) - header.occlusion;
	wad.occlusion = assets.read_bytes(header.occlusion, occlusion_size, "occlusion");
	s32 sky_size = next_asset_block(header.sky, header) - header.sky;
	wad.sky = assets.read_bytes(header.sky, sky_size, "sky");
	s32 collision_size = next_asset_block(header.collision, header) - header.collision;
	std::vector<u8> collision = assets.read_bytes(header.collision, collision_size, "collision");
	wad.collision_bin = collision;
	wad.collision = read_collision(Buffer(collision));
	
	verify(header.moby_classes.count >= 1, "Level has no moby classes.");
	verify(header.tie_classes.count >= 1, "Level has no tie classes.");
	verify(header.shrub_classes.count >= 1, "Level has no shrub classes.");
	
	auto moby_classes = asset_header.read_multiple<MobyClassEntry>(header.moby_classes.offset, header.moby_classes.count, "moby class table");
	auto tie_classes = asset_header.read_multiple<TieClassEntry>(header.tie_classes.offset, header.tie_classes.count, "tie class table");
	auto shrub_classes = asset_header.read_multiple<ShrubClassEntry>(header.shrub_classes.offset, header.shrub_classes.count, "shrub class table");
	
	s32 textures_size = moby_classes[0].offset_in_asset_wad - header.textures_base_offset;
	wad.textures = assets.read_bytes(header.textures_base_offset, textures_size, "textures");
	
	s32 mobies_size = tie_classes[0].offset_in_asset_wad - moby_classes[0].offset_in_asset_wad;
	wad.mobies = assets.read_bytes(moby_classes[0].offset_in_asset_wad, mobies_size, "moby classes");
	
	s32 ties_size = shrub_classes[0].offset_in_asset_wad - tie_classes[0].offset_in_asset_wad;
	wad.ties = assets.read_bytes(tie_classes[0].offset_in_asset_wad, ties_size, "tie classes");
	
	s32 shrubs_size = header.assets_decompressed_size - shrub_classes[0].offset_in_asset_wad;
	wad.shrubs = assets.read_bytes(shrub_classes[0].offset_in_asset_wad, shrubs_size, "shrub classes");
}

static SectorRange write_assets(OutBuffer header_dest, OutBuffer data_dest, const LevelWad& wad) {
	DeadlockedAssetHeader header;
	
	data_dest.pad(0x40);
	header.tfrags = data_dest.write_multiple(wad.tfrags);
	data_dest.pad(0x40);
	header.occlusion = data_dest.write_multiple(wad.occlusion);
	data_dest.pad(0x40);
	header.sky = data_dest.write_multiple(wad.sky);
	data_dest.pad(0x40);
	header.collision = data_dest.write_multiple(wad.collision_bin);//dest.write_multiple(write_dae(mesh_to_dae(wad.collision)));
	data_dest.pad(0x40);
	header.textures_base_offset = data_dest.write_multiple(wad.textures);
	data_dest.pad(0x40);
	data_dest.write_multiple(wad.mobies);
	data_dest.pad(0x40);
	data_dest.write_multiple(wad.ties);
	data_dest.pad(0x40);
	data_dest.write_multiple(wad.shrubs);
	
	return {0, 0};
}

static s32 next_asset_block(s32 ofs, const DeadlockedAssetHeader& header) {
	if(ofs == 0) {
		return 0;
	}
	
	s32 offsets[] = {
		header.tfrags,
		header.occlusion,
		header.sky,
		header.collision,
		header.textures_base_offset
	};
	s32 value = -1;
	for(s32 compare : offsets) {
		if((value == -1 || compare < value) && compare > ofs) {
			value = compare;
		}
	}
	return value;
}
