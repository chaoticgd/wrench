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

#include "level_core.h"

#include <core/timer.h>
#include <engine/moby_low.h>
#include <engine/compression.h>
#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>
#include <wrenchbuild/level/level_classes.h>
#include <wrenchbuild/level/tfrags_asset.h>

// This file is quite messy! Also the texture packing code needs to be redone!

static std::vector<s64> enumerate_level_core_block_boundaries(InputStream& src, const LevelCoreHeader& header, Game game);
static void print_level_core_header(const LevelCoreHeader& header);

void unpack_level_core(
	LevelWadAsset& dest,
	InputStream& src,
	ByteRange index_range,
	ByteRange data_range,
	ByteRange gs_ram_range,
	BuildConfig config)
{
	SubInputStream index(src, index_range.bytes());
	std::vector<u8> decompressed_data;
	std::vector<u8> compressed_data = src.read_multiple<u8>(data_range.offset, data_range.size);
	decompress_wad(decompressed_data, compressed_data);
	MemoryInputStream data(decompressed_data);
	SubInputStream gs_ram(src, gs_ram_range.bytes());
	
	auto header = index.read<LevelCoreHeader>(0);
	std::vector<s64> block_bounds = enumerate_level_core_block_boundaries(index, header, config.game());
	
	print_level_core_header(header);
	
	s32 tfrags_size;
	if (header.occlusion != 0) {
		tfrags_size = header.occlusion;
	} else if (header.sky != 0) {
		tfrags_size = header.sky;
	} else if (header.collision != 0) {
		tfrags_size = header.collision;
	} else {
		verify_not_reached("Unable to determine size of tfrag block.");
	}
	
	ChunkAsset& chunk = dest.chunks().foreign_child<ChunkAsset>(stringf("chunks/0/chunk_0.asset", 0, 0), false, 0);
	TfragsAsset& tfrags = chunk.tfrags(SWITCH_FILES);
	
	unpack_asset(tfrags, data, ByteRange{header.tfrags, tfrags_size}, config);
	if (header.occlusion) {
		unpack_asset(dest.occlusion(), data, level_core_block_range(header.occlusion, block_bounds), config);
	}
	if (header.sky) {
		unpack_asset(dest.sky<SkyAsset>(SWITCH_FILES), data, level_core_block_range(header.sky, block_bounds), config);
	}
	unpack_asset(chunk.collision<CollisionAsset>(SWITCH_FILES), data, level_core_block_range(header.collision, block_bounds), config);
	
	CollectionAsset& tfrag_textures_collection = tfrags.materials();
	SubInputStream texture_data(data, header.textures_base_offset, data.size() - header.textures_base_offset);
	auto tfrag_textures = index.read_multiple<TextureEntry>(header.tfrag_textures);
	for (s32 i = 0; i < (s32) tfrag_textures.size(); i++) {
		unpack_level_material(tfrag_textures_collection.child<MaterialAsset>(i), tfrag_textures[i], texture_data, gs_ram, config.game(), i);
	}
	
	SubInputStream part_defs(index, header.part_defs_offset, index.size() - header.part_defs_offset);
	std::vector<ParticleTextureEntry> part_entries = index.read_multiple<ParticleTextureEntry>(header.part_textures);
	SubInputStream part_bank(data, header.part_bank_offset, data.size() - header.part_bank_offset);
	unpack_particle_textures(dest.particle_textures(), part_defs, part_entries, part_bank, config.game());
	
	SubInputStream fx_bank(data, header.fx_bank_offset, data.size() - header.fx_bank_offset);
	auto fx_textures = index.read_multiple<FxTextureEntry>(header.fx_textures);
	unpack_fx_textures(dest, fx_textures, fx_bank, config.game());
	
	//if (wad.game != Game::DL) {
	//	wad.unknown_a0 = assets.read_bytes(header.unknown_a0, 0x40, "unknown a0");
	//}
	
	BuildAsset* build;
	if (config.is_testing()) {
		build = &dest.child<BuildAsset>("test_build");
	} else {
		build = &build_from_level_wad_asset(dest);
	}
	
	std::vector<GsRamEntry> gs_table;
	if (config.game() == Game::RAC) {
		gs_table = index.read_multiple<GsRamEntry>(header.gs_ram.offset, header.gs_ram.count);
	} else {
		gs_table = index.read_multiple<GsRamEntry>(header.gs_ram.offset, header.gs_ram.count + header.moby_gs_stash_count_rac23dl);
	}
	
	
	// List of classes that have their textures stored permanently in GS memory.
	std::set<s32> moby_stash;
	if (config.game() != Game::RAC) {
		for (s32 i = 0;; i++) {
			s16 o_class = index.read<s16>(header.moby_gs_stash_list + i * 2);
			if (o_class < 0) {
				break;
			}
			moby_stash.emplace(o_class);
		}
	}
	
	s32 moby_stash_addr = -1;
	if (config.game() != Game::RAC) {
		if (header.moby_gs_stash_count_rac23dl > 0) {
			moby_stash_addr = gs_table[header.gs_ram.count].address;
		}
	}
	
	// Unpack all the classes into the global directory and then create
	// references to them for the current level.
	CollectionAsset& moby_data = build->moby_classes();
	CollectionAsset& moby_refs = dest.moby_classes(SWITCH_FILES);
	unpack_moby_classes(moby_data, moby_refs, header, index, data, gs_table, gs_ram, block_bounds, config, moby_stash_addr, moby_stash);
	
	CollectionAsset& tie_data = build->tie_classes();
	CollectionAsset& tie_refs = dest.tie_classes(SWITCH_FILES);
	unpack_tie_classes(tie_data, tie_refs, header, index, data, gs_ram, block_bounds, config);
	
	CollectionAsset& shrub_data = build->shrub_classes();
	CollectionAsset& shrub_refs = dest.shrub_classes(SWITCH_FILES);
	unpack_shrub_classes(shrub_data, shrub_refs, header, index, data, gs_ram, block_bounds, config);
	
	if (config.game() == Game::DL) {
		SoundRemapHeader sound_remap = index.read<SoundRemapHeader>(header.sound_remap_offset);
		s32 sound_remap_size = sound_remap.third_part_ofs + sound_remap.third_part_count * 4;
		unpack_asset(dest.sound_remap(), index, ByteRange{header.sound_remap_offset, sound_remap_size}, config);
		
		MobySoundRemapHeader moby_remap = index.read<MobySoundRemapHeader>(header.moby_sound_remap_offset);
		unpack_asset(dest.moby_sound_remap(), index, ByteRange{header.moby_sound_remap_offset, moby_remap.size}, config);
	} else {
		SoundRemapHeader sound_remap = index.read<SoundRemapHeader>(header.sound_remap_offset);
		SoundRemapElement sound_remap_last = index.read<SoundRemapElement>(header.sound_remap_offset + sound_remap.second_part_ofs - 4);
		s32 sound_remap_size = sound_remap_last.offset + sound_remap_last.size * 4;
		unpack_asset(dest.sound_remap(), index, ByteRange{header.sound_remap_offset, sound_remap_size}, config);
	}
	
	if (config.game() != Game::DL && header.ratchet_seqs_rac123 != 0) {
		CollectionAsset& ratchet_seqs = dest.ratchet_seqs(SWITCH_FILES);
		auto ratchet_seq_offsets = index.read_multiple<s32>(header.ratchet_seqs_rac123, 256);
		for (s32 i = 0; i < 256; i++) {
			if (ratchet_seq_offsets[i] != 0) {
				unpack_asset(ratchet_seqs.child<BinaryAsset>(i), data, level_core_block_range(ratchet_seq_offsets[i], block_bounds), config);
			}
		}
	}
	
	if (config.game() == Game::RAC) {
		CollectionAsset& gadgets = dest.gadgets(SWITCH_FILES);
		auto gadget_entries = index.read_multiple<RacGadgetHeader>(header.gadget_offset_rac1, header.gadget_count_rac1);
		for (RacGadgetHeader& entry : gadget_entries) {
			ByteRange range{entry.offset_in_asset_wad, (s32) data.size() - entry.offset_in_asset_wad};
			MobyClassAsset& moby = gadgets.foreign_child<MobyClassAsset>(entry.class_number);
			moby.set_id(entry.class_number);
			unpack_compressed_asset(moby, data, range, config, FMT_MOBY_CLASS_PHAT);
		}
	}
}

