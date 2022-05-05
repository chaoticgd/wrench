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

#include <pakrac/asset_unpacker.h>
#include <pakrac/asset_packer.h>

packed_struct(ArmorHeader,
	/* 0x0 */ SectorRange mesh;
	/* 0x8 */ SectorRange textures;
)

packed_struct(Rac2ArmorWadHeader,
	/* 0x0 */ s32 header_size;
	/* 0x4 */ Sector32 sector;
	/* 0x8 */ ArmorHeader armors[15];
)	

packed_struct(Rac3ArmorWadHeader,
	/* 0x000 */ s32 header_size;
	/* 0x004 */ Sector32 sector;
	/* 0x008 */ ArmorHeader armors[14];
	/* 0x0e8 */ ArmorHeader multiplayer_armors[41];
	/* 0x378 */ SectorRange textures[2];
)	

packed_struct(DeadlockedArmorWadHeader,
	/* 0x000 */ s32 header_size;
	/* 0x004 */ Sector32 sector;
	/* 0x008 */ ArmorHeader armors[20];
	/* 0x148 */ SectorRange bot_textures[12];
	/* 0x1a8 */ SectorRange landstalker_textures[8];
	/* 0x1e8 */ SectorRange dropship_textures[8];
)

static void unpack_rac2_armor_wad(ArmorWadAsset& dest, InputStream& src, Game game);
static void pack_rac2_armor_wad(OutputStream& dest, std::vector<u8>* header_dest, ArmorWadAsset& src, Game game);
static void unpack_rac3_armor_wad(ArmorWadAsset& dest, InputStream& src, Game game);
static void pack_rac3_armor_wad(OutputStream& dest, std::vector<u8>* header_dest, ArmorWadAsset& src, Game game);
static void unpack_dl_armor_wad(ArmorWadAsset& dest, InputStream& src, Game game);
static void pack_dl_armor_wad(OutputStream& dest, std::vector<u8>* header_dest, ArmorWadAsset& src, Game game);
static void unpack_armors(CollectionAsset& dest, InputStream& src, ArmorHeader* headers, s32 count, Game game);
static void pack_armors(OutputStream& dest, ArmorHeader* headers, s32 count, CollectionAsset& src, Game game, s64 base);

on_load(Armor, []() {
	ArmorWadAsset::funcs.unpack_rac2 = wrap_wad_unpacker_func<ArmorWadAsset>(unpack_rac2_armor_wad);
	ArmorWadAsset::funcs.unpack_rac3 = wrap_wad_unpacker_func<ArmorWadAsset>(unpack_rac3_armor_wad);
	ArmorWadAsset::funcs.unpack_dl = wrap_wad_unpacker_func<ArmorWadAsset>(unpack_dl_armor_wad);
	
	ArmorWadAsset::funcs.pack_rac2 = wrap_wad_packer_func<ArmorWadAsset>(pack_rac2_armor_wad);
	ArmorWadAsset::funcs.pack_rac3 = wrap_wad_packer_func<ArmorWadAsset>(pack_rac3_armor_wad);
	ArmorWadAsset::funcs.pack_dl = wrap_wad_packer_func<ArmorWadAsset>(pack_dl_armor_wad);
})

static void unpack_rac2_armor_wad(ArmorWadAsset& dest, InputStream& src, Game game) {
	auto header = src.read<Rac2ArmorWadHeader>(0);
	
	unpack_armors(dest.armors(), src, ARRAY_PAIR(header.armors), game);
}

static void pack_rac2_armor_wad(OutputStream& dest, std::vector<u8>* header_dest, ArmorWadAsset& src, Game game) {
	s64 base = dest.tell();
	
	Rac2ArmorWadHeader header = {0};
	header.header_size = sizeof(Rac2ArmorWadHeader);
	dest.write(header);
	dest.pad(SECTOR_SIZE, 0);
	
	pack_armors(dest, ARRAY_PAIR(header.armors), src.get_armors(), game, base);
	
	dest.write(base, header);
	if(header_dest) {
		OutBuffer(*header_dest).write(0, header);
	}
}

static void unpack_rac3_armor_wad(ArmorWadAsset& dest, InputStream& src, Game game) {
	auto header = src.read<Rac3ArmorWadHeader>(0);
	
	unpack_armors(dest.armors(), src, ARRAY_PAIR(header.armors), game);
}

