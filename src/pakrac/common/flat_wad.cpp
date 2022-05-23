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
#include <pakrac/asset_unpacker.h>
#include <pakrac/asset_packer.h>

static void unpack_flat_wad_asset(FlatWadAsset& dest, InputStream& src, Game game);
static void unpack_image(FlatWadAsset& dest, InputStream& src, s32 offset, Game game);
static bool is_common_texture_size(s32 number);
static void pack_flat_wad_asset(OutputStream& dest, FlatWadAsset& src, Game game);

on_load(FlatWad, []() {
	FlatWadAsset::funcs.unpack_rac1 = wrap_unpacker_func<FlatWadAsset>(unpack_flat_wad_asset);
	FlatWadAsset::funcs.unpack_rac2 = wrap_unpacker_func<FlatWadAsset>(unpack_flat_wad_asset);
	FlatWadAsset::funcs.unpack_rac3 = wrap_unpacker_func<FlatWadAsset>(unpack_flat_wad_asset);
	FlatWadAsset::funcs.unpack_dl = wrap_unpacker_func<FlatWadAsset>(unpack_flat_wad_asset);
	
	FlatWadAsset::funcs.pack_rac1 = wrap_packer_func<FlatWadAsset>(pack_flat_wad_asset);
	FlatWadAsset::funcs.pack_rac2 = wrap_packer_func<FlatWadAsset>(pack_flat_wad_asset);
	FlatWadAsset::funcs.pack_rac3 = wrap_packer_func<FlatWadAsset>(pack_flat_wad_asset);
	FlatWadAsset::funcs.pack_dl = wrap_packer_func<FlatWadAsset>(pack_flat_wad_asset);
})

static void unpack_flat_wad_asset(FlatWadAsset& dest, InputStream& src, Game game) {
	s32 header_size = src.read<s32>(0);
	std::vector<SectorRange> ranges = src.read_multiple<SectorRange>(0x8, header_size / 0x8);
	for(size_t i = 0; i < ranges.size(); i++) {
		s32 offset = 0x8 + i * 0x8;
		unpack_asset(dest.child<BinaryAsset>(stringf("%04d_%04x", offset, offset).c_str()), src, ranges[i], game);
		SubInputStream stream(src, ranges[i].bytes());
		unpack_image(dest, stream, offset, game);
	}
}

static void unpack_image(FlatWadAsset& dest, InputStream& src, s32 offset, Game game) {
	if(src.size() < 8) {
		return;
	}
	
	char header[8];
	src.seek(0);
	src.read_n((u8*) header, sizeof(header));
	if(memcmp(header, "WAD", 3) == 0) {
		std::vector<u8> bytes;
		std::vector<u8> compressed_bytes = src.read_multiple<u8>(0, src.size());
		decompress_wad(bytes, compressed_bytes);
		MemoryInputStream stream(bytes);
		unpack_image(dest, stream, offset, game);
		return;
	}
	
	if(memcmp(header, "2FIP", 4) == 0) {
		unpack_asset(dest.child<TextureAsset>(stringf("%04d_%04x_pif", offset, offset).c_str()), src, ByteRange{0, (s32) src.size()}, game);
		return;
	}
	
	s32 width = *(s32*) &header[0];
	s32 height = *(s32*) &header[4];
	bool common_size = is_common_texture_size(width) || is_common_texture_size(height);
	if(width > 0 && height > 0 && common_size && src.size() >= 0x10 + width * height * 4) {
		unpack_asset(dest.child<TextureAsset>(stringf("%04d_%04x_rgba", offset, offset).c_str()), src, ByteRange{0, (s32) src.size()}, game, FMT_TEXTURE_RGBA);
	}
}

static bool is_common_texture_size(s32 number) {
	for(s32 i = 32; i < 1024; i *= 2) {
		if(i == number) {
			return true;
		}
	}
	return false;
}

static void pack_flat_wad_asset(OutputStream& dest, FlatWadAsset& src, Game game) {
	s32 header_size = 0;
	src.for_each_logical_child([&](Asset& child) {
		header_size = std::max(header_size, (s32) parse_number(child.tag()) + 0x8);
	});
	
	dest.alloc_multiple<u8>(header_size);
	
	std::vector<u8> header(header_size);
	src.for_each_logical_child([&](Asset& child) {
		size_t offset = parse_number(child.tag());
		*(SectorRange*) &header[offset] = pack_asset_sa<SectorRange>(dest, child, game);
	});
	
	dest.seek(0);
	dest.write_v(header);
}
