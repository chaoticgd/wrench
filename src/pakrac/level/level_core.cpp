/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2022 chaoticgd

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

#include "level_core.h"

#include <core/png.h>
#include <core/timer.h>
#include <engine/moby.h>
#include <engine/texture.h>
#include <engine/collision.h>
#include <engine/compression.h>
#include <pakrac/asset_unpacker.h>
#include <pakrac/asset_packer.h>

static void unpack_moby_classes(LevelCoreAsset& core, const LevelCoreHeader& header, InputStream& index, InputStream& data, InputStream& gs_ram, const std::vector<s64>& block_bounds, Game game);
static void unpack_tie_classes(LevelCoreAsset& core, const LevelCoreHeader& header, InputStream& index, InputStream& data, InputStream& gs_ram, const std::vector<s64>& block_bounds, Game game);
static void unpack_shrub_classes(LevelCoreAsset& core, const LevelCoreHeader& header, InputStream& index, InputStream& data, InputStream& gs_ram, const std::vector<s64>& block_bounds, Game game);
static void unpack_level_textures(CollectionAsset& dest, const u8 indices[16], const std::vector<TextureEntry>& textures, InputStream& data, InputStream& gs_ram, Game game);
static void unpack_level_texture(TextureAsset& dest, const TextureEntry& entry, InputStream& data, InputStream& gs_ram, Game game, s32 i);
static BuildAsset& build_from_level_core_asset(LevelCoreAsset& core);
static std::vector<s64> enumerate_asset_block_boundaries(InputStream& src, const LevelCoreHeader& header, Game game);
static ByteRange block_range(s32 ofs, const std::vector<s64>& block_bounds);
static void print_level_core_header(const LevelCoreHeader& header);

void unpack_level_core(LevelCoreAsset& dest, InputStream& src, ByteRange index_range, ByteRange data_range, ByteRange gs_ram_range, Game game) {
	SubInputStream index(src, index_range.bytes());
	std::vector<u8> decompressed_data;
	std::vector<u8> compressed_data = src.read_multiple<u8>(data_range.offset, data_range.size);
	decompress_wad(decompressed_data, compressed_data);
	MemoryInputStream data(decompressed_data);
	SubInputStream gs_ram(src, gs_ram_range.bytes());
	
	auto header = index.read<LevelCoreHeader>(0);
	std::vector<s64> block_bounds = enumerate_asset_block_boundaries(index, header, game);
	
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
	
	unpack_asset(dest.tfrags(), data, ByteRange{header.tfrags, tfrags_size}, game);
	unpack_asset(dest.occlusion(), data, block_range(header.occlusion, block_bounds), game);
	unpack_asset(dest.sky(), data, block_range(header.sky, block_bounds), game);
	unpack_asset(dest.collision(), data, block_range(header.collision, block_bounds), game);
	
	auto particle_textures = index.read_multiple<ParticleTextureEntry>(header.part_textures);
	auto fx_textures = index.read_multiple<FXTextureEntry>(header.fx_textures);
	
	CollectionAsset& tfrag_textures_collection = dest.tfrag_textures().switch_files();
	SubInputStream texture_data(data, header.textures_base_offset, data.size() - header.textures_base_offset);
	auto tfrag_textures = index.read_multiple<TextureEntry>(header.tfrag_textures);
	for(s32 i = 0; i < (s32) tfrag_textures.size(); i++) {
		unpack_level_texture(tfrag_textures_collection.child<TextureAsset>(i), tfrag_textures[i], texture_data, gs_ram, game, i);
	}
	
	//if(wad.game != Game::DL) {
	//	wad.unknown_a0 = assets.read_bytes(header.unknown_a0, 0x40, "unknown a0");
	//}
	
	unpack_moby_classes(dest, header, index, data, gs_ram, block_bounds, game);
	unpack_tie_classes(dest, header, index, data, gs_ram, block_bounds, game);
	unpack_shrub_classes(dest, header, index, data, gs_ram, block_bounds, game);
	
	if(game != Game::DL && header.ratchet_seqs_rac123 != 0) {
		CollectionAsset& ratchet_seqs = dest.ratchet_seqs();
		auto ratchet_seq_offsets = index.read_multiple<s32>(header.ratchet_seqs_rac123, 256);
		for(s32 i = 0; i < 256; i++) {
			unpack_asset(ratchet_seqs.child<BinaryAsset>(i), src, block_range(ratchet_seq_offsets[i], block_bounds), game);
		}
	}
	
	unpack_asset(dest.part_bank(), data, block_range(header.part_bank_offset, block_bounds), game);
	unpack_asset(dest.fx_bank(), data, block_range(header.fx_bank_offset, block_bounds), game);
	unpack_asset(dest.part_defs(), data, block_range(header.part_defs_offset, block_bounds), game);
	if(game != Game::RAC1) {
		unpack_asset(dest.sound_remap(), data, block_range(header.sound_remap_offset, block_bounds), game);
	}
	
	print_level_core_header(header);
}

