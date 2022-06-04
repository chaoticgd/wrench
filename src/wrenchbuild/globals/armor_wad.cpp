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
	/* 0x0 */ s32 header_size;
	/* 0x4 */ Sector32 sector;
	/* 0x8 */ ArmorHeader armors[15];
)	

packed_struct(UyaArmorWadHeader,
	/* 0x000 */ s32 header_size;
	/* 0x004 */ Sector32 sector;
	/* 0x008 */ ArmorHeader armors[14];
	/* 0x0e8 */ ArmorHeader multiplayer_armors[41];
	/* 0x378 */ SectorRange textures[2];
)	

packed_struct(DlArmorWadHeader,
	/* 0x000 */ s32 header_size;
	/* 0x004 */ Sector32 sector;
	/* 0x008 */ ArmorHeader armors[20];
	/* 0x148 */ SectorRange bot_textures[12];
	/* 0x1a8 */ SectorRange landstalker_textures[8];
	/* 0x1e8 */ SectorRange dropship_textures[8];
)

static void unpack_rac2_armor_wad(ArmorWadAsset& dest, const GcArmorWadHeader& header, InputStream& src, Game game);
static void pack_rac2_armor_wad(OutputStream& dest, GcArmorWadHeader& header, const ArmorWadAsset& src, Game game);
static void unpack_rac3_armor_wad(ArmorWadAsset& dest, const UyaArmorWadHeader& header, InputStream& src, Game game);
static void pack_rac3_armor_wad(OutputStream& dest, UyaArmorWadHeader& header, const ArmorWadAsset& src, Game game);
static void unpack_dl_armor_wad(ArmorWadAsset& dest, const DlArmorWadHeader& header, InputStream& src, Game game);
static void pack_dl_armor_wad(OutputStream& dest, DlArmorWadHeader& header, const ArmorWadAsset& src, Game game);
static void unpack_armors(CollectionAsset& dest, InputStream& src, const ArmorHeader* headers, s32 count, Game game);
static void pack_armors(OutputStream& dest, ArmorHeader* headers, s32 count, const CollectionAsset& src, Game game);

on_load(Armor, []() {
	ArmorWadAsset::funcs.unpack_rac2 = wrap_wad_unpacker_func<ArmorWadAsset, GcArmorWadHeader>(unpack_rac2_armor_wad);
	ArmorWadAsset::funcs.unpack_rac3 = wrap_wad_unpacker_func<ArmorWadAsset, UyaArmorWadHeader>(unpack_rac3_armor_wad);
	ArmorWadAsset::funcs.unpack_dl = wrap_wad_unpacker_func<ArmorWadAsset, DlArmorWadHeader>(unpack_dl_armor_wad);
	
	ArmorWadAsset::funcs.pack_rac2 = wrap_wad_packer_func<ArmorWadAsset, GcArmorWadHeader>(pack_rac2_armor_wad);
	ArmorWadAsset::funcs.pack_rac3 = wrap_wad_packer_func<ArmorWadAsset, UyaArmorWadHeader>(pack_rac3_armor_wad);
	ArmorWadAsset::funcs.pack_dl = wrap_wad_packer_func<ArmorWadAsset, DlArmorWadHeader>(pack_dl_armor_wad);
})

static void unpack_rac2_armor_wad(ArmorWadAsset& dest, const GcArmorWadHeader& header, InputStream& src, Game game) {
	unpack_armors(dest.armors(), src, ARRAY_PAIR(header.armors), game);
}

static void pack_rac2_armor_wad(OutputStream& dest, GcArmorWadHeader& header, const ArmorWadAsset& src, Game game) {
	pack_armors(dest, ARRAY_PAIR(header.armors), src.get_armors(), game);
}

static void unpack_rac3_armor_wad(ArmorWadAsset& dest, const UyaArmorWadHeader& header, InputStream& src, Game game) {
	unpack_armors(dest.armors(), src, ARRAY_PAIR(header.armors), game);
}

static void pack_rac3_armor_wad(OutputStream& dest, UyaArmorWadHeader& header, const ArmorWadAsset& src, Game game) {
	pack_armors(dest, ARRAY_PAIR(header.armors), src.get_armors(), game);
}

static void unpack_dl_armor_wad(ArmorWadAsset& dest, const DlArmorWadHeader& header, InputStream& src, Game game) {
	unpack_armors(dest.armors(), src, ARRAY_PAIR(header.armors), game);
	unpack_assets<BinaryAsset>(dest.bot_textures().switch_files(), src, ARRAY_PAIR(header.bot_textures), game);
	unpack_assets<BinaryAsset>(dest.landstalker_textures().switch_files(), src, ARRAY_PAIR(header.landstalker_textures), game);
	unpack_assets<BinaryAsset>(dest.dropship_textures().switch_files(), src, ARRAY_PAIR(header.dropship_textures), game);
}

static void pack_dl_armor_wad(OutputStream& dest, DlArmorWadHeader& header, const ArmorWadAsset& src, Game game) {
	pack_armors(dest, ARRAY_PAIR(header.armors), src.get_armors(), game);
	pack_assets_sa(dest, ARRAY_PAIR(header.bot_textures), src.get_bot_textures(), game);
	pack_assets_sa(dest, ARRAY_PAIR(header.landstalker_textures), src.get_landstalker_textures(), game);
	pack_assets_sa(dest, ARRAY_PAIR(header.dropship_textures), src.get_dropship_textures(), game);
}

packed_struct(ArmorMeshHeader,
	u8 submesh_count;
	u8 low_lod_submesh_count;
	u8 metal_submesh_count;
	u8 metal_submesh_begin;
	s32 submesh_table;
	s32 gif_usage;
)

static void unpack_armors(CollectionAsset& dest, InputStream& src, const ArmorHeader* headers, s32 count, Game game) {
	for(s32 i = 0; i < count; i++) {
		if(headers[i].mesh.size.sectors > 0) {
			Asset& armor_file = dest.switch_files(stringf("armors/%02d/armor%02d.asset", i, i));
			MobyClassAsset& moby = armor_file.child<MobyClassAsset>(std::to_string(i).c_str());
			unpack_asset(moby.core<BinaryAsset>(), src, headers[i].mesh, game);
			unpack_asset(moby.materials(), src, headers[i].textures, game, FMT_COLLECTION_PIF8);
		}
	}
}

static void pack_armors(OutputStream& dest, ArmorHeader* headers, s32 count, const CollectionAsset& src, Game game) {
	for(size_t i = 0; i < count; i++) {
		if(src.has_child(i)) {
			const MobyClassAsset& moby = src.get_child(i).as<MobyClassAsset>();
			headers[i].mesh = pack_asset_sa<SectorRange>(dest, moby.get_core(), game);
			headers[i].textures = pack_asset_sa<SectorRange>(dest, moby.get_materials(), game, FMT_COLLECTION_PIF8);
		}
	}
}