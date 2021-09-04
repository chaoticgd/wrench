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

#include "collision.h"
#include "texture.h"
#include "../lz/compression.h"
#include "../timer.h"

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

static std::vector<s64> enumerate_asset_block_boundaries(Buffer src, const AssetHeader& header); // Used to determining the size of certain blocks.
static s64 next_asset_block_size(s32 ofs, const std::vector<s64>& block_bounds);
static void print_asset_header(const AssetHeader& header);

void read_assets(LevelWad& wad, Buffer asset_header, Buffer assets, Buffer gs_ram) {
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
	auto particle_textures = asset_header.read_multiple<ParticleTextureEntry>(header.part_textures.offset, header.part_textures.count, "particle texture table");
	auto fx_textures = asset_header.read_multiple<FXTextureEntry>(header.fx_textures.offset, header.fx_textures.count, "fx texture table");
	
	Buffer texture_data = assets.subbuf(header.textures_base_offset);
	
	wad.tfrag_textures = read_tfrag_textures(tfrag_textures, texture_data, gs_ram);
	
	for(s64 i = 0; i < moby_classes.size(); i++) {
		const MobyClassEntry& entry = moby_classes[i];
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
		
		if(entry.offset_in_asset_wad != 0) {
			s64 model_size = next_asset_block_size(entry.offset_in_asset_wad, block_bounds);
			moby->model = assets.read_bytes(entry.offset_in_asset_wad, model_size, "moby model");
		}
		moby->textures = read_instance_textures(moby_textures, entry.textures, texture_data, gs_ram);
		moby->has_asset_table_entry = true;
	}
	
	for(s64 i = 0; i < tie_classes.size(); i++) {
		verify(tie_classes[i].offset_in_asset_wad != 0, "Tie class %d has no model.", i);
		TieClass tie;
		tie.o_class = tie_classes[i].o_class;
		s64 model_size = next_asset_block_size(tie_classes[i].offset_in_asset_wad, block_bounds);
		tie.model = assets.read_bytes(tie_classes[i].offset_in_asset_wad, model_size, "tie model");
		tie.textures = read_instance_textures(tie_textures, tie_classes[i].textures, texture_data, gs_ram);
		wad.tie_classes.emplace_back(std::move(tie));
	}
	
	for(s64 i = 0; i < shrub_classes.size(); i++) {
		verify(shrub_classes[i].offset_in_asset_wad != 0, "Shrub class %d has no model.", i);
		ShrubClass shrub;
		shrub.o_class = shrub_classes[i].o_class;
		s64 model_size = next_asset_block_size(shrub_classes[i].offset_in_asset_wad, block_bounds);
		shrub.model = assets.read_bytes(shrub_classes[i].offset_in_asset_wad, model_size, "shrub model");
		shrub.textures = read_instance_textures(shrub_textures, shrub_classes[i].textures, texture_data, gs_ram);
		wad.shrub_classes.emplace_back(std::move(shrub));
	}
	
	Buffer particle_data = assets.subbuf(header.part_bank_offset);
	wad.particle_textures = read_particle_textures(particle_textures, particle_data);
	
	Buffer fx_data = assets.subbuf(header.fx_bank_offset);
	wad.fx_textures = read_fx_textures(fx_textures, fx_data);
	
	wad.particle_defs = asset_header.read_bytes(header.part_defs_offset, header.sound_remap_offset - header.part_defs_offset, "particle defs");
	wad.sound_remap = asset_header.read_bytes(header.sound_remap_offset, header.moby_gs_stash_list - header.sound_remap_offset, "sound remap");
	if(header.light_cuboids_offset != 0) {
		wad.light_cuboids = asset_header.read_bytes(header.light_cuboids_offset, 1024, "light cuboids");
	}
	
	print_asset_header(header);
}