static void unpack_moby_classes(LevelCoreAsset& core, const LevelCoreHeader& header, InputStream& index, InputStream& data, InputStream& gs_ram, const std::vector<s64>& block_bounds, Game game) {
	BuildAsset& build = build_from_level_core_asset(core);
	CollectionAsset& collection = build.child<CollectionAsset>("mobies");
	
	auto classes = index.read_multiple<MobyClassEntry>(header.moby_classes);
	auto textures = index.read_multiple<TextureEntry>(header.moby_textures);
	
	SubInputStream texture_data(data, header.textures_base_offset, data.size() - header.textures_base_offset);
	
	for(const MobyClassEntry& entry : classes) {
		std::string path = stringf("mobies/%d/moby%d.asset", entry.o_class, entry.o_class);
		MobyClassAsset& asset = collection.bank().asset_file(path).root().child<MobyClassAsset>(entry.o_class);
		asset.set_has_moby_table_entry(true);
		
		unpack_level_textures(asset.materials(), entry.textures, textures, texture_data, gs_ram, game);
		
		if(entry.offset_in_asset_wad != 0) {
			unpack_asset(asset.binary(), data, block_range(entry.offset_in_asset_wad, block_bounds), game);
			//if(entry.o_class >= 10 && !(game == Game::RAC3 && (entry.o_class == 5952 || entry.o_class == 5953))) {
			//	unpack_asset(asset, data, block_range(entry.offset_in_asset_wad, block_bounds), game);
			//}
		}
	}
}

static void unpack_tie_classes(LevelCoreAsset& core, const LevelCoreHeader& header, InputStream& index, InputStream& data, InputStream& gs_ram, const std::vector<s64>& block_bounds, Game game) {
	BuildAsset& build = build_from_level_core_asset(core);
	CollectionAsset& collection = build.child<CollectionAsset>("ties");
	
	auto classes = index.read_multiple<MobyClassEntry>(header.tie_classes);
	auto textures = index.read_multiple<TextureEntry>(header.tie_textures);
	
	SubInputStream texture_data(data, header.textures_base_offset, data.size() - header.textures_base_offset);
	
	for(const MobyClassEntry& entry : classes) {
		std::string path = stringf("ties/%d/tie%d.asset", entry.o_class, entry.o_class);
		TieClassAsset& asset = collection.bank().asset_file(path).root().child<TieClassAsset>(entry.o_class);
		
		unpack_level_textures(asset.textures(), entry.textures, textures, texture_data, gs_ram, game);
		
		if(entry.offset_in_asset_wad != 0) {
			unpack_asset(asset.binary(), data, block_range(entry.offset_in_asset_wad, block_bounds), game);
		}
	}
}

static void unpack_shrub_classes(LevelCoreAsset& core, const LevelCoreHeader& header, InputStream& index, InputStream& data, InputStream& gs_ram, const std::vector<s64>& block_bounds, Game game) {
	BuildAsset& build = build_from_level_core_asset(core);
	CollectionAsset& collection = build.child<CollectionAsset>("shrubs");
	
	auto classes = index.read_multiple<ShrubClassEntry>(header.shrub_classes);
	auto textures = index.read_multiple<TextureEntry>(header.shrub_textures);
	
	SubInputStream texture_data(data, header.textures_base_offset, data.size() - header.textures_base_offset);
	
	for(const ShrubClassEntry& entry : classes) {
		std::string path = stringf("shrubs/%d/shrub%d.asset", entry.o_class, entry.o_class);
		ShrubClassAsset& asset = collection.bank().asset_file(path).root().child<ShrubClassAsset>(entry.o_class);
		
		unpack_level_textures(asset.textures(), entry.textures, textures, texture_data, gs_ram, game);
		
		if(entry.offset_in_asset_wad != 0) {
			unpack_asset(asset.binary(), data, block_range(entry.offset_in_asset_wad, block_bounds), game);
		}
	}
}

static void unpack_level_textures(CollectionAsset& dest, const u8 indices[16], const std::vector<TextureEntry>& textures, InputStream& data, InputStream& gs_ram, Game game) {
	for(s32 i = 0; i < 16; i++) {
		if(indices[i] != 0xff) {
			const TextureEntry& texture = textures.at(indices[i]);
			unpack_level_texture(dest.child<TextureAsset>(i), texture, data, gs_ram, game, i);
		} else {
			break;
		}
	}
}

