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

static void unpack_boot_wad(BootWadAsset& dest, InputStream& src, Game game);
static void pack_boot_wad(OutputStream& dest, BootWadAsset& src, Game game);

on_load(Boot, []() {
	BootWadAsset::funcs.unpack_dl = wrap_unpacker_func<BootWadAsset>(unpack_boot_wad);
	
	BootWadAsset::funcs.pack_dl = wrap_packer_func<BootWadAsset>(pack_boot_wad);
})

packed_struct(DeadlockedBootHeader,
	/* 0x00 */ ByteRange english;
	/* 0x08 */ ByteRange french;
	/* 0x10 */ ByteRange german;
	/* 0x18 */ ByteRange spanish;
	/* 0x20 */ ByteRange italian;
	/* 0x28 */ ByteRange hudwad[6];
	/* 0x58 */ ByteRange boot_plates[4];
	/* 0x78 */ ByteRange sram;
)

static void unpack_boot_wad(BootWadAsset& dest, InputStream& src, Game game) {
	auto header = src.read<DeadlockedBootHeader>(0);
	
	unpack_compressed_asset(dest.english(), src, header.english, game);
	unpack_compressed_asset(dest.french(), src, header.french, game);
	unpack_compressed_asset(dest.german(), src, header.german, game);
	unpack_compressed_asset(dest.spanish(), src, header.spanish, game);
	unpack_compressed_asset(dest.italian(), src, header.italian, game);
	unpack_asset(dest.hud().child<BinaryAsset>(0), src, header.hudwad[0], game);
	for(s32 i = 1; i < 6; i++) {
		unpack_compressed_asset(dest.hud().child<BinaryAsset>(i), src, header.hudwad[i], game);
	}
	unpack_compressed_assets<BinaryAsset>(dest.boot_plates().switch_files(), src, ARRAY_PAIR(header.boot_plates), game);
	unpack_compressed_asset(dest.sram(), src, header.sram, game);
}

static void pack_boot_wad(OutputStream& dest, BootWadAsset& src, Game game) {
	dest.pad(SECTOR_SIZE, 0);
	s64 begin = dest.tell();
	DeadlockedBootHeader header;
	dest.write(header);
	header.english = pack_compressed_asset_aligned<ByteRange>(dest, src.get_english(), game, begin, 0x40);
	header.french = pack_compressed_asset_aligned<ByteRange>(dest, src.get_french(), game, begin, 0x40);
	header.german = pack_compressed_asset_aligned<ByteRange>(dest, src.get_german(), game, begin, 0x40);
	header.spanish = pack_compressed_asset_aligned<ByteRange>(dest, src.get_spanish(), game, begin, 0x40);
	header.italian = pack_compressed_asset_aligned<ByteRange>(dest, src.get_italian(), game, begin, 0x40);
	if(src.hud().has_child(0)) {
		header.hudwad[0] = pack_asset_aligned<ByteRange>(dest, src.get_hud().get_child(0), game, begin, 0x40);
	}
	for(s32 i = 1; i < 6; i++) {
		if(src.hud().has_child(i)) {
			header.hudwad[i] = pack_compressed_asset_aligned<ByteRange>(dest, src.get_hud().get_child(i), game, begin, 0x40);
		}
	}
	pack_compressed_assets_aligned(dest, ARRAY_PAIR(header.boot_plates), src.get_boot_plates(), game, begin, 0x40);
	header.sram = pack_compressed_asset_aligned<ByteRange>(dest, src.get_sram(), game, begin, 0x40);
	dest.write(begin, header);
	s64 end = dest.tell();
}
