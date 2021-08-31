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
#include "texture.h"

static void read_assets(LevelWad& wad, Buffer asset_header, Buffer assets, Buffer gs_ram);
static void write_assets(OutBuffer header_dest, std::vector<u8>& compressed_data_dest, OutBuffer gs_ram, const LevelWad& wad);
static std::vector<s64> enumerate_asset_block_boundaries(Buffer src, const AssetHeader& header); // Used to determining the size of certain blocks.
static s64 next_asset_block_size(s32 ofs, const std::vector<s64>& block_bounds);
static void print_asset_header(const AssetHeader& header);

static WadBuffer wad_buffer(Buffer buf) {
	return {buf.lo, buf.hi};
}

void read_primary(LevelWad& wad, Buffer src) {
	std::vector<u8> header_bytes = src.read_bytes(0, sizeof(DeadlockedPrimaryHeader), "primary header");
	PrimaryHeader header = {0};
	swap_primary_header(header, header_bytes, wad.game);
	
	wad.code = src.read_bytes(header.code.offset, header.code.size, "code");
	wad.asset_header = src.read_bytes(header.asset_header.offset, header.asset_header.size, "asset header");
	wad.hud_header = src.read_bytes(header.hud_header.offset, header.hud_header.size, "hud header");
	for(s32 i = 0; i < 5; i++) {
		if(header.hud_banks[i].offset > 0) {
			wad.hud_banks[i] = src.read_bytes(header.hud_banks[i].offset, header.hud_banks[i].size, "hud bank");
		}
	}
	std::vector<u8> assets_vec;
	verify(decompress_wad(assets_vec, wad_buffer(src.subbuf(header.assets.offset))), "Failed to decompress assets.");
	Buffer assets(assets_vec);
	read_assets(wad, wad.asset_header, assets, src.subbuf(header.gs_ram.offset));
	if(header.transition_textures.has_value()) {
		wad.transition_textures = src.read_bytes(header.transition_textures->offset, header.transition_textures->size, "transition textures");
	}
	if(header.moby8355_pvars.has_value()) {
		wad.moby8355_pvars = src.read_bytes(header.moby8355_pvars->offset, header.moby8355_pvars->size, "moby 8355 pvars");
	}
	if(header.global_nav_data.has_value()) {
		wad.global_nav_data = src.read_bytes(header.global_nav_data->offset, header.global_nav_data->size, "global nav data");
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
	std::vector<u8> gs_ram;
	write_assets(OutBuffer(asset_header), asset_data, gs_ram, wad);
	header.asset_header = write_primary_block(dest, asset_header, header_ofs);
	header.gs_ram = write_primary_block(dest, gs_ram, header_ofs);
	header.hud_header = write_primary_block(dest, wad.hud_header, header_ofs);
	for(s32 i = 0; i < 5; i++) {
		if(wad.hud_banks[i].size() > 0) {
			header.hud_banks[i] = write_primary_block(dest, wad.hud_banks[i], header_ofs);
		}
	}
	
	header.assets = write_primary_block(dest, asset_data, header_ofs);
	if(wad.transition_textures.has_value()) {
		header.transition_textures = write_primary_block(dest, *wad.transition_textures, header_ofs);
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
				l.transition_textures = ByteRange {0, 0};
			}
			l.moby8355_pvars = {};
			SWAP_PACKED(l.code, packed_header.code);
			SWAP_PACKED(l.asset_header, packed_header.asset_header);
			SWAP_PACKED(l.gs_ram, packed_header.gs_ram);
			SWAP_PACKED(l.hud_header, packed_header.hud_header);
			for(s32 i = 0; i < 5; i++) {
				SWAP_PACKED(l.hud_banks[i], packed_header.hud_banks[i]);
			}
			SWAP_PACKED(l.assets, packed_header.assets);
			SWAP_PACKED(*l.transition_textures, packed_header.transition_textures);
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
			SWAP_PACKED(l.gs_ram, packed_header.gs_ram);
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
	s32 offset_in_asset_wad;
	s32 o_class;
	s32 unknown_8;
	s32 unknown_c;
	u8 textures[16];
	u8 unknown_20[16];
)

static void read_assets(LevelWad& wad, Buffer asset_header, Buffer assets, Buffer gs_ram) {
	auto header = asset_header.read<AssetHeader>(0, "asset header");
	std::vector<s64> block_bounds = enumerate_asset_block_boundaries(asset_header, header);
	
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
	s32 occlusion_size = next_asset_block_size(header.occlusion, block_bounds);
	wad.occlusion = assets.read_bytes(header.occlusion, occlusion_size, "occlusion");
	s32 sky_size = next_asset_block_size(header.sky, block_bounds);
	wad.sky = assets.read_bytes(header.sky, sky_size, "sky");
	s32 collision_size = next_asset_block_size(header.collision, block_bounds);
	std::vector<u8> collision = assets.read_bytes(header.collision, collision_size, "collision");
	wad.collision_bin = collision;
	wad.collision = read_collision(Buffer(collision));
	
	verify(header.moby_classes.count >= 1, "Level has no moby classes.");
	verify(header.tie_classes.count >= 1, "Level has no tie classes.");
	verify(header.shrub_classes.count >= 1, "Level has no shrub classes.");
	
	auto moby_classes = asset_header.read_multiple<MobyClassEntry>(header.moby_classes.offset, header.moby_classes.count, "moby class table");
	auto tie_classes = asset_header.read_multiple<TieClassEntry>(header.tie_classes.offset, header.tie_classes.count, "tie class table");
	auto shrub_classes = asset_header.read_multiple<ShrubClassEntry>(header.shrub_classes.offset, header.shrub_classes.count, "shrub class table");
	
	auto gs_ram_table = asset_header.read_multiple<GsRamEntry>(header.gs_ram.offset, header.gs_ram.count, "gs ram table");
	auto tfrag_textures = asset_header.read_multiple<TextureEntry>(header.tfrag_textures.offset, header.tfrag_textures.count, "tfrag texture table");
	auto moby_textures = asset_header.read_multiple<TextureEntry>(header.moby_textures.offset, header.moby_textures.count, "moby texture table");
	auto tie_textures = asset_header.read_multiple<TextureEntry>(header.tie_textures.offset, header.tie_textures.count, "tie texture table");
	auto shrub_textures = asset_header.read_multiple<TextureEntry>(header.shrub_textures.offset, header.shrub_textures.count, "shrub texture table");
	auto part_textures = asset_header.read_multiple<TextureEntry>(header.part_textures.offset, header.part_textures.count, "part texture table");
	auto fx_textures = asset_header.read_multiple<TextureEntry>(header.fx_textures.offset, header.fx_textures.count, "fx texture table");
	
	Buffer texture_data = assets.subbuf(header.textures_base_offset);
	
	wad.tfrag_textures = read_tfrag_textures(tfrag_textures, texture_data, gs_ram);
	
	for(s64 i = 0; i < moby_classes.size(); i++) {
		const MobyClassEntry& entry = moby_classes[i];
		MobyClass& moby = wad.moby_classes[entry.o_class];
		if(entry.offset_in_asset_wad != 0) {
			s64 model_size = next_asset_block_size(entry.offset_in_asset_wad, block_bounds);
			moby.model = assets.read_bytes(entry.offset_in_asset_wad, model_size, "moby model");
		}
		moby.textures = read_instance_textures(moby_textures, entry.textures, texture_data, gs_ram);
		moby.has_asset_table_entry = true;
	}
	
	for(s64 i = 0; i < tie_classes.size(); i++) {
		verify(tie_classes[i].offset_in_asset_wad != 0, "Tie class %d has no model.", i);
		TieClass tie;
		s64 model_size = next_asset_block_size(tie_classes[i].offset_in_asset_wad, block_bounds);
		tie.model = assets.read_bytes(tie_classes[i].offset_in_asset_wad, model_size, "tie model");
		tie.textures = read_instance_textures(tie_textures, tie_classes[i].textures, texture_data, gs_ram);
		wad.tie_classes.emplace(tie_classes[i].o_class, tie);
	}
	
	for(s64 i = 0; i < shrub_classes.size(); i++) {
		verify(shrub_classes[i].offset_in_asset_wad != 0, "Shrub class %d has no model.", i);
		ShrubClass shrub;
		s64 model_size = next_asset_block_size(shrub_classes[i].offset_in_asset_wad, block_bounds);
		shrub.model = assets.read_bytes(shrub_classes[i].offset_in_asset_wad, model_size, "shrub model");
		shrub.textures = read_instance_textures(shrub_textures, shrub_classes[i].textures, texture_data, gs_ram);
		wad.shrub_classes.emplace(shrub_classes[i].o_class, shrub);
	}
	
	print_asset_header(header);
}

static void write_assets(OutBuffer header_dest, std::vector<u8>& compressed_data_dest, OutBuffer gs_ram, const LevelWad& wad) {
	std::vector<u8> data_vec;
	OutBuffer data_dest(data_vec);
	
	AssetHeader header = {0};
	header_dest.alloc<AssetHeader>();
	
	data_dest.pad(0x40);
	header.tfrags = data_dest.write_multiple(wad.tfrags);
	data_dest.pad(0x40);
	header.occlusion = data_dest.write_multiple(wad.occlusion);
	data_dest.pad(0x40);
	header.sky = data_dest.write_multiple(wad.sky);
	data_dest.pad(0x40);
	header.collision = data_dest.write_multiple(wad.collision_bin);//dest.write_multiple(write_dae(mesh_to_dae(wad.collision)));
	
	// Allocate class tables.
	header_dest.pad(0x40);
	header.moby_classes.count = 0;
	for(const auto& [number, moby] : wad.moby_classes) {
		if(!moby.has_asset_table_entry) {
			continue;
		}
		header.moby_classes.count++;
	}
	header.tie_classes.count = wad.tie_classes.size();
	header.shrub_classes.count = wad.shrub_classes.size();
	header.moby_classes.offset = header_dest.alloc_multiple<MobyClassEntry>(header.moby_classes.count);
	header.tie_classes.offset = header_dest.alloc_multiple<TieClassEntry>(header.tie_classes.count);
	header.shrub_classes.offset = header_dest.alloc_multiple<ShrubClassEntry>(header.shrub_classes.count);
	
	// Deduplicate textures.
	auto [texture_pointers, layout] = flatten_textures(wad);
	std::vector<PalettedTexture> paletted_textures;
	paletted_textures.reserve(texture_pointers.size());
	for(const Texture* texture : texture_pointers) {
		// TODO: Find optimal palette instead.
		paletted_textures.emplace_back(find_suboptimal_palette(*texture));
	}
	deduplicate_textures(paletted_textures);
	deduplicate_palettes(paletted_textures);
	
	for(PalettedTexture& texture : paletted_textures) {
		assert(texture.texture_out_edge == -1 || paletted_textures[texture.texture_out_edge].texture_out_edge == -1);
	}
	
	encode_palette_indices(paletted_textures);
	
	std::vector<GsRamEntry> gs_ram_table;
	
	data_dest.pad(0x40);
	header.textures_base_offset = data_dest.tell();
	for(PalettedTexture& texture : paletted_textures) {
		if(texture.texture_out_edge == -1) {
			texture.texture_offset = data_dest.write_multiple(texture.data);
			if(texture.palette_out_edge == -1) {
				gs_ram.pad(0x100, 0);
				texture.palette_offset = gs_ram.write_multiple(texture.palette.colours);
				gs_ram_table.push_back({0, 0, 0, texture.palette_offset / 0x100, texture.palette_offset / 0x100});
			}
		}
	}
	
	// Write texture tables.
	auto write_texture_table = [&](s32 table, s32 low, s32 high) {
		s32 table_offset = header_dest.tell();
		s32 table_count = 0;
		for(size_t i = low; i < high; i++) {
			PalettedTexture& texture = paletted_textures[i];
			if(texture.texture_out_edge > -1) {
				texture = paletted_textures[texture.texture_out_edge];
			}
			if(!texture.indices[table].has_value()) {
				assert(texture.is_first_occurence);
				assert(texture.texture_out_edge == -1);
				assert(texture.texture_offset != -1);
				TextureEntry entry;
				entry.data_offset = texture.texture_offset - header.textures_base_offset;
				entry.width = texture.width;
				entry.height = texture.height;
				entry.unknown_8 = 3;
				PalettedTexture& palette_texture = texture;
				while(palette_texture.palette_out_edge > -1) {
					palette_texture = paletted_textures[palette_texture.palette_out_edge];
				}
				entry.palette = palette_texture.palette_offset / 0x100;
				assert(entry.palette != -1);
				entry.mipmap = 0;
				header_dest.write(entry);
				texture.indices[table] = table_count;
				table_count++;
			}
		}
		return ArrayRange {table_count, table_offset};
	};
	header.tfrag_textures = write_texture_table(TFRAG_TEXTURE_INDEX, layout.tfrags_begin, layout.mobies_begin);
	header.moby_textures = write_texture_table(MOBY_TEXTURE_INDEX, layout.mobies_begin, layout.ties_begin);
	header.tie_textures = write_texture_table(TIE_TEXTURE_INDEX, layout.ties_begin, layout.shrubs_begin);
	header.shrub_textures = write_texture_table(SHRUB_TEXTURE_INDEX, layout.shrubs_begin, paletted_textures.size());
	
	header.gs_ram.count = gs_ram_table.size();
	header.gs_ram.offset = header_dest.tell();
	header_dest.write_multiple(gs_ram_table);
	
	// Write classes.
	size_t i = 0, class_index = 0;
	for(const auto& [number, moby] : wad.moby_classes) {
		if(!moby.has_asset_table_entry) {
			continue;
		}
		
		MobyClassEntry entry = {0};
		entry.o_class = number;
		if(moby.model.has_value()) {
			data_dest.pad(0x40);
			entry.offset_in_asset_wad = data_dest.tell();
			verify(moby.textures.size() < 16, "error: Moby %d has too many textures.\n", number);
			for(s32 j = 0; j < moby.textures.size(); j++) {
				PalettedTexture& texture = paletted_textures.at(layout.mobies_begin + class_index + j);
				if(texture.texture_out_edge > -1) {
					texture = paletted_textures[texture.texture_out_edge];
				}
				assert(texture.is_first_occurence);
				assert(texture.texture_out_edge == -1);
				assert(texture.indices[MOBY_TEXTURE_INDEX].has_value());
				//verify(*texture.indices[MOBY_TEXTURE_INDEX] < 0xff,
				//	"Too many moby textures (%d, should be at most 254).\n",
				//	texture.indices[MOBY_TEXTURE_INDEX]);
				if(texture.indices[MOBY_TEXTURE_INDEX]>=255)entry.textures[j]=0;else
				entry.textures[j] = *texture.indices[MOBY_TEXTURE_INDEX];
			}
			for(s32 j = moby.textures.size(); j < 16; j++) {
				entry.textures[j] = 0xff;
			}
			class_index += moby.textures.size();
			data_dest.write_multiple(*moby.model);
		} else {
			for(s32 j = 0; j < 16; j++) {
				entry.textures[j] = 0xff;
			}
		}
		header_dest.write(header.moby_classes.offset + (i++) * sizeof(MobyClassEntry), entry);
	}
	
	i = 0;
	for(const auto& [number, tie] : wad.tie_classes) {
		TieClassEntry entry = {0};
		entry.o_class = number;
		data_dest.pad(0x40);
		entry.offset_in_asset_wad = data_dest.tell();
		for(s32 j = 0; j < 16; j++) {
			entry.textures[j] = 0xff;
		}
		header_dest.write(header.tie_classes.offset + (i++) * sizeof(TieClassEntry), entry);
		data_dest.write_multiple(tie.model);
	}
	
	i = 0;
	for(const auto& [number, shrub] : wad.shrub_classes) {
		ShrubClassEntry entry = {0};
		entry.o_class = number;
		data_dest.pad(0x40);
		entry.offset_in_asset_wad = data_dest.tell();
		for(s32 j = 0; j < 16; j++) {
			entry.textures[j] = 0xff;
		}
		header_dest.write(header.shrub_classes.offset + (i++) * sizeof(ShrubClassEntry), entry);
		data_dest.write_multiple(shrub.model);
	}
	
	
	header.scene_view_size = 0x1321540;
	
	compress_wad(compressed_data_dest, data_vec, 8);
	header.assets_decompressed_size = data_vec.size();
	header.assets_compressed_size = compressed_data_dest.size();
	
	print_asset_header(header);
	
	header_dest.write(0, header);
	write_file("/tmp", "newheader.bin", Buffer(header_dest.vec));
	write_file("/tmp", "gsram.bin", Buffer(gs_ram.vec));
	write_file("/tmp", "assets.bin", Buffer(data_dest.vec));
}

static std::vector<s64> enumerate_asset_block_boundaries(Buffer src, const AssetHeader& header) {
	std::vector<s64> blocks {
		header.tfrags,
		header.occlusion,
		header.sky,
		header.collision,
		header.textures_base_offset
	};
	
	auto moby_classes = src.read_multiple<MobyClassEntry>(header.moby_classes.offset, header.moby_classes.count, "moby class table");
	for(const MobyClassEntry& entry : moby_classes) {
		blocks.push_back(entry.offset_in_asset_wad);
	}
	
	auto tie_classes = src.read_multiple<TieClassEntry>(header.tie_classes.offset, header.tie_classes.count, "tie class table");
	for(const TieClassEntry& entry : tie_classes) {
		blocks.push_back(entry.offset_in_asset_wad);
	}
	
	auto shrub_classes = src.read_multiple<ShrubClassEntry>(header.shrub_classes.offset, header.shrub_classes.count, "shrub class table");
	for(const ShrubClassEntry& entry : shrub_classes) {
		blocks.push_back(entry.offset_in_asset_wad);
	}
	
	blocks.push_back(header.assets_decompressed_size);
	return blocks;
}

static s64 next_asset_block_size(s32 ofs, const std::vector<s64>& block_bounds) {
	if(ofs == 0) {
		// e.g. if there is no sky.
		return 0;
	}
	s32 next_ofs = -1;
	for(s64 bound : block_bounds) {
		if(bound > ofs && (next_ofs == -1 || bound < next_ofs)) {
			next_ofs = bound;
		}
	}
	verify(next_ofs != -1, "Failed to determine size of asset block.");
	return next_ofs - ofs;
}

static void print_asset_header(const AssetHeader& header) {
	printf("%32s %8x", "gs_ram_count", header.gs_ram.count);
	printf("%32s %8x", "gs_ram_offset", header.gs_ram.offset);
	printf("%32s %8x", "tfrags", header.tfrags);
	printf("%32s %8x\n", "occlusion", header.occlusion);
	printf("%32s %8x", "sky", header.sky);
	printf("%32s %8x", "collision", header.collision);
	printf("%32s %8x", "moby_classes_count", header.moby_classes.count);
	printf("%32s %8x\n", "moby_classes_offset", header.moby_classes.offset);
	printf("%32s %8x", "tie_classes_count", header.tie_classes.count);
	printf("%32s %8x", "tie_classes_offset", header.tie_classes.offset);
	printf("%32s %8x", "shrub_classes_count", header.shrub_classes.count);
	printf("%32s %8x\n", "shrub_classes_offset", header.shrub_classes.offset);
	printf("%32s %8x", "tfrag_textures_count", header.tfrag_textures.count);
	printf("%32s %8x", "tfrag_textures_offset", header.tfrag_textures.offset);
	printf("%32s %8x", "moby_textures_count", header.moby_textures.count);
	printf("%32s %8x\n", "moby_textures_offset", header.moby_textures.offset);
	printf("%32s %8x", "tie_textures_count", header.tie_textures.count);
	printf("%32s %8x", "tie_textures_offset", header.tie_textures.offset);
	printf("%32s %8x", "shrub_textures_count", header.shrub_textures.count);
	printf("%32s %8x\n", "shrub_textures_offset", header.shrub_textures.offset);
	printf("%32s %8x", "part_textures_count", header.part_textures.count);
	printf("%32s %8x", "part_textures_offset", header.part_textures.offset);
	printf("%32s %8x", "fx_textures_count", header.fx_textures.count);
	printf("%32s %8x\n", "fx_textures_offset", header.fx_textures.offset);
	printf("%32s %8x", "textures_base_offset", header.textures_base_offset);
	printf("%32s %8x", "part_bank_offset", header.part_bank_offset);
	printf("%32s %8x", "fx_bank_offset", header.fx_bank_offset);
	printf("%32s %8x\n", "part_defs_offset", header.part_defs_offset);
	printf("%32s %8x", "sound_remap_offset", header.sound_remap_offset);
	printf("%32s %8x", "assets_base_address", header.assets_base_address);
	printf("%32s %8x", "light_cuboids_offset", header.light_cuboids_offset);
	printf("%32s %8x\n", "scene_view_size", header.scene_view_size);
	printf("%32s %8x", "index_into_some1_texs", header.index_into_some1_texs);
	printf("%32s %8x", "moby_gs_stash_count", header.moby_gs_stash_count);
	printf("%32s %8x", "assets_compressed_size", header.assets_compressed_size);
	printf("%32s %8x\n", "assets_decompressed_size", header.assets_decompressed_size);
	printf("%32s %8x", "chrome_map_texture", header.chrome_map_texture);
	printf("%32s %8x", "chrome_map_palette", header.chrome_map_palette);
	printf("%32s %8x", "glass_map_texture", header.glass_map_texture);
	printf("%32s %8x\n", "glass_map_palette", header.glass_map_palette);
	printf("%32s %8x", "unknown_a0", header.unknown_a0);
	printf("%32s %8x", "heightmap_offset", header.heightmap_offset);
	printf("%32s %8x", "occlusion_oct_offset", header.occlusion_oct_offset);
	printf("%32s %8x\n", "moby_gs_stash_list", header.moby_gs_stash_list);
	printf("%32s %8x", "occlusion_rad_offset", header.occlusion_rad_offset);
	printf("%32s %8x", "moby_sound_remap_offset", header.moby_sound_remap_offset);
	printf("%32s %8x\n", "occlusion_rad2_offset", header.occlusion_rad2_offset);
}