static void unpack_level_texture(TextureAsset& dest, const TextureEntry& entry, InputStream& data, InputStream& gs_ram, Game game, s32 i) {
	std::vector<u8> pixels = data.read_multiple<u8>(entry.data_offset, entry.width * entry.height);
	std::vector<u32> palette = gs_ram.read_multiple<u32>(entry.palette * 0x100, 256);
	Texture texture = Texture::create_8bit_paletted(entry.width, entry.height, pixels, palette);
	
	if(game == Game::DL) {
		texture.swizzle();
	}
	texture.swizzle_palette();
	
	auto [stream, ref] = dest.file().open_binary_file_for_writing(stringf("%d.png", i));
	write_png(*stream, texture);
	dest.set_src(ref);
}

// Only designed to work on assets that have just been unpacked.
static BuildAsset& build_from_level_core_asset(LevelCoreAsset& core) {
	assert(core.parent()); // LevelDataWadAsset
	assert(core.parent()->parent()); // LevelWadAsset
	assert(core.parent()->parent()->parent()); // LevelAsset
	assert(core.parent()->parent()->parent()->parent()); // CollectionAsset
	assert(core.parent()->parent()->parent()->parent()->parent()); // BuildAsset
	return core.parent()->parent()->parent()->parent()->parent()->as<BuildAsset>();
}

