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

#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>

packed_struct(ArmorHeader,
	/* 0x0 */ SectorRange mesh;
	/* 0x8 */ SectorRange textures;
)

packed_struct(GcArmorWadHeader,
	/* 0x00 */ s32 header_size;
	/* 0x04 */ Sector32 sector;
	/* 0x08 */ ArmorHeader armors[9];
	/* 0x50 */ ArmorHeader wrenches[6];
)	

packed_struct(UyaArmorWadHeader,
	/* 0x000 */ s32 header_size;
	/* 0x004 */ Sector32 sector;
	/* 0x008 */ ArmorHeader armors[29];
	/* 0x1d8 */ ArmorHeader wrenches[6];
	/* 0x238 */ ArmorHeader multiplayer_armors[21];
	/* 0x388 */ SectorRange clank_textures[2];
)
static_assert(sizeof(UyaArmorWadHeader) == 0x398);

packed_struct(DlArmorWadHeader,
	/* 0x000 */ s32 header_size;
	/* 0x004 */ Sector32 sector;
	/* 0x008 */ ArmorHeader armors[20];
	/* 0x148 */ SectorRange bot_textures[12];
	/* 0x1a8 */ SectorRange landstalker_textures[8];
	/* 0x1e8 */ SectorRange dropship_textures[8];
)

static void unpack_gc_armor_wad(
	ArmorWadAsset& dest, const GcArmorWadHeader& header, InputStream& src, BuildConfig config);
static void pack_gc_armor_wad(
	OutputStream& dest, GcArmorWadHeader& header, const ArmorWadAsset& src, BuildConfig config);
static void unpack_uya_armor_wad(
	ArmorWadAsset& dest, const UyaArmorWadHeader& header, InputStream& src, BuildConfig config);
static void pack_uya_armor_wad(
	OutputStream& dest, UyaArmorWadHeader& header, const ArmorWadAsset& src, BuildConfig config);
static void unpack_dl_armor_wad(
	ArmorWadAsset& dest, const DlArmorWadHeader& header, InputStream& src, BuildConfig config);
static void pack_dl_armor_wad(
	OutputStream& dest, DlArmorWadHeader& header, const ArmorWadAsset& src, BuildConfig config);
static void unpack_armors(
	CollectionAsset& dest,
	InputStream& src,
	const ArmorHeader* headers,
	s32 count,
	BuildConfig config,
	const char* hint);
static void pack_armors(
	OutputStream& dest,
	ArmorHeader* headers,
	s32 count,
	const CollectionAsset& src,
	BuildConfig config,
	const char* hint);

on_load(Armor, []() {
	ArmorWadAsset::funcs.unpack_rac2 = wrap_wad_unpacker_func<ArmorWadAsset, GcArmorWadHeader>(unpack_gc_armor_wad);
	ArmorWadAsset::funcs.unpack_rac3 = wrap_wad_unpacker_func<ArmorWadAsset, UyaArmorWadHeader>(unpack_uya_armor_wad);
	ArmorWadAsset::funcs.unpack_dl = wrap_wad_unpacker_func<ArmorWadAsset, DlArmorWadHeader>(unpack_dl_armor_wad);
	
	ArmorWadAsset::funcs.pack_rac2 = wrap_wad_packer_func<ArmorWadAsset, GcArmorWadHeader>(pack_gc_armor_wad);
	ArmorWadAsset::funcs.pack_rac3 = wrap_wad_packer_func<ArmorWadAsset, UyaArmorWadHeader>(pack_uya_armor_wad);
	ArmorWadAsset::funcs.pack_dl = wrap_wad_packer_func<ArmorWadAsset, DlArmorWadHeader>(pack_dl_armor_wad);
})

static void unpack_gc_armor_wad(
	ArmorWadAsset& dest, const GcArmorWadHeader& header, InputStream& src, BuildConfig config)
{
	unpack_armors(dest.armors(SWITCH_FILES), src, ARRAY_PAIR(header.armors), config, "meshonly,0.145833328,true");
	unpack_armors(dest.wrenches(SWITCH_FILES), src, ARRAY_PAIR(header.wrenches), config, "meshonly,1,true");
}

static void pack_gc_armor_wad(
	OutputStream& dest, GcArmorWadHeader& header, const ArmorWadAsset& src, BuildConfig config)
{
	pack_armors(dest, ARRAY_PAIR(header.armors), src.get_armors(), config, "meshonly");
	pack_armors(dest, ARRAY_PAIR(header.wrenches), src.get_wrenches(), config, "meshonly");
}

