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

#include "assets.h"

#include "moby.h"
#include "texture.h"
#include "collision.h"
#include "../core/timer.h"
#include "../lz/compression.h"

static void print_asset_header(const AssetHeader& header);

void read_assets(LevelWad& wad, Buffer asset_header, Buffer assets, Buffer gs_ram) {
	auto header = asset_header.read<AssetHeader>(0, "asset header");
	std::vector<s64> block_bounds = enumerate_asset_block_boundaries(asset_header, header, wad.game);
	
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
	wad.collision = read_collision(assets.read_bytes(header.collision, collision_size, "collision"));
	
	verify(header.moby_classes.count >= 1, "Level has no moby classes.");
	verify(header.tie_classes.count >= 1, "Level has no tie classes.");
	verify(header.shrub_classes.count >= 1, "Level has no shrub classes.");
	
	auto gs_ram_table = asset_header.read_multiple<GsRamEntry>(header.gs_ram, "gs ram table");
	auto tfrag_textures = asset_header.read_multiple<TextureEntry>(header.tfrag_textures, "tfrag texture table");
	auto moby_textures = asset_header.read_multiple<TextureEntry>(header.moby_textures, "moby texture table");
	auto tie_textures = asset_header.read_multiple<TextureEntry>(header.tie_textures, "tie texture table");
	auto shrub_textures = asset_header.read_multiple<TextureEntry>(header.shrub_textures, "shrub texture table");
	auto particle_textures = asset_header.read_multiple<ParticleTextureEntry>(header.part_textures, "particle texture table");
	auto fx_textures = asset_header.read_multiple<FXTextureEntry>(header.fx_textures, "fx texture table");
	
	Buffer texture_data = assets.subbuf(header.textures_base_offset);
	
	for(const TextureEntry& tfrag_texture : tfrag_textures) {
		wad.tfrag_textures.emplace_back(read_shared_texture(texture_data, gs_ram, tfrag_texture));
	}
	
	if(wad.game != Game::DL) {
		wad.unknown_a0 = assets.read_bytes(header.unknown_a0, 0x40, "unknown a0");
	}
	
	auto moby_classes = asset_header.read_multiple<MobyClassEntry>(header.moby_classes, "moby class table");
	for(const MobyClassEntry& entry : moby_classes) {
		ERROR_CONTEXT("moby %d", entry.o_class);
		
		MobyClass* moby = nullptr;
		for(MobyClass& cur : wad.moby_classes) {
			if(cur.o_class == entry.o_class) {
				moby = &cur;
			}
		}
		if(moby == nullptr) {
			MobyClass new_class;
			new_class.o_class = entry.o_class;
			wad.moby_classes.emplace_back(std::move(new_class));
			moby = &wad.moby_classes.back();
		}
		for(s32 i = 0; i < 16; i++) {
			if(entry.textures[i] != 0xff) {
				const TextureEntry& texture = moby_textures[entry.textures[i]];
				moby->textures.emplace_back(read_shared_texture(texture_data, gs_ram, texture));
			} else {
				break;
			}
		}
		if(entry.offset_in_asset_wad != 0) {
			s64 model_size = next_asset_block_size(entry.offset_in_asset_wad, block_bounds);
			moby->model = assets.read_bytes(entry.offset_in_asset_wad, model_size, "moby model");
			if(entry.o_class >= 10) {
				moby->high_model = recover_moby_class(read_moby_class(*moby->model, wad.game), entry.o_class, moby->textures.size());
			}
		}
		moby->has_asset_table_entry = true;
	}
	
	auto tie_classes = asset_header.read_multiple<TieClassEntry>(header.tie_classes, "tie class table");
	for(const TieClassEntry& entry : tie_classes) {
		ERROR_CONTEXT("tie %d", entry.o_class);
		verify(entry.offset_in_asset_wad != 0, "Pointer to header is null.");
		TieClass tie;
		tie.o_class = entry.o_class;
		s64 model_size = next_asset_block_size(entry.offset_in_asset_wad, block_bounds);
		tie.model = assets.read_bytes(entry.offset_in_asset_wad, model_size, "tie model");
		for(s32 i = 0; i < 16; i++) {
			if(entry.textures[i] != 0xff) {
				const TextureEntry& texture = tie_textures[entry.textures[i]];
				tie.textures.emplace_back(read_shared_texture(texture_data, gs_ram, texture));
			} else {
				break;
			}
		}
		wad.tie_classes.emplace_back(std::move(tie));
	}
	
	auto shrub_classes = asset_header.read_multiple<ShrubClassEntry>(header.shrub_classes, "shrub class table");
	for(const ShrubClassEntry& entry : shrub_classes) {
		ERROR_CONTEXT("shrub %d", entry.o_class);
		verify(entry.offset_in_asset_wad != 0, "Pointer to header is null.");
		ShrubClass shrub;
		shrub.o_class = entry.o_class;
		s64 model_size = next_asset_block_size(entry.offset_in_asset_wad, block_bounds);
		shrub.model = assets.read_bytes(entry.offset_in_asset_wad, model_size, "shrub model");
		for(s32 i = 0; i < 16; i++) {
			if(entry.textures[i] != 0xff) {
				const TextureEntry& texture = shrub_textures[entry.textures[i]];
				shrub.textures.emplace_back(read_shared_texture(texture_data, gs_ram, texture));
			} else {
				break;
			}
		}
		wad.shrub_classes.emplace_back(std::move(shrub));
	}
	
	if(wad.game != Game::DL) {
		auto ratchet_seqs = asset_header.read_multiple<s32>(header.ratchet_seqs_rac123, 256, "ratchet seqs");
		wad.ratchet_seqs.clear();
		for(s32 ratchet_seq_ofs : ratchet_seqs) {
			if(ratchet_seq_ofs != 0) {
				s64 seq_size = next_asset_block_size(ratchet_seq_ofs, block_bounds);
				wad.ratchet_seqs.emplace_back(assets.read_bytes(ratchet_seq_ofs, seq_size, "ratchet seq"));
			} else {
				wad.ratchet_seqs.push_back({});
			}
		}
	}
	
	Buffer particle_data = assets.subbuf(header.part_bank_offset);
	wad.particle_textures = read_particle_textures(particle_textures, particle_data);
	
	Buffer fx_data = assets.subbuf(header.fx_bank_offset);
	wad.fx_textures = read_fx_textures(fx_textures, fx_data);
	
	wad.particle_defs = asset_header.read_bytes(header.part_defs_offset, header.sound_remap_offset - header.part_defs_offset, "particle defs");
	if(wad.game != Game::RAC1) {
		wad.sound_remap = asset_header.read_bytes(header.sound_remap_offset, header.moby_gs_stash_list - header.sound_remap_offset, "sound remap");
	}
	
	print_asset_header(header);
}