void pack_level_core(std::vector<u8>& index, std::vector<u8>& compressed_data, std::vector<u8>& gs_ram, LevelCoreAsset& src, Game game) {
	//LevelCoreHeader header = {0};
	//index.alloc<LevelCoreHeader>();
	//
	//core.pad(0x40);
	//header.tfrags = core.write_multiple(wad.tfrags);
	//core.pad(0x40);
	//header.occlusion = core.write_multiple(wad.occlusion);
	//core.pad(0x40);
	//header.sky = core.write_multiple(wad.sky);
	//core.pad(0x40);
	//header.collision = core.tell();
	//write_collision(core, wad.collision);
	//
	//// Allocate class tables.
	//index.pad(0x40);
	//header.moby_classes.count = 0;
	//for(const MobyClass& moby : wad.moby_classes) {
	//	if(!moby.has_asset_table_entry) {
	//		continue;
	//	}
	//	header.moby_classes.count++;
	//}
	//header.tie_classes.count = wad.tie_classes.size();
	//header.shrub_classes.count = wad.shrub_classes.size();
	//header.moby_classes.offset = index.alloc_multiple<MobyClassEntry>(header.moby_classes.count);
	//header.tie_classes.offset = index.alloc_multiple<TieClassEntry>(header.tie_classes.count);
	//header.shrub_classes.offset = index.alloc_multiple<ShrubClassEntry>(header.shrub_classes.count);
	//
	//TextureDedupeInput dedupe_input {
	//	wad.tfrag_textures,
	//	wad.moby_classes,
	//	wad.tie_classes,
	//	wad.shrub_classes
	//};
	//auto dedupe_output = prepare_texture_dedupe_records(dedupe_input);
	//deduplicate_textures(dedupe_output.records);
	//deduplicate_palettes(dedupe_output.records);
	//std::vector<GsRamEntry> gs_ram_table;
	//header.textures_base_offset = write_shared_texture_data(core, gs_ram, gs_ram_table, dedupe_output.records);
	//
	//// Write texture tables.
	//auto write_texture_table = [&](s32 table, size_t begin, size_t count) {
	//	s32 table_offset = index.tell();
	//	s32 table_count = 0;
	//	for(size_t i = 0; i < count; i++) {
	//		TextureDedupeRecord* record = &dedupe_output.records.at(begin + i);
	//		if(record->texture_out_edge > -1) {
	//			record = &dedupe_output.records[record->texture_out_edge];
	//		}
	//		if(record->texture != nullptr && !record->indices[table].has_value()) {
	//			const Texture& texture = *record->texture;
	//			assert(record->texture_offset != -1);
	//			TextureEntry entry;
	//			entry.data_offset = record->texture_offset - header.textures_base_offset;
	//			entry.width = texture.width;
	//			entry.height = texture.height;
	//			entry.unknown_8 = 3;
	//			TextureDedupeRecord* palette_record = record;
	//			if(palette_record->palette_out_edge > -1) {
	//				palette_record = &dedupe_output.records[palette_record->palette_out_edge];
	//			}
	//			assert(palette_record->palette_offset != -1);
	//			entry.palette = palette_record->palette_offset / 0x100;
	//			entry.mipmap = record->mipmap_offset / 0x100;
	//			record->indices[table] = table_count;
	//			index.write(entry);
	//			table_count++;
	//		}
	//	}
	//	return ArrayRange {table_count, table_offset};
	//};
	//header.tfrag_textures = write_texture_table(TFRAG_TEXTURE_INDEX, dedupe_output.tfrags_begin, wad.tfrag_textures.size());
	//header.moby_textures = write_texture_table(MOBY_TEXTURE_INDEX, dedupe_output.mobies_begin, wad.moby_classes.size() * 16);
	//header.tie_textures = write_texture_table(TIE_TEXTURE_INDEX, dedupe_output.ties_begin, wad.tie_classes.size() * 16);
	//header.shrub_textures = write_texture_table(SHRUB_TEXTURE_INDEX, dedupe_output.shrubs_begin, wad.shrub_classes.size() * 16);
	//
	//core.pad(0x100, 0);
	//header.part_bank_offset = core.tell();
	//header.part_textures = write_particle_textures(index, core, wad.particle_textures);
	//core.pad(0x100, 0);
	//header.fx_bank_offset = core.tell();
	//header.fx_textures = write_fx_textures(index, core, wad.fx_textures);
	//printf("Shared texture memory: 0x%x bytes\n", header.part_bank_offset - header.textures_base_offset);
	//
	//header.gs_ram.count = gs_ram_table.size();
	//header.gs_ram.offset = index.tell();
	//index.write_multiple(gs_ram_table);
	//
	//index.pad(0x10, 0);
	//header.unknown_a0 = core.tell();
	//core.write(wad.unknown_a0);
	//
	//// Write classes.
	//auto write_texture_list = [&](u8 dest[16], s32 begin, s32 table) {
	//	for(s32 i = 0; i < 16; i++) {
	//		TextureDedupeRecord* record = &dedupe_output.records.at(begin + i);
	//		if(record->texture != nullptr) {
	//			if(record->texture_out_edge > -1) {
	//				record = &dedupe_output.records[record->texture_out_edge];
	//			}
	//			assert(record->indices[table].has_value());
	//			verify(*record->indices[table] < 0xff, "Too many textures.\n");
	//			dest[i] = *record->indices[table];
	//		} else {
	//			dest[i] = 0xff;
	//		}
	//	}
	//};
	//
	//size_t mi = 0;
	//for(size_t i = 0; i < wad.moby_classes.size(); i++) {
	//	const MobyClass& cls = wad.moby_classes[i];
	//	ERROR_CONTEXT("moby %d", cls.o_class);
	//	
	//	if(!cls.has_asset_table_entry) {
	//		continue;
	//	}
	//	
	//	MobyClassEntry entry = {0};
	//	entry.o_class = cls.o_class;
	//	if(cls.model.has_value()) {
	//		core.pad(0x40);
	//		entry.offset_in_asset_wad = core.tell();
	//		if(entry.o_class > 10) {
	//			auto cd = read_moby_class(*cls.model, Game::RAC2);
	//			write_moby_class(core, cd, Game::RAC2);
	//		} else
	//		core.write_multiple(*cls.model);
	//	}
	//	write_texture_list(entry.textures, dedupe_output.mobies_begin + i * 16, MOBY_TEXTURE_INDEX);
	//	index.write(header.moby_classes.offset + (mi++) * sizeof(MobyClassEntry), entry);
	//}
	//
	//for(size_t i = 0; i < wad.tie_classes.size(); i++) {
	//	const TieClass& cls = wad.tie_classes[i];
	//	ERROR_CONTEXT("tie %d", cls.o_class);
	//	
	//	TieClassEntry entry = {0};
	//	entry.o_class = cls.o_class;
	//	core.pad(0x40);
	//	entry.offset_in_asset_wad = core.tell();
	//	write_texture_list(entry.textures, dedupe_output.ties_begin + i * 16, TIE_TEXTURE_INDEX);
	//	index.write(header.tie_classes.offset + i * sizeof(TieClassEntry), entry);
	//	core.write_multiple(cls.model);
	//}
	//
	//for(size_t i = 0; i < wad.shrub_classes.size(); i++) {
	//	const ShrubClass& cls = wad.shrub_classes[i];
	//	ERROR_CONTEXT("shrub %d", cls.o_class);
	//	
	//	ShrubClassEntry entry = {0};
	//	entry.o_class = cls.o_class;
	//	core.pad(0x40);
	//	entry.offset_in_asset_wad = core.tell();
	//	write_texture_list(entry.textures, dedupe_output.shrubs_begin + i * 16, SHRUB_TEXTURE_INDEX);
	//	index.write(header.shrub_classes.offset + i * sizeof(ShrubClassEntry), entry);
	//	core.write_multiple(cls.model);
	//}
	//
	//core.pad(0x10);
	//header.scene_view_size = core.tell();
	//
	//if(wad.game != Game::DL) {
	//	assert(wad.ratchet_seqs.size() == 256);
	//	std::vector<s32> ratchet_seq_offsets;
	//	for(const Opt<std::vector<u8>>& ratchet_seq : wad.ratchet_seqs) {
	//		if(ratchet_seq.has_value()) {
	//			core.pad(0x10);
	//			ratchet_seq_offsets.push_back(core.write_multiple(*ratchet_seq));
	//		} else {
	//			ratchet_seq_offsets.push_back(0);
	//		}
	//	}
	//	index.pad(0x10);
	//	header.ratchet_seqs_rac123 = index.write_multiple(ratchet_seq_offsets);
	//}
	//
	//index.pad(0x10);
	//header.part_defs_offset = index.tell();
	//index.write_multiple(wad.particle_defs);
	//
	//index.pad(0x10);
	//header.sound_remap_offset = index.tell();
	//index.write_multiple(wad.sound_remap);
	//
	//index.pad(0x10);
	//header.moby_gs_stash_list = index.tell();
	//index.write<s16>(-1);
	//
	//if(wad.game != Game::RAC1) {
	//	header.moby_gs_stash_count_rac23dl = 1;
	//}
	//
	//header.glass_map_texture = 0x4000;
	//header.glass_map_palette = 0x400;
	//
	//header.assets_decompressed_size = core.tell();
	//
	////print_level_core_header(header);
	//
	//index.write(0, header);
	////write_file("/tmp", "newheader.bin", Buffer(index.vec));
	////write_file("/tmp", "gsram.bin", Buffer(gs_ram.vec));
	////write_file("/tmp", "assets.bin", Buffer(core.vec));
}

