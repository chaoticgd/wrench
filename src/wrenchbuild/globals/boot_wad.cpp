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

static void unpack_boot_wad(BootWadAsset& dest, InputStream& src, BuildConfig config);
static void pack_boot_wad(OutputStream& dest, const BootWadAsset& src, BuildConfig config);

on_load(Boot, []() {
	BootWadAsset::funcs.unpack_rac3 = wrap_unpacker_func<BootWadAsset>(unpack_boot_wad);
	BootWadAsset::funcs.unpack_dl = wrap_unpacker_func<BootWadAsset>(unpack_boot_wad);
	
	BootWadAsset::funcs.pack_rac3 = wrap_packer_func<BootWadAsset>(pack_boot_wad);
	BootWadAsset::funcs.pack_dl = wrap_packer_func<BootWadAsset>(pack_boot_wad);
})

packed_struct(DlBootHeader,
	/* 0x00 */ ByteRange english;
	/* 0x08 */ ByteRange french;
	/* 0x10 */ ByteRange german;
	/* 0x18 */ ByteRange spanish;
	/* 0x20 */ ByteRange italian;
	/* 0x28 */ ByteRange hud_header;
	/* 0x30 */ ByteRange hud_banks[5];
	/* 0x58 */ ByteRange boot_plates[4];
	/* 0x78 */ ByteRange sram;
)

static void unpack_boot_wad(BootWadAsset& dest, InputStream& src, BuildConfig config)
{
	auto header = src.read<DlBootHeader>(0);
	
	unpack_compressed_asset(dest.english(), src, header.english, config);
	unpack_compressed_asset(dest.french(), src, header.french, config);
	unpack_compressed_asset(dest.german(), src, header.german, config);
	unpack_compressed_asset(dest.spanish(), src, header.spanish, config);
	unpack_compressed_asset(dest.italian(), src, header.italian, config);
	unpack_asset(dest.hud_header(), src, header.hud_header, config);
	unpack_compressed_assets<BinaryAsset>(dest.hud_banks(SWITCH_FILES), src, ARRAY_PAIR(header.hud_banks), config);
	unpack_compressed_assets<TextureAsset>(dest.boot_plates(SWITCH_FILES), src, ARRAY_PAIR(header.boot_plates), config, FMT_TEXTURE_RGBA);
	unpack_compressed_asset(dest.sram(), src, header.sram, config);
}

static void pack_boot_wad(OutputStream& dest, const BootWadAsset& src, BuildConfig config)
{
	DlBootHeader header;
	dest.write(header);
	
	header.english = pack_compressed_asset<ByteRange>(dest, src.get_english(), config, 0x40, "english");
	header.french = pack_compressed_asset<ByteRange>(dest, src.get_french(), config, 0x40, "french");
	header.german = pack_compressed_asset<ByteRange>(dest, src.get_german(), config, 0x40, "german");
	header.spanish = pack_compressed_asset<ByteRange>(dest, src.get_spanish(), config, 0x40, "spanish");
	header.italian = pack_compressed_asset<ByteRange>(dest, src.get_italian(), config, 0x40, "italian");
	header.hud_header = pack_asset<ByteRange>(dest, src.get_hud_header(), config, 0x40);
	pack_compressed_assets<ByteRange>(dest, ARRAY_PAIR(header.hud_banks), src.get_hud_banks(), config, 0x40, "hudwad");
	pack_compressed_assets(dest, ARRAY_PAIR(header.boot_plates), src.get_boot_plates(), config, 0x40, "bootplate", FMT_TEXTURE_RGBA);
	header.sram = pack_compressed_asset<ByteRange>(dest, src.get_sram(), config, 0x40, "sram");
	
	dest.write(0, header);
	s64 end = dest.tell();
}
