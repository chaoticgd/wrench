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

#include "level_classes.h"

#include <assetmgr/asset_path_gen.h>
#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>
#include <wrenchbuild/level/level_core.h> // LevelCoreHeader

void unpack_moby_classes(
	CollectionAsset& data_dest,
	CollectionAsset& refs_dest,
	const LevelCoreHeader& header,
	InputStream& index,
	InputStream& data,
	const std::vector<GsRamEntry>& gs_table,
	InputStream& gs_ram,
	const std::vector<s64>& block_bounds,
	BuildConfig config,
	s32 moby_stash_addr,
	const std::set<s32>& moby_stash)
{
	auto classes = index.read_multiple<MobyClassEntry>(header.moby_classes);
	auto textures = index.read_multiple<TextureEntry>(header.moby_textures);
	
	SubInputStream texture_data(data, header.textures_base_offset, data.size() - header.textures_base_offset);
	
	for (const MobyClassEntry& entry : classes) {
		std::string path = generate_moby_class_asset_path(entry.o_class, data_dest);
		MobyClassAsset& asset = data_dest.foreign_child<MobyClassAsset>(path, true, entry.o_class);
		asset.set_id(entry.o_class);
		asset.set_has_moby_table_entry(true);
		
		bool stashed = moby_stash.contains(entry.o_class);
		if (stashed) {
			asset.set_stash_textures(true);
		}
		
		unpack_level_materials(asset.materials(), entry.textures, textures, texture_data, gs_ram, config.game(), stashed ? moby_stash_addr : -1);
		
		if (entry.offset_in_asset_wad != 0) {
			unpack_asset(asset, data, level_core_block_range(entry.offset_in_asset_wad, block_bounds), config, FMT_MOBY_CLASS_PHAT);
		}
		
		refs_dest.child<ReferenceAsset>(entry.o_class).set_asset(asset.absolute_link());
	}
}

void pack_moby_classes(
	OutputStream& index,
	OutputStream& core,
	const CollectionAsset& classes,
	const std::vector<LevelTexture>& textures,
	s32 table,
	s32 texture_index,
	BuildConfig config)
{
	s32 i = 0;
	classes.for_each_logical_child_of_type<MobyClassAsset>([&](const MobyClassAsset& child) {
		if (child.has_moby_table_entry()) {
			MobyClassEntry entry = {0};
			entry.o_class = child.id();
			if (child.has_core()) {
				entry.offset_in_asset_wad = pack_asset<ByteRange>(core, child, config, 0x40, FMT_MOBY_CLASS_PHAT).offset;
			}
			if (!g_asset_packer_dry_run) {
				write_level_texture_indices(entry.textures, textures, texture_index, MOBY_TEXTURE_TABLE);
				texture_index += 16;
			}
			index.write(table + (i++) * sizeof(MobyClassEntry), entry);
		}
	});
}

void unpack_tie_classes(
	CollectionAsset& data_dest,
	CollectionAsset& refs_dest,
	const LevelCoreHeader& header,
	InputStream& index,
	InputStream& data,
	InputStream& gs_ram,
	const std::vector<s64>& block_bounds,
	BuildConfig config)
{
	auto classes = index.read_multiple<MobyClassEntry>(header.tie_classes);
	auto textures = index.read_multiple<TextureEntry>(header.tie_textures);
	
	SubInputStream texture_data(data, header.textures_base_offset, data.size() - header.textures_base_offset);
	
	for (const MobyClassEntry& entry : classes) {
		std::string path = generate_tie_class_asset_path(entry.o_class, data_dest);
		TieClassAsset& asset = data_dest.foreign_child<TieClassAsset>(path, true, entry.o_class);
		asset.set_id(entry.o_class);
		
		unpack_level_materials(asset.materials(), entry.textures, textures, texture_data, gs_ram, config.game());
		
		if (entry.offset_in_asset_wad != 0) {
			unpack_asset(asset, data, level_core_block_range(entry.offset_in_asset_wad, block_bounds), config);
		}
		
		refs_dest.child<ReferenceAsset>(entry.o_class).set_asset(asset.absolute_link());
	}
}

void pack_tie_classes(
	OutputStream& index,
	OutputStream& core,
	const CollectionAsset& classes,
	const std::vector<LevelTexture>& textures,
	s32 table,
	s32 texture_index,
	BuildConfig config)
{
	s32 i = 0;
	classes.for_each_logical_child_of_type<TieClassAsset>([&](const TieClassAsset& child) {
		TieClassEntry entry = {0};
		entry.o_class = child.id();
		if (child.has_core()) {
			entry.offset_in_asset_wad = pack_asset<ByteRange>(core, child, config, 0x40).offset;
		}
		if (!g_asset_packer_dry_run) {
			write_level_texture_indices(entry.textures, textures, texture_index, TIE_TEXTURE_TABLE);
			texture_index += 16;
		}
		index.write(table + (i++) * sizeof(TieClassEntry), entry);
	});
}