static void pack_rac3_armor_wad(OutputStream& dest, std::vector<u8>* header_dest, ArmorWadAsset& src, Game game) {
	s64 base = dest.tell();
	
	Rac3ArmorWadHeader header = {0};
	header.header_size = sizeof(Rac3ArmorWadHeader);
	dest.write(header);
	dest.pad(SECTOR_SIZE, 0);
	
	pack_armors(dest, ARRAY_PAIR(header.armors), src.get_armors(), game, base);
	
	dest.write(base, header);
	if(header_dest) {
		OutBuffer(*header_dest).write(0, header);
	}
}

static void unpack_dl_armor_wad(ArmorWadAsset& dest, InputStream& src, Game game) {
	auto header = src.read<DeadlockedArmorWadHeader>(0);
	
	unpack_armors(dest.armors(), src, ARRAY_PAIR(header.armors), game);
	unpack_assets<BinaryAsset>(dest.bot_textures().switch_files(), src, ARRAY_PAIR(header.bot_textures), game);
	unpack_assets<BinaryAsset>(dest.landstalker_textures().switch_files(), src, ARRAY_PAIR(header.landstalker_textures), game);
	unpack_assets<BinaryAsset>(dest.dropship_textures().switch_files(), src, ARRAY_PAIR(header.dropship_textures), game);
}

static void pack_dl_armor_wad(OutputStream& dest, std::vector<u8>* header_dest, ArmorWadAsset& src, Game game) {
	s64 base = dest.tell();
	
	DeadlockedArmorWadHeader header = {0};
	header.header_size = sizeof(DeadlockedArmorWadHeader);
	dest.write(header);
	dest.pad(SECTOR_SIZE, 0);
	
	pack_armors(dest, ARRAY_PAIR(header.armors), src.get_armors(), game, base);
	pack_assets_sa(dest, ARRAY_PAIR(header.bot_textures), src.get_bot_textures(), game, base);
	pack_assets_sa(dest, ARRAY_PAIR(header.landstalker_textures), src.get_landstalker_textures(), game, base);
	pack_assets_sa(dest, ARRAY_PAIR(header.dropship_textures), src.get_dropship_textures(), game, base);
	
	dest.write(base, header);
	if(header_dest) {
		OutBuffer(*header_dest).write(0, header);
	}
}

packed_struct(ArmorMeshHeader,
	u8 submesh_count;
	u8 low_lod_submesh_count;
	u8 metal_submesh_count;
	u8 metal_submesh_begin;
	s32 submesh_table;
	s32 gif_usage;
)

static void unpack_armors(CollectionAsset& dest, InputStream& src, ArmorHeader* headers, s32 count, Game game) {
	for(s32 i = 0; i < count; i++) {
		if(headers[i].mesh.size.sectors > 0) {
			Asset& armor_file = dest.switch_files(stringf("armors/%02d/armor%02d.asset", i, i));
			MobyClassAsset& moby = armor_file.child<MobyClassAsset>(std::to_string(i).c_str());
			CollectionAsset& materials = moby.materials();
			s32 texture_count = src.read<s32>(headers[i].textures.bytes().offset);
			verify(texture_count < 0x10, "Armor has too many textures in PIF list.");
			for(s32 j = 0; j < texture_count; j++) {
				s32 offset = src.read<s32>(headers[i].textures.bytes().offset + (j + 1) * 4);
				ByteRange range {
					(s32) headers[i].textures.bytes().offset + offset,
					(s32) headers[i].textures.bytes().size - offset
				};
				unpack_asset(materials.child<TextureAsset>(j), src, range, game, FMT_TEXTURE_PIF8);
			}
			if(g_asset_unpacker.dump_binaries) {
				BinaryAsset& bin = moby.mesh<BinaryAsset>();
				bin.set_asset_type("MobyClass");
				bin.set_format_hint(0);
				unpack_asset(bin, src, headers[i].mesh, game);
			} else {
				unpack_asset(moby, src, headers[i].mesh, game);
			}
		}
	}
}

static void pack_armors(OutputStream& dest, ArmorHeader* headers, s32 count, CollectionAsset& src, Game game, s64 base) {
	for(size_t i = 0; i < count; i++) {
		if(src.has_child(i)) {
			MobyClassAsset& moby = src.get_child(i).as<MobyClassAsset>();
			headers[i].mesh = pack_asset<SectorRange>(dest, moby, game, base);
			//headers[i].textures = pack_assets<SectorRange>(dest, moby.get_materials(), game, base);
		}
	}
}