static std::vector<s64> enumerate_asset_block_boundaries(InputStream& src, const LevelCoreHeader& header, Game game) {
	std::vector<s64> blocks {
		header.tfrags,
		header.occlusion,
		header.sky,
		header.collision,
		header.textures_base_offset,
		header.assets_decompressed_size
	};
	
	auto moby_classes = src.read_multiple<MobyClassEntry>(header.moby_classes.offset, header.moby_classes.count);
	for(const MobyClassEntry& entry : moby_classes) {
		blocks.push_back(entry.offset_in_asset_wad);
	}
	
	auto tie_classes = src.read_multiple<TieClassEntry>(header.tie_classes.offset, header.tie_classes.count);
	for(const TieClassEntry& entry : tie_classes) {
		blocks.push_back(entry.offset_in_asset_wad);
	}
	
	auto shrub_classes = src.read_multiple<ShrubClassEntry>(header.shrub_classes.offset, header.shrub_classes.count);
	for(const ShrubClassEntry& entry : shrub_classes) {
		blocks.push_back(entry.offset_in_asset_wad);
	}
	
	if(header.ratchet_seqs_rac123 != 0 && game != Game::DL) {
		auto ratchet_seqs = src.read_multiple<s32>(header.ratchet_seqs_rac123, 256);
		for(s32 ratchet_seq_ofs : ratchet_seqs) {
			if(ratchet_seq_ofs > 0) {
				blocks.push_back(ratchet_seq_ofs);
			}
		}
	}
	
	if(header.thing_table_offset_rac1 != 0 && game == Game::RAC1) {
		auto things = src.read_multiple<ThingEntry>(header.thing_table_offset_rac1, header.thing_table_count_rac1);
		for(const ThingEntry& entry : things) {
			blocks.push_back(entry.offset_in_asset_wad);
		}
	}
	
	return blocks;
}

static ByteRange block_range(s32 ofs, const std::vector<s64>& block_bounds) {
	if(ofs == 0) {
		// e.g. if there is no sky.
		return {0, 0};
	}
	s32 next_ofs = -1;
	for(s64 bound : block_bounds) {
		if(bound > ofs && (next_ofs == -1 || bound < next_ofs)) {
			next_ofs = bound;
		}
	}
	if(next_ofs != -1) {
		return {ofs, next_ofs - ofs};
	} else {
		return {0, 0};
	}
}

static void print_level_core_header(const LevelCoreHeader& header) {
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
	printf("%32s %8x", "index_into_some1_texs", header.index_into_some1_texs_rac2_maybe3);
	printf("%32s %8x", "moby_gs_stash_count_rac23dl", header.moby_gs_stash_count_rac23dl);
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