void write_assets(OutBuffer header_dest, std::vector<u8>& compressed_data_dest, OutBuffer gs_ram, const LevelWad& wad) {
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
	
	start_timer("Deduplicating textures");
	
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
				PalettedTexture* palette_texture = &texture;
				while(palette_texture->palette_out_edge > -1) {
					palette_texture = &paletted_textures[palette_texture->palette_out_edge];
				}
				assert(palette_texture->palette_offset != -1);
				entry.palette = palette_texture->palette_offset / 0x100;
				entry.mipmap = 0;
				if(i % 4 == 0) { // HACK: Gets around textures not being deduped properly.
					header_dest.write(entry);
					texture.indices[table] = table_count;
					table_count++;
				} else {
					texture.indices[table] = table_count - 1;
				}
			}
		}
		return ArrayRange {table_count, table_offset};
	};
	header.tfrag_textures = write_texture_table(TFRAG_TEXTURE_INDEX, layout.tfrags_begin, layout.mobies_begin);
	header.moby_textures = write_texture_table(MOBY_TEXTURE_INDEX, layout.mobies_begin, layout.ties_begin);
	header.tie_textures = write_texture_table(TIE_TEXTURE_INDEX, layout.ties_begin, layout.shrubs_begin);
	header.shrub_textures = write_texture_table(SHRUB_TEXTURE_INDEX, layout.shrubs_begin, paletted_textures.size());
	
	while(header_dest.vec.size() < 0x51d0) {
		header_dest.write<u8>(0);
	}
	while(data_dest.vec.size() < 0xaea100) {
		data_dest.write<u8>(0);
	}
	data_dest.pad(0x100, 0);
	header.part_bank_offset = data_dest.tell();
	header.part_textures = write_particle_textures(header_dest, data_dest, wad.particle_textures);
	data_dest.pad(0x100, 0);
	header.fx_bank_offset = data_dest.tell();
	header.fx_textures = write_fx_textures(header_dest, data_dest, wad.fx_textures);
	
	stop_timer();
	
	header.gs_ram.count = gs_ram_table.size();
	header.gs_ram.offset = header_dest.tell();
	header_dest.write_multiple(gs_ram_table);
	
	auto write_texture_list = [&](u8 dest[16], const std::vector<Texture>& textures, s32 o_class, s32 table, size_t first_index) {
		verify(textures.size() < 16, "Class %d has too many textures.\n", o_class);
		for(s32 i = 0; i < textures.size(); i++) {
			PalettedTexture* texture = &paletted_textures.at(first_index + i);
			if(texture->texture_out_edge > -1) {
				texture = &paletted_textures[texture->texture_out_edge];
			}
			assert(texture->is_first_occurence);
			assert(texture->texture_out_edge == -1);
			assert(texture->indices[table].has_value());
			verify(*texture->indices[table] < 0xff, "Too many textures.\n");
			dest[i] = *texture->indices[table];
		}
		for(s32 i = textures.size(); i < 16; i++) {
			dest[i] = 0xff;
		}
	};
	
	// Write classes.
	size_t i = 0;
	size_t class_index = 0;
	for(const MobyClass& cls : wad.moby_classes) {
		if(!cls.has_asset_table_entry) {
			continue;
		}
		
		MobyClassEntry entry = {0};
		entry.o_class = cls.o_class;
		if(cls.model.has_value()) {
			data_dest.pad(0x40);
			entry.offset_in_asset_wad = data_dest.tell();
			write_texture_list(entry.textures, cls.textures, cls.o_class, MOBY_TEXTURE_INDEX, layout.mobies_begin + class_index);
			class_index += cls.textures.size();
			data_dest.write_multiple(*cls.model);
		} else {
			for(s32 j = 0; j < 16; j++) {
				entry.textures[j] = 0xff;
			}
		}
		header_dest.write(header.moby_classes.offset + (i++) * sizeof(MobyClassEntry), entry);
	}
	
	i = 0;
	class_index = 0;
	for(const TieClass& cls : wad.tie_classes) {
		TieClassEntry entry = {0};
		entry.o_class = cls.o_class;
		data_dest.pad(0x40);
		entry.offset_in_asset_wad = data_dest.tell();
		write_texture_list(entry.textures, cls.textures, cls.o_class, TIE_TEXTURE_INDEX, layout.ties_begin + class_index);
		class_index += cls.textures.size();
		header_dest.write(header.tie_classes.offset + (i++) * sizeof(TieClassEntry), entry);
		data_dest.write_multiple(cls.model);
	}
	
	i = 0;
	class_index = 0;
	for(const ShrubClass& cls : wad.shrub_classes) {
		ShrubClassEntry entry = {0};
		entry.o_class = cls.o_class;
		data_dest.pad(0x40);
		entry.offset_in_asset_wad = data_dest.tell();
		write_texture_list(entry.textures, cls.textures, cls.o_class, SHRUB_TEXTURE_INDEX, layout.shrubs_begin + class_index);
		class_index += cls.textures.size();
		header_dest.write(header.shrub_classes.offset + (i++) * sizeof(ShrubClassEntry), entry);
		data_dest.write_multiple(cls.model);
	}
	
	//while(header_dest.vec.size() < 0x7b90) {
	//	header_dest.write<u8>(0);
	//}
	//header_dest.pad(0x10, 0);
	//header.part_defs_offset = header_dest.tell();
	//header_dest.write_multiple(wad.particle_defs);
	
	header.assets_base_address = 0x8a3700;
	
	header_dest.pad(0x10, 0);
	header.sound_remap_offset = header_dest.tell();
	header_dest.write_multiple(wad.sound_remap);
	
	header_dest.pad(0x10, 0);
	header.moby_gs_stash_list = header_dest.tell();
	header_dest.write<s16>(0x259);
	header_dest.write<s16>(0x25f);
	header_dest.write<s16>(0x260);
	header_dest.write<s16>(0x261);
	header_dest.write<s16>(0x50a);
	header_dest.write<s16>(0xa73);
	header_dest.write<s16>(0xb08);
	header_dest.write<s16>(-1);
	
	header_dest.pad(0x10, 0);
	header.light_cuboids_offset = header_dest.tell();
	header_dest.write_multiple(wad.light_cuboids);
	
	header.scene_view_size = 0x1321540;
	header.moby_gs_stash_count = 8;
	
	header.glass_map_texture = 0x4000;
	header.glass_map_palette = 0x400;
	
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