void pack_level_core(
	std::vector<u8>& index_dest,
	std::vector<u8>& data_dest,
	std::vector<u8>& gs_ram_dest,
	const std::vector<LevelChunk>& chunks,
	const LevelWadAsset& src,
	BuildConfig config)
{
	MemoryOutputStream index(index_dest);
	MemoryOutputStream gs_ram(gs_ram_dest);
	
	std::vector<u8> uncompressed_data;
	BlackHoleOutputStream fake_data;
	MemoryOutputStream real_data(uncompressed_data);
	OutputStream* data_ptr;
	if (g_asset_packer_dry_run) {
		data_ptr = &fake_data;
	} else {
		data_ptr = &real_data;
	}
	OutputStream& data = *data_ptr;
	
	LevelCoreHeader header = {};
	index.alloc<LevelCoreHeader>();
	
	const CollectionAsset& chunk_collection = src.get_chunks();
	const ChunkAsset& first_chunk_asset = chunk_collection.get_child(0).as<ChunkAsset>();
	
	s32 max_tfrags_size = 0;
	s32 max_collision_size = 0;
	for (const LevelChunk& chunk : chunks) {
		max_tfrags_size = std::max(max_tfrags_size, (s32) chunk.tfrags.size());
		max_collision_size = std::max(max_collision_size, (s32) chunk.collision.size());
	}
	
	data.pad(0x40, 0);
	header.tfrags = (s32) data.tell();
	data.write_v(chunks[0].tfrags);
	// Insert padding so there's space for the tfrags from the other chunks.
	for (s32 i = 0; i < max_tfrags_size - chunks[0].tfrags.size(); i++) {
		data.write<u8>(0);
	}
	
	if (src.has_occlusion()) {
		const OcclusionAsset& occlusion_asset = src.get_occlusion();
		if (occlusion_asset.has_grid()) {
			header.occlusion = pack_asset<ByteRange>(data, occlusion_asset, config, 0x40).offset;
		}
	}
	if (src.has_sky()) {
		header.sky = pack_asset<ByteRange>(data, src.get_sky(), config, 0x40).offset;
	}
	data.pad(0x40, 0);
	header.collision = (s32) data.tell();
	data.write_v(chunks[0].collision);
	// Insert padding so there's space for the collision from the other chunks.
	for (s32 i = 0; i < max_collision_size - chunks[0].collision.size(); i++) {
		data.write<u8>(0);
	}
	
	const CollectionAsset& mobies = src.get_moby_classes();
	const CollectionAsset& ties = src.get_tie_classes();
	const CollectionAsset& shrubs = src.get_shrub_classes();
	
	auto [moby_tab, tie_tab, shrub_tab] = allocate_class_tables(index, mobies, ties, shrubs);
	header.moby_classes = moby_tab;
	header.tie_classes = tie_tab;
	header.shrub_classes = shrub_tab;
	
	SharedLevelTextures shared;
	std::vector<GsRamEntry> gs_table;
	std::vector<u8> part_defs;
	if (!g_asset_packer_dry_run) {
		shared = read_level_textures(first_chunk_asset.get_tfrags().get_materials(), mobies, ties, shrubs);
		
		for (LevelTexture& record : shared.textures) {
			if (record.texture.has_value()) {
				record.texture->to_8bit_paletted();
				record.texture->divide_alphas();
				record.texture->swizzle_palette();
				if (config.game() == Game::DL) {
					record.texture->swizzle();
				}
			}
		}
		
		deduplicate_level_textures(shared.textures);
		deduplicate_level_palettes(shared.textures);
		
		auto [textures_ofs, stash_count] = write_shared_level_textures(data, gs_ram, gs_table, shared.textures);
		header.textures_base_offset = textures_ofs;
		if (config.game() != Game::RAC) {
			header.moby_gs_stash_count_rac23dl = stash_count;
		}
		
		header.tfrag_textures = write_level_texture_table(index, shared.textures, shared.tfrag_range);
		header.moby_textures = write_level_texture_table(index, shared.textures, shared.moby_range);
		header.tie_textures = write_level_texture_table(index, shared.textures, shared.tie_range);
		header.shrub_textures = write_level_texture_table(index, shared.textures, shared.shrub_range);
		
		auto part_info = pack_particle_textures(index, data, src.get_particle_textures(), config.game());
		header.part_textures = std::get<0>(part_info);
		part_defs = std::get<1>(part_info);
		header.part_bank_offset = std::get<2>(part_info);
		auto [fx_textures, fx_bank_offset] = pack_fx_textures(index, data, src.get_fx_textures(), config.game());
		header.fx_textures = fx_textures;
		header.fx_bank_offset = fx_bank_offset;
		
		printf("Shared texture memory: 0x%x bytes\n", header.part_bank_offset - header.textures_base_offset);
	}
	
	if (config.game() == Game::RAC) {
		header.gs_ram.count = (s32) gs_table.size();
	} else {
		header.gs_ram.count = (s32) gs_table.size() - header.moby_gs_stash_count_rac23dl;
	}
	index.pad(0x10, 0);
	header.gs_ram.offset = index.tell();
	index.write_v(gs_table);
	
	if (!part_defs.empty()) {
		index.pad(0x10, 0);
		header.part_defs_offset = index.tell();
		index.write_v(part_defs);
	}
	
	const CollectionAsset& moby_classes = src.get_moby_classes();
	
	pack_moby_classes(index, data, moby_classes, shared.textures, moby_tab.offset, shared.moby_range.begin, config);
	pack_tie_classes(index, data, src.get_tie_classes(), shared.textures, tie_tab.offset, shared.tie_range.begin, config);
	pack_shrub_classes(index, data, src.get_shrub_classes(), shared.textures, shrub_tab.offset, shared.shrub_range.begin, config);
	
	data.pad(0x10, 0);
	header.scene_view_size = data.tell();
	
	if (src.has_sound_remap()) {
		header.sound_remap_offset = pack_asset<ByteRange>(index, src.get_sound_remap(), config, 0x10).offset;
	}
	if (src.has_moby_sound_remap() && config.game() == Game::DL) {
		header.moby_sound_remap_offset = pack_asset<ByteRange>(index, src.get_moby_sound_remap(), config, 0x10).offset;
	}
	
	if (config.game() == Game::GC || config.game() == Game::UYA) {
		index.pad(0x10, 0);
		header.moby_gs_stash_list = index.tell();
		moby_classes.for_each_logical_child_of_type<MobyClassAsset>([&](const MobyClassAsset& child) {
			if (child.stash_textures(false)) {
				index.write<s16>(child.id());
			}
		});
		index.write<s16>(-1);
	}
	
	if (src.has_ratchet_seqs() && config.game() != Game::DL) {
		const CollectionAsset& ratchet_seqs = src.get_ratchet_seqs();
		std::vector<s32> ratchet_seq_offsets(256, 0);
		for (s32 i = 0; i < 256; i++) {
			if (ratchet_seqs.has_child(i)) {
				ratchet_seq_offsets[i] = pack_asset<ByteRange>(data, ratchet_seqs.get_child(i), config, 0x10).offset;
			}
		}
		index.pad(0x10, 0);
		header.ratchet_seqs_rac123 = index.tell();
		index.write_v(ratchet_seq_offsets);
	}
	
	if (config.game() == Game::RAC) {
		std::vector<RacGadgetHeader> entries;
		const CollectionAsset& gadgets = src.get_gadgets();
		gadgets.for_each_logical_child_of_type<MobyClassAsset>([&](const MobyClassAsset& moby) {
			RacGadgetHeader entry = {};
			entry.class_number = moby.id();
			entry.offset_in_asset_wad = pack_compressed_asset<ByteRange>(data, moby, config, 0x40, "gadget", FMT_MOBY_CLASS_PHAT).offset;
			entry.compressed_size = data.tell() - entry.offset_in_asset_wad;
			entries.push_back(entry);
		});
		header.gadget_count_rac1 = (s32) entries.size();
		index.pad(0x10, 0);
		header.gadget_offset_rac1 = index.tell();
		index.write_v(entries);
	}
	
	if (config.game() == Game::DL) {
		index.pad(2, 0);
		header.moby_gs_stash_list = index.tell();
		moby_classes.for_each_logical_child_of_type<MobyClassAsset>([&](const MobyClassAsset& child) {
			if (child.stash_textures(false)) {
				index.write<s16>(child.id());
			}
		});
		index.write<s16>(-1);
	}
	
	index.pad(0x10, 0);
	
	header.glass_map_texture = 0x4000;
	header.glass_map_palette = 0x400;
	
	compress_wad(data_dest, uncompressed_data, "coredata", 8);
	header.assets_compressed_size = data_dest.size();
	header.assets_decompressed_size = uncompressed_data.size();
	
	if (!g_asset_packer_dry_run) {
		print_level_core_header(header);
	}
	
	index.write(0, header);
}

