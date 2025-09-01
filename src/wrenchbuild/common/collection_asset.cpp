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

#include <core/png.h>
#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>
#include <wrenchbuild/common/subtitles.h>

static void unpack_collection_asset(
	CollectionAsset& dest, InputStream& src, BuildConfig config, const char* hint);
static void pack_collection_asset(
	OutputStream& dest, const CollectionAsset& src, BuildConfig config, const char* hint);
static void unpack_texture_list(
	CollectionAsset& dest, InputStream& src, BuildConfig config, const char* hint);
static void pack_texture_list(
	OutputStream& dest, const CollectionAsset& src, BuildConfig config, const char* hint);
static void unpack_material_list(
	CollectionAsset& dest, InputStream& src, BuildConfig config, const char* hint);
static void pack_material_list(
	OutputStream& dest, const CollectionAsset& src, BuildConfig config, const char* hint);
static void unpack_mission_classes(CollectionAsset& dest, InputStream& src, BuildConfig config);
static void pack_mission_classes(OutputStream& dest, const CollectionAsset& src, BuildConfig config);

on_load(Collection, []() {
	CollectionAsset::funcs.unpack_rac1 = wrap_hint_unpacker_func<CollectionAsset>(unpack_collection_asset);
	CollectionAsset::funcs.unpack_rac2 = wrap_hint_unpacker_func<CollectionAsset>(unpack_collection_asset);
	CollectionAsset::funcs.unpack_rac3 = wrap_hint_unpacker_func<CollectionAsset>(unpack_collection_asset);
	CollectionAsset::funcs.unpack_dl = wrap_hint_unpacker_func<CollectionAsset>(unpack_collection_asset);
	
	CollectionAsset::funcs.pack_rac1 = wrap_hint_packer_func<CollectionAsset>(pack_collection_asset);
	CollectionAsset::funcs.pack_rac2 = wrap_hint_packer_func<CollectionAsset>(pack_collection_asset);
	CollectionAsset::funcs.pack_rac3 = wrap_hint_packer_func<CollectionAsset>(pack_collection_asset);
	CollectionAsset::funcs.pack_dl = wrap_hint_packer_func<CollectionAsset>(pack_collection_asset);
})

static void unpack_collection_asset(CollectionAsset& dest, InputStream& src, BuildConfig config, const char* hint)
{
	const char* type = next_hint(&hint);
	if (strcmp(type, "texlist") == 0) {
		unpack_texture_list(dest, src, config, hint);
	} else if (strcmp(type, "matlist") == 0) {
		unpack_material_list(dest, src, config, hint);
	} else if (strcmp(type, "subtitles") == 0) {
		unpack_subtitles(dest, src, config);
	} else if (strcmp(type, "missionclasses") == 0) {
		unpack_mission_classes(dest, src, config);
	} else {
		verify_not_reached("Invalid hint \"%s\" passed to collection asset unpacker.", hint);
	}
}

static void pack_collection_asset(OutputStream& dest, const CollectionAsset& src, BuildConfig config, const char* hint)
{
	const char* type = next_hint(&hint);
	if (strcmp(type, "texlist") == 0) {
		pack_texture_list(dest, src, config, hint);
	} else if (strcmp(type, "matlist") == 0) {
		pack_material_list(dest, src, config, hint);
	} else if (strcmp(type, "subtitles") == 0) {
		pack_subtitles(dest, src, config);
	} else if (strcmp(type, "missionclasses") == 0) {
		pack_mission_classes(dest, src, config);
	} else {
		verify_not_reached("Invalid hint \"%s\" passed to collection asset packer.", hint);
	}
}

static void unpack_texture_list(CollectionAsset& dest, InputStream& src, BuildConfig config, const char* hint)
{
	s32 count = src.read<s32>(0);
	verify(count < 0x1000, "matlist has too many elements and is probably corrupted.");
	src.seek(4);
	std::vector<s32> offsets = src.read_multiple<s32>(count);
	for (s32 i = 0; i < count; i++) {
		s32 offset = offsets[i];
		s32 size;
		if (i + 1 < count) {
			size = offsets[i + 1] - offsets[i];
		} else {
			size = src.size() - offsets[i];
		}
		unpack_asset(dest.child<TextureAsset>(i), src, ByteRange{offset, size}, config, hint);
	}
}

static void pack_texture_list(OutputStream& dest, const CollectionAsset& src, BuildConfig config, const char* hint)
{
	s32 count;
	for (count = 0; count < 256; count++) {
		if (!src.has_child(count)) {
			break;
		}
	}
	dest.write<s32>(count);
	
	std::vector<s32> offsets(count);
	dest.write_v(offsets);
	
	for (s32 i = 0; i < 256; i++) {
		if (src.has_child(i)) {
			offsets[i] = pack_asset<ByteRange>(dest, src.get_child(i), config, 0x10, hint).offset;
		} else {
			break;
		}
	}
	
	dest.seek(4);
	dest.write_v(offsets);
}