static void unpack_uya_armor_wad(
	ArmorWadAsset& dest, const UyaArmorWadHeader& header, InputStream& src, BuildConfig config)
{
	unpack_armors(dest.armors(SWITCH_FILES), src, ARRAY_PAIR(header.armors), config, "meshonly,0.145833328,true");
	unpack_armors(dest.wrenches(SWITCH_FILES), src, ARRAY_PAIR(header.wrenches), config, "meshonly,0.0833333358,true"); // TODO: Check if this scale is correct.
	unpack_armors(dest.multiplayer_armors(SWITCH_FILES), src, ARRAY_PAIR(header.multiplayer_armors), config, FMT_MOBY_CLASS_PHAT);
	unpack_assets<TextureAsset>(dest.clank_textures(SWITCH_FILES), src, ARRAY_PAIR(header.clank_textures), config, FMT_TEXTURE_PIF8, true);
}

static void pack_uya_armor_wad(
	OutputStream& dest, UyaArmorWadHeader& header, const ArmorWadAsset& src, BuildConfig config)
{
	pack_armors(dest, ARRAY_PAIR(header.armors), src.get_armors(), config, "meshonly");
	pack_armors(dest, ARRAY_PAIR(header.wrenches), src.get_wrenches(), config, "meshonly");
	pack_armors(dest, ARRAY_PAIR(header.multiplayer_armors), src.get_multiplayer_armors(), config, FMT_MOBY_CLASS_PHAT);
	pack_assets_sa(dest, ARRAY_PAIR(header.clank_textures), src.get_clank_textures(), config, FMT_TEXTURE_PIF8);
}

static void unpack_dl_armor_wad(
	ArmorWadAsset& dest, const DlArmorWadHeader& header, InputStream& src, BuildConfig config)
{
	unpack_armors(dest.armors(SWITCH_FILES), src, ARRAY_PAIR(header.armors), config, FMT_MOBY_CLASS_PHAT);
	unpack_assets<CollectionAsset>(dest.bot_textures(SWITCH_FILES), src, ARRAY_PAIR(header.bot_textures), config, FMT_COLLECTION_PIF8, true);
	unpack_assets<CollectionAsset>(dest.landstalker_textures(SWITCH_FILES), src, ARRAY_PAIR(header.landstalker_textures), config, FMT_COLLECTION_PIF8, true);
	unpack_assets<CollectionAsset>(dest.dropship_textures(SWITCH_FILES), src, ARRAY_PAIR(header.dropship_textures), config, FMT_COLLECTION_PIF8, true);
}

static void pack_dl_armor_wad(
	OutputStream& dest, DlArmorWadHeader& header, const ArmorWadAsset& src, BuildConfig config)
{
	pack_armors(dest, ARRAY_PAIR(header.armors), src.get_armors(), config, FMT_MOBY_CLASS_PHAT);
	pack_assets_sa(dest, ARRAY_PAIR(header.bot_textures), src.get_bot_textures(), config, FMT_COLLECTION_PIF8);
	pack_assets_sa(dest, ARRAY_PAIR(header.landstalker_textures), src.get_landstalker_textures(), config, FMT_COLLECTION_PIF8);
	pack_assets_sa(dest, ARRAY_PAIR(header.dropship_textures), src.get_dropship_textures(), config, FMT_COLLECTION_PIF8);
}

packed_struct(ArmorMeshHeader,
	u8 submesh_count;
	u8 low_lod_submesh_count;
	u8 metal_submesh_count;
	u8 metal_submesh_begin;
	s32 submesh_table;
	s32 gif_usage;
)

static void unpack_armors(
	CollectionAsset& dest,
	InputStream& src,
	const ArmorHeader* headers,
	s32 count,
	BuildConfig config,
	const char* hint)
{
	for (s32 i = 0; i < count; i++) {
		if (headers[i].mesh.size.sectors > 0) {
			MobyClassAsset& moby = dest.foreign_child<MobyClassAsset>(i);
			unpack_asset(moby.materials(), src, headers[i].textures, config, FMT_COLLECTION_MATLIST_PIF8);
			unpack_asset(moby, src, headers[i].mesh, config, hint);
		}
	}
}

static void pack_armors(
	OutputStream& dest,
	ArmorHeader* headers,
	s32 count,
	const CollectionAsset& src,
	BuildConfig config,
	const char* hint)
{
	for (size_t i = 0; i < count; i++) {
		if (src.has_child(i)) {
			const MobyClassAsset& moby = src.get_child(i).as<MobyClassAsset>();
			headers[i].mesh = pack_asset_sa<SectorRange>(dest, moby, config, hint);
			headers[i].textures = pack_asset_sa<SectorRange>(dest, moby.get_materials(), config, FMT_COLLECTION_MATLIST_PIF8);
		}
	}
}