void write_assets(OutBuffer header_dest, OutBuffer data_dest, OutBuffer gs_ram, const LevelWad& wad) {
	AssetHeader header = {0};
	header_dest.alloc<AssetHeader>();
	
	data_dest.pad(0x40);
	header.tfrags = data_dest.write_multiple(wad.tfrags);
	data_dest.pad(0x40);
	header.occlusion = data_dest.write_multiple(wad.occlusion);
	data_dest.pad(0x40);
	header.sky = data_dest.write_multiple(wad.sky);
	data_dest.pad(0x40);
	header.collision = data_dest.tell();
	write_collision(data_dest, wad.collision);
	
	// Allocate class tables.
	header_dest.pad(0x40);
	header.moby_classes.count = 0;
	for(const MobyClass& moby : wad.moby_classes) {
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
	
	TextureDedupeInput dedupe_input {
		wad.tfrag_textures,
		wad.moby_classes,
		wad.tie_classes,
		wad.shrub_classes
	};
	auto dedupe_output = prepare_texture_dedupe_records(dedupe_input);
	deduplicate_textures(dedupe_output.records);
	deduplicate_palettes(dedupe_output.records);
	std::vector<GsRamEntry> gs_ram_table;
	header.textures_base_offset = write_shared_texture_data(data_dest, gs_ram, gs_ram_table, dedupe_output.records);
	
	// Write texture tables.
	auto write_texture_table = [&](s32 table, size_t begin, size_t count) {
		s32 table_offset = header_dest.tell();
		s32 table_count = 0;
		for(size_t i = 0; i < count; i++) {
			TextureDedupeRecord* record = &dedupe_output.records.at(begin + i);
			if(record->texture_out_edge > -1) {
				record = &dedupe_output.records[record->texture_out_edge];
			}
			if(record->texture != nullptr && !record->indices[table].has_value()) {
				const Texture& texture = *record->texture;
				assert(record->texture_offset != -1);
				TextureEntry entry;
				entry.data_offset = record->texture_offset - header.textures_base_offset;
				entry.width = texture.width;
				entry.height = texture.height;
				entry.unknown_8 = 3;
				TextureDedupeRecord* palette_record = record;
				if(palette_record->palette_out_edge > -1) {
					palette_record = &dedupe_output.records[palette_record->palette_out_edge];
				}
				assert(palette_record->palette_offset != -1);
				entry.palette = palette_record->palette_offset / 0x100;
				entry.mipmap = record->mipmap_offset / 0x100;
				record->indices[table] = table_count;
				header_dest.write(entry);
				table_count++;
			}
		}
		return ArrayRange {table_count, table_offset};
	};
	header.tfrag_textures = write_texture_table(TFRAG_TEXTURE_INDEX, dedupe_output.tfrags_begin, wad.tfrag_textures.size());
	header.moby_textures = write_texture_table(MOBY_TEXTURE_INDEX, dedupe_output.mobies_begin, wad.moby_classes.size() * 16);
	header.tie_textures = write_texture_table(TIE_TEXTURE_INDEX, dedupe_output.ties_begin, wad.tie_classes.size() * 16);
	header.shrub_textures = write_texture_table(SHRUB_TEXTURE_INDEX, dedupe_output.shrubs_begin, wad.shrub_classes.size() * 16);
	
	data_dest.pad(0x100, 0);
	header.part_bank_offset = data_dest.tell();
	header.part_textures = write_particle_textures(header_dest, data_dest, wad.particle_textures);
	data_dest.pad(0x100, 0);
	header.fx_bank_offset = data_dest.tell();
	header.fx_textures = write_fx_textures(header_dest, data_dest, wad.fx_textures);
	printf("Shared texture memory: 0x%x bytes\n", header.part_bank_offset - header.textures_base_offset);
	
	header.gs_ram.count = gs_ram_table.size();
	header.gs_ram.offset = header_dest.tell();
	header_dest.write_multiple(gs_ram_table);
	
	header_dest.pad(0x10, 0);
	header.unknown_a0 = data_dest.tell();
	data_dest.write(wad.unknown_a0);
	
	// Write classes.
	auto write_texture_list = [&](u8 dest[16], s32 begin, s32 table) {
		for(s32 i = 0; i < 16; i++) {
			TextureDedupeRecord* record = &dedupe_output.records.at(begin + i);
			if(record->texture != nullptr) {
				if(record->texture_out_edge > -1) {
					record = &dedupe_output.records[record->texture_out_edge];
				}
				assert(record->indices[table].has_value());
				verify(*record->indices[table] < 0xff, "Too many textures.\n");
				dest[i] = *record->indices[table];
			} else {
				dest[i] = 0xff;
			}
		}
	};
	
	size_t mi = 0;
	for(size_t i = 0; i < wad.moby_classes.size(); i++) {
		const MobyClass& cls = wad.moby_classes[i];
		ERROR_CONTEXT("moby %d", cls.o_class);
		
		if(!cls.has_asset_table_entry) {
			continue;
		}
		
		MobyClassEntry entry = {0};
		entry.o_class = cls.o_class;
		if(cls.model.has_value()) {
			data_dest.pad(0x40);
			entry.offset_in_asset_wad = data_dest.tell();
			data_dest.write_multiple(*cls.model);
		}
		write_texture_list(entry.textures, dedupe_output.mobies_begin + i * 16, MOBY_TEXTURE_INDEX);
		header_dest.write(header.moby_classes.offset + (mi++) * sizeof(MobyClassEntry), entry);
	}
	
	for(size_t i = 0; i < wad.tie_classes.size(); i++) {
		const TieClass& cls = wad.tie_classes[i];
		ERROR_CONTEXT("tie %d", cls.o_class);
		
		TieClassEntry entry = {0};
		entry.o_class = cls.o_class;
		data_dest.pad(0x40);
		entry.offset_in_asset_wad = data_dest.tell();
		write_texture_list(entry.textures, dedupe_output.ties_begin + i * 16, TIE_TEXTURE_INDEX);
		header_dest.write(header.tie_classes.offset + i * sizeof(TieClassEntry), entry);
		data_dest.write_multiple(cls.model);
	}
	
	for(size_t i = 0; i < wad.shrub_classes.size(); i++) {
		const ShrubClass& cls = wad.shrub_classes[i];
		ERROR_CONTEXT("shrub %d", cls.o_class);
		
		ShrubClassEntry entry = {0};
		entry.o_class = cls.o_class;
		data_dest.pad(0x40);
		entry.offset_in_asset_wad = data_dest.tell();
		write_texture_list(entry.textures, dedupe_output.shrubs_begin + i * 16, SHRUB_TEXTURE_INDEX);
		header_dest.write(header.shrub_classes.offset + i * sizeof(ShrubClassEntry), entry);
		data_dest.write_multiple(cls.model);
	}
	
	data_dest.pad(0x10);
	header.scene_view_size = data_dest.tell();
	
	if(wad.game != Game::DL) {
		assert(wad.ratchet_seqs.size() == 256);
		std::vector<s32> ratchet_seq_offsets;
		for(const Opt<std::vector<u8>>& ratchet_seq : wad.ratchet_seqs) {
			if(ratchet_seq.has_value()) {
				data_dest.pad(0x10);
				ratchet_seq_offsets.push_back(data_dest.write_multiple(*ratchet_seq));
			} else {
				ratchet_seq_offsets.push_back(0);
			}
		}
		header_dest.pad(0x10);
		header.ratchet_seqs_rac123 = header_dest.write_multiple(ratchet_seq_offsets);
	}
	
	header_dest.pad(0x10);
	header.part_defs_offset = header_dest.tell();
	header_dest.write_multiple(wad.particle_defs);
	
	header_dest.pad(0x10);
	header.sound_remap_offset = header_dest.tell();
	header_dest.write_multiple(wad.sound_remap);
	
	header_dest.pad(0x10);
	header.moby_gs_stash_list = header_dest.tell();
	header_dest.write<s16>(-1);
	
	header.moby_gs_stash_count = 1;
	
	header.glass_map_texture = 0x4000;
	header.glass_map_palette = 0x400;
	
	header.assets_decompressed_size = data_dest.tell();
	
	print_asset_header(header);
	
	header_dest.write(0, header);
	write_file("/tmp", "newheader.bin", Buffer(header_dest.vec));
	write_file("/tmp", "gsram.bin", Buffer(gs_ram.vec));
	write_file("/tmp", "assets.bin", Buffer(data_dest.vec));
}

std::vector<s64> enumerate_asset_block_boundaries(Buffer src, const AssetHeader& header, Game game) {
	std::vector<s64> blocks {
		header.tfrags,
		header.occlusion,
		header.sky,
		header.collision,
		header.textures_base_offset,
		header.assets_decompressed_size
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
	if(game != Game::DL) {
		auto ratchet_seqs = src.read_multiple<s32>(header.ratchet_seqs_rac123, 256, "ratchet sequence offsets");
		for(s32 ratchet_seq_ofs : ratchet_seqs) {
			if(ratchet_seq_ofs != 0) {
				blocks.push_back(ratchet_seq_ofs);
			}
		}
	}
	return blocks;
}

s64 next_asset_block_size(s32 ofs, const std::vector<s64>& block_bounds) {
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
	printf("%32s %8x", "unknown_74", header.unknown_74);
	printf("%32s %8x", "ratchet_seqs_rac123", header.ratchet_seqs_rac123);
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