// Only designed to work on assets that have just been unpacked.
BuildAsset& build_from_level_wad_asset(LevelWadAsset& core)
{
	verify_fatal(core.parent()); // Level
	verify_fatal(core.parent()->parent()); // Collection
	verify_fatal(core.parent()->parent()->parent()); // Build
	Asset& build = *core.parent()->parent()->parent();
	for (Asset* asset = &build.highest_precedence(); asset != nullptr; asset = asset->lower_precedence()) {
		if (asset->logical_type() == BuildAsset::ASSET_TYPE) {
			return asset->as<BuildAsset>();
		}
	}
	verify_fatal(0);
}

ByteRange level_core_block_range(s32 ofs, const std::vector<s64>& block_bounds)
{
	if (ofs == 0) {
		// e.g. if there is no sky.
		return {0, 0};
	}
	s32 next_ofs = -1;
	for (s64 bound : block_bounds) {
		if (bound > ofs && (next_ofs == -1 || bound < next_ofs)) {
			next_ofs = bound;
		}
	}
	if (next_ofs != -1) {
		return {ofs, next_ofs - ofs};
	} else {
		return {0, 0};
	}
}

static std::vector<s64> enumerate_level_core_block_boundaries(InputStream& src, const LevelCoreHeader& header, Game game)
{
	std::vector<s64> blocks {
		header.tfrags,
		header.occlusion,
		header.sky,
		header.collision,
		header.textures_base_offset,
		header.assets_decompressed_size
	};
	
	auto moby_classes = src.read_multiple<MobyClassEntry>(header.moby_classes.offset, header.moby_classes.count);
	for (const MobyClassEntry& entry : moby_classes) {
		blocks.push_back(entry.offset_in_asset_wad);
	}
	
	auto tie_classes = src.read_multiple<TieClassEntry>(header.tie_classes.offset, header.tie_classes.count);
	for (const TieClassEntry& entry : tie_classes) {
		blocks.push_back(entry.offset_in_asset_wad);
	}
	
	auto shrub_classes = src.read_multiple<ShrubClassEntry>(header.shrub_classes.offset, header.shrub_classes.count);
	for (const ShrubClassEntry& entry : shrub_classes) {
		blocks.push_back(entry.offset_in_asset_wad);
	}
	
	if ((game != Game::DL) && header.moby_sound_remap_offset != 0) {
		blocks.push_back(header.moby_sound_remap_offset);
	}
	
	if (game != Game::DL && header.ratchet_seqs_rac123 != 0) {
		auto ratchet_seqs = src.read_multiple<s32>(header.ratchet_seqs_rac123, 256);
		for (s32 ratchet_seq_ofs : ratchet_seqs) {
			if (ratchet_seq_ofs > 0) {
				blocks.push_back(ratchet_seq_ofs);
			}
		}
	}
	
	if (game == Game::RAC && header.gadget_offset_rac1 != 0) {
		auto gadgets = src.read_multiple<RacGadgetHeader>(header.gadget_offset_rac1, header.gadget_count_rac1);
		for (const RacGadgetHeader& entry : gadgets) {
			blocks.push_back(entry.offset_in_asset_wad);
		}
	}
	
	return blocks;
}

static void print_level_core_header(const LevelCoreHeader& header)
{
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
