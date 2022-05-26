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

static void unpack_online_data_wad(OnlineDataWadAsset& dest, InputStream& src, Game game);
static void pack_online_data_wad(OutputStream& dest, OnlineDataWadAsset& src, Game game);

on_load(OnlineData, []() {
	OnlineDataWadAsset::funcs.unpack_dl = wrap_unpacker_func<OnlineDataWadAsset>(unpack_online_data_wad);
	
	OnlineDataWadAsset::funcs.pack_dl = wrap_packer_func<OnlineDataWadAsset>(pack_online_data_wad);
})

packed_struct(OnlineMobyHeader,
	/* 0x0 */ s32 o_class;
	/* 0x4 */ ByteRange core;
	/* 0xc */ ByteRange textures;
)

packed_struct(OnlineDataHeader,
	/* 0x000 */ ByteRange onlinew3d;
	/* 0x008 */ ByteRange eula_screen[2]; // 0,1
	/* 0x018 */ ByteRange buddies_list[2]; // 2,3
	/* 0x028 */ ByteRange unk1[6]; // 4,5,6,7,8,9
	/* 0x058 */ ByteRange maps[22]; // 10..31
	/* 0x108 */ ByteRange unk2[8]; // 32,33,34,35,36,37,38,39
	/* 0x158 */ ByteRange staging[2]; // 42,43
	/* 0x158 */ ByteRange unk3[2]; // 42,43
	/* 0x168 */ ByteRange save_level[11]; // 44..54
	/* 0x1c0 */ ByteRange online_menu[12]; // 55..66
	/* 0x220 */ ByteRange profile_screen[2]; // 67,68
	/* 0x230 */ ByteRange hero_image; // 69
	/* 0x238 */ ByteRange unk4; // 70
	/* 0x240 */ ByteRange unk5; // 71
	/* 0x248 */ ByteRange staging_options; // 72
	/* 0x250 */ OnlineMobyHeader moby_classes[44];
)
static_assert(offsetof(OnlineDataHeader, moby_classes) == 0x250);

static void unpack_online_data_wad(OnlineDataWadAsset& dest, InputStream& src, Game game) {
	auto header = src.read<OnlineDataHeader>(0);
	
	unpack_asset(dest.onlinew3d(), src, header.onlinew3d, game);
	unpack_compressed_assets<TextureAsset>(dest.eula_screen().switch_files(), src, ARRAY_PAIR(header.eula_screen), game, FMT_TEXTURE_PIF8);
	unpack_compressed_assets<TextureAsset>(dest.buddies_list().switch_files(), src, ARRAY_PAIR(header.buddies_list), game, FMT_TEXTURE_PIF8);
	unpack_compressed_assets<TextureAsset>(dest.maps().switch_files(), src, ARRAY_PAIR(header.maps), game, FMT_TEXTURE_PIF8);
	unpack_compressed_assets<TextureAsset>(dest.staging().switch_files(), src, ARRAY_PAIR(header.staging), game, FMT_TEXTURE_PIF8);
	unpack_compressed_assets<TextureAsset>(dest.save_level().switch_files(), src, ARRAY_PAIR(header.save_level), game, FMT_TEXTURE_PIF8);
	unpack_compressed_assets<TextureAsset>(dest.online_menu().switch_files(), src, ARRAY_PAIR(header.online_menu), game, FMT_TEXTURE_PIF8);
	unpack_compressed_assets<TextureAsset>(dest.profile_screen().switch_files(), src, ARRAY_PAIR(header.profile_screen), game, FMT_TEXTURE_PIF8);
	unpack_compressed_asset(dest.hero_image<TextureAsset>(), src, header.hero_image, game, FMT_TEXTURE_PIF8);
	unpack_compressed_asset(dest.staging_options<TextureAsset>(), src, header.staging_options, game, FMT_TEXTURE_PIF8);
	CollectionAsset& moby_classes = dest.moby_classes().switch_files();
	for(s32 i = 0; i < ARRAY_SIZE(header.moby_classes); i++) {
		MobyClassAsset& moby = moby_classes.child<MobyClassAsset>(i).switch_files();
		unpack_compressed_asset(moby.core<BinaryAsset>(), src, header.moby_classes[i].core, game);
		unpack_compressed_asset(moby.materials(), src, header.moby_classes[i].textures, game, FMT_COLLECTION_PIF8);
	}
}

static void pack_online_data_wad(OutputStream& dest, OnlineDataWadAsset& src, Game game) {
	OnlineDataHeader header;
	dest.alloc<OnlineDataHeader>();
	
	header.onlinew3d = pack_asset_sa<ByteRange>(dest, src.get_onlinew3d(), game);
	pack_compressed_assets(dest, ARRAY_PAIR(header.eula_screen), src.get_eula_screen(), game, 0x10, "eula_screen", FMT_TEXTURE_PIF8);
	pack_compressed_assets(dest, ARRAY_PAIR(header.buddies_list), src.get_buddies_list(), game, 0x10, "buddies_list", FMT_TEXTURE_PIF8);
	pack_compressed_assets(dest, ARRAY_PAIR(header.maps), src.get_maps(), game, 0x10, "maps", FMT_TEXTURE_PIF8);
	pack_compressed_assets(dest, ARRAY_PAIR(header.staging), src.get_staging(), game, 0x10, "staging", FMT_TEXTURE_PIF8);
	pack_compressed_assets(dest, ARRAY_PAIR(header.save_level), src.get_save_level(), game, 0x10, "save_level", FMT_TEXTURE_PIF8);
	pack_compressed_assets(dest, ARRAY_PAIR(header.online_menu), src.get_online_menu(), game, 0x10, "online_menu", FMT_TEXTURE_PIF8);
	pack_compressed_assets(dest, ARRAY_PAIR(header.profile_screen), src.get_profile_screen(), game, 0x10, "profile_screen", FMT_TEXTURE_PIF8);
	header.hero_image = pack_compressed_asset<ByteRange>(dest, src.get_hero_image(), game, 0x10, "hero_image", FMT_TEXTURE_PIF8);
	header.staging_options = pack_compressed_asset<ByteRange>(dest, src.get_staging_options(), game, 0x10, "staging_options", FMT_TEXTURE_PIF8);
	CollectionAsset& moby_classes = src.get_moby_classes();
	for(s32 i = 0; i < ARRAY_SIZE(header.moby_classes); i++) {
		if(moby_classes.has_child(i)) {
			MobyClassAsset& moby = moby_classes.get_child(i).as<MobyClassAsset>();
			header.moby_classes[i].core = pack_compressed_asset<ByteRange>(dest, moby.get_core(), game, 0x10, "moby_core");
			if(moby.has_materials()) {
				header.moby_classes[i].textures = pack_compressed_asset<ByteRange>(dest, moby.get_materials(), game, 0x10, "textures", FMT_COLLECTION_PIF8);
			}
		}
	}
	
	dest.write(0, header);
}