void unpack_shrub_classes(
	CollectionAsset& data_dest,
	CollectionAsset& refs_dest,
	const LevelCoreHeader& header,
	InputStream& index,
	InputStream& data,
	InputStream& gs_ram,
	const std::vector<s64>& block_bounds,
	BuildConfig config)
{
	auto classes = index.read_multiple<ShrubClassEntry>(header.shrub_classes);
	auto textures = index.read_multiple<TextureEntry>(header.shrub_textures);
	
	SubInputStream texture_data(data, header.textures_base_offset, data.size() - header.textures_base_offset);
	
	for (const ShrubClassEntry& entry : classes) {
		std::string path = generate_shrub_class_asset_path(entry.o_class, data_dest);
		ShrubClassAsset& asset = data_dest.foreign_child<ShrubClassAsset>(path, true, entry.o_class);
		asset.set_id(entry.o_class);
		
		unpack_level_materials(asset.materials(), entry.textures, textures, texture_data, gs_ram, config.game());
		if (entry.billboard.texture_width != 0 && !g_asset_unpacker.dump_binaries) {
			unpack_shrub_billboard_texture(asset.billboard().texture(), entry.billboard, gs_ram, config.game());
		}
		
		if (entry.offset_in_asset_wad != 0) {
			unpack_asset(asset, data, level_core_block_range(entry.offset_in_asset_wad, block_bounds), config);
		}
		
		refs_dest.child<ReferenceAsset>(entry.o_class).set_asset(asset.absolute_link());
	}
	
	if (!classes.empty()) {
		s32 last_shrub_usage = level_core_block_range(classes.back().offset_in_asset_wad, block_bounds).size;
		s32 shrub_mem_usage = classes.back().offset_in_asset_wad - classes.front().offset_in_asset_wad + last_shrub_usage;
		printf("%d shrub mem: %dk\n", g_asset_unpacker.current_level_id, shrub_mem_usage / 1024);
	}
}

void pack_shrub_classes(
	OutputStream& index,
	OutputStream& core,
	const CollectionAsset& classes,
	const std::vector<LevelTexture>& textures,
	s32 table,
	s32 texture_index,
	BuildConfig config)
{
	s64 begin = core.tell();
	
	s32 i = 0;
	classes.for_each_logical_child_of_type<ShrubClassAsset>([&](const ShrubClassAsset& child) {
		ShrubClassEntry entry = {0};
		entry.o_class = child.id();
		entry.offset_in_asset_wad = pack_asset<ByteRange>(core, child, config, 0x40).offset;
		if (!g_asset_packer_dry_run) {
			write_level_texture_indices(entry.textures, textures, texture_index, SHRUB_TEXTURE_TABLE);
			texture_index += 16;
		}
		index.write(table + (i++) * sizeof(ShrubClassEntry), entry);
	});
	
	s64 end = core.tell();
	s32 shrub_mem_usage = (s32) ((end - begin) / 1024);
	if (!g_asset_packer_dry_run) {
		printf("%d shrub mem: %dk\n", g_asset_packer_current_level_id, shrub_mem_usage);
	}
}

std::array<ArrayRange, 3> allocate_class_tables(
	OutputStream& index,
	const CollectionAsset& mobies,
	const CollectionAsset& ties,
	const CollectionAsset& shrubs)
{
	ArrayRange moby, tie, shrub;
	moby.count = 0;
	mobies.for_each_logical_child_of_type<MobyClassAsset>([&](const MobyClassAsset& child) {
		if (!child.has_has_moby_table_entry() || child.has_moby_table_entry()) {
			moby.count++;
		}
	});
	tie.count = 0;
	ties.for_each_logical_child_of_type<TieClassAsset>([&](const TieClassAsset& child) {
		tie.count++;
	});
	shrub.count = 0;
	shrubs.for_each_logical_child_of_type<ShrubClassAsset>([&](const ShrubClassAsset& child) {
		shrub.count++;
	});
	index.pad(0x40, 0);
	moby.offset = index.alloc_multiple<MobyClassEntry>(moby.count);
	tie.offset = index.alloc_multiple<TieClassEntry>(tie.count);
	shrub.offset = index.alloc_multiple<ShrubClassEntry>(shrub.count);
	return {moby, tie, shrub};
}