static void unpack_material_list(CollectionAsset& dest, InputStream& src, BuildConfig config, const char* hint)
{
	s32 count = src.read<s32>(0);
	verify(count < 0x1000, "texlist has too many elements and is probably corrupted.");
	src.seek(4);
	std::vector<s32> offsets = src.read_multiple<s32>(count);
	for (s32 i = 0; i < count; i++) {
		s32 offset = offsets[i];
		s32 size;
		if (i + 1 < count) {
			size = offsets[i + 1] - offsets[i];
		} else {
			size = src.size() - offsets[i];
		}
		unpack_asset(dest.child<MaterialAsset>(i).diffuse(), src, ByteRange{offset, size}, config, hint);
	}
}

static void pack_material_list(OutputStream& dest, const CollectionAsset& src, BuildConfig config, const char* hint)
{
	s32 count;
	for (count = 0; count < 256; count++) {
		if (!src.has_child(count)) {
			break;
		}
	}
	dest.write<s32>(count);
	
	std::vector<s32> offsets(count);
	dest.write_v(offsets);
	
	for (s32 i = 0; i < 256; i++) {
		if (src.has_child(i)) {
			offsets[i] = pack_asset<ByteRange>(dest, src.get_child(i).as<MaterialAsset>().get_diffuse(), config, 0x10, hint).offset;
		} else {
			break;
		}
	}
	
	dest.seek(4);
	dest.write_v(offsets);
}

packed_struct(MissionClassEntry,
	s32 o_class;
	s32 class_offset;
	s32 texture_list_offset;
	s32 pad;
)

static void unpack_mission_classes(CollectionAsset& dest, InputStream& src, BuildConfig config)
{
	s32 class_count = src.read<s32>(0);
	
	// Find the end of each block.
	std::vector<s32> block_bounds;
	for (s32 i = 0; i < class_count; i++) {
		MissionClassEntry entry = src.read<MissionClassEntry>(0x10 + i * sizeof(MissionClassEntry));
		block_bounds.push_back(entry.class_offset);
		block_bounds.push_back(entry.texture_list_offset);
	}
	block_bounds.emplace_back(src.size());
	
	for (s32 i = 0; i < class_count; i++) {
		MissionClassEntry entry = src.read<MissionClassEntry>(0x10 + i * sizeof(MissionClassEntry));
		std::string path = stringf("moby_classes/%d/moby%d.asset", entry.o_class, entry.o_class);
		MobyClassAsset& moby = dest.foreign_child<MobyClassAsset>(path, false, entry.o_class);
		moby.set_id(entry.o_class);
		moby.set_has_moby_table_entry(true);
		
		CollectionAsset& materials = moby.materials();
		if (entry.texture_list_offset != 0) {
			s32 end = 0;
			for (s32 bound : block_bounds) {
				if (bound > entry.texture_list_offset) {
					end = bound;
					break;
				}
			}
			
			ByteRange textures_range{entry.texture_list_offset, end - entry.texture_list_offset};
			unpack_asset(materials, src, textures_range, config, FMT_COLLECTION_MATLIST_PIF8_4MIPS);
		}
		
		if (entry.class_offset != 0) {
			s32 end = 0;
			for (s32 bound : block_bounds) {
				if (bound > entry.class_offset) {
					end = bound;
					break;
				}
			}
			
			ByteRange class_range{entry.class_offset, end - entry.class_offset};
			unpack_asset(moby, src, class_range, config, FMT_MOBY_CLASS_PHAT);
		}
	}
}

static void pack_mission_classes(OutputStream& dest, const CollectionAsset& src, BuildConfig config)
{
	s32 class_count = 0;
	src.for_each_logical_child_of_type<MobyClassAsset>([&](const MobyClassAsset& moby) {
		if (moby.has_moby_table_entry()) {
			class_count++;
		}
	});
	
	dest.write(class_count);
	dest.pad(0x10, 0);
	s64 list_ofs = dest.alloc_multiple<MissionClassEntry>(class_count);
	
	src.for_each_logical_child_of_type<MobyClassAsset>([&](const MobyClassAsset& moby) {
		if (moby.has_moby_table_entry()) {
			dest.pad(0x10, 0);
			MissionClassEntry entry = {};
			entry.o_class = moby.id();
			
			if (moby.has_core()) {
				entry.class_offset = pack_asset<ByteRange>(dest, moby, config, 0x10, FMT_MOBY_CLASS_PHAT).offset;
			}
			
			if (moby.has_materials()) {
				const CollectionAsset& materials = moby.get_materials();
				s32 material_count = 0;
				materials.for_each_logical_child_of_type<TextureAsset>([&](const TextureAsset&) {
					material_count++;
				});
				
				if (material_count > 0) {
					entry.texture_list_offset = pack_asset<ByteRange>(dest, moby.get_materials(), config, 0x10, FMT_COLLECTION_MATLIST_PIF8_4MIPS).offset;
				}
			}
			
			dest.write(list_ofs, entry);
			list_ofs += sizeof(MissionClassEntry);
		}
	});
}
