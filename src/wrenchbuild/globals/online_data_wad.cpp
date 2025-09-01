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

static void unpack_online_data_wad(OnlineDataWadAsset& dest, InputStream& src, BuildConfig config);
static void pack_online_data_wad(OutputStream& dest, const OnlineDataWadAsset& src, BuildConfig config);

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
	/* 0x008 */ ByteRange images[73];
	/* 0x250 */ OnlineMobyHeader moby_classes[44];
)
static_assert(offsetof(OnlineDataHeader, moby_classes) == 0x250);

static void unpack_online_data_wad(OnlineDataWadAsset& dest, InputStream& src, BuildConfig config)
{
	auto header = src.read<OnlineDataHeader>(0);

	unpack_asset(dest.onlinew3d(), src, header.onlinew3d, config);
	unpack_compressed_assets<TextureAsset>(dest.images(SWITCH_FILES), src, ARRAY_PAIR(header.images), config, FMT_TEXTURE_PIF8);
	CollectionAsset& moby_classes = dest.moby_classes(SWITCH_FILES);
	for (s32 i = 0; i < ARRAY_SIZE(header.moby_classes); i++) {
		MobyClassAsset& moby = moby_classes.foreign_child<MobyClassAsset>(i);
		unpack_compressed_asset(moby.materials(), src, header.moby_classes[i].textures, config, FMT_COLLECTION_MATLIST_PIF8);
		unpack_compressed_asset(moby, src, header.moby_classes[i].core, config, FMT_MOBY_CLASS_PHAT);
	}
}

static void pack_online_data_wad(OutputStream& dest, const OnlineDataWadAsset& src, BuildConfig config)
{
	OnlineDataHeader header;
	dest.alloc<OnlineDataHeader>();
	
	header.onlinew3d = pack_asset_sa<ByteRange>(dest, src.get_onlinew3d(), config);
	pack_compressed_assets(dest, ARRAY_PAIR(header.images), src.get_images(), config, 0x10, "images", FMT_TEXTURE_PIF8);
	const CollectionAsset& moby_classes = src.get_moby_classes();
	for (s32 i = 0; i < ARRAY_SIZE(header.moby_classes); i++) {
		if (moby_classes.has_child(i)) {
			const MobyClassAsset& moby = moby_classes.get_child(i).as<MobyClassAsset>();
			header.moby_classes[i].core = pack_compressed_asset<ByteRange>(dest, moby.get_core(), config, 0x10, "moby_core", FMT_MOBY_CLASS_PHAT);
			if (moby.has_materials()) {
				header.moby_classes[i].textures = pack_compressed_asset<ByteRange>(dest, moby.get_materials(), config, 0x10, "textures", FMT_COLLECTION_MATLIST_PIF8);
			}
		}
	}
	
	dest.write(0, header);
}
