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

#ifndef WRENCH_BUILD_ASSET_UNPACKER_H
#define WRENCH_BUILD_ASSET_UNPACKER_H

#include <assetmgr/asset.h>
#include <assetmgr/asset_types.h>
#include <engine/compression.h>

void unpack_asset_impl(Asset& dest, InputStream& src, Game game, AssetFormatHint hint = FMT_NO_HINT);

template <typename ThisAsset, typename Range>
void unpack_asset(ThisAsset& dest, InputStream& src, Range range, Game game, AssetFormatHint hint = FMT_NO_HINT) {
	if(!range.empty()) {
		SubInputStream stream(src, range.bytes());
		unpack_asset_impl(dest, stream, game, hint);
	}
}

template <typename ThisAsset, typename Range>
void unpack_compressed_asset(ThisAsset& dest, InputStream& src, Range range, Game game, AssetFormatHint hint = FMT_NO_HINT) {
	if(!range.empty()) {
		src.seek(range.bytes().offset);
		std::vector<u8> compressed_bytes = src.read_multiple<u8>(range.bytes().size);
		
		std::vector<u8> bytes;
		decompress_wad(bytes, compressed_bytes);
		
		MemoryInputStream stream(bytes);
		unpack_asset_impl(dest, stream, game, hint);
	}
}

template <typename ChildAsset, typename Range>
void unpack_assets(CollectionAsset& dest, InputStream& src, Range* ranges, s32 count, Game game, AssetFormatHint hint = FMT_NO_HINT) {
	for(s32 i = 0; i < count; i++) {
		if(!ranges[i].empty()) {
			std::string name = std::to_string(i);
			unpack_asset(dest.child<ChildAsset>(name.c_str()), src, ranges[i], game, hint);
		}
	}
}

template <typename ChildAsset, typename Range>
void unpack_compressed_assets(CollectionAsset& dest, InputStream& src, Range* ranges, s32 count, Game game, AssetFormatHint hint = FMT_NO_HINT) {
	for(s32 i = 0; i < count; i++) {
		if(!ranges[i].empty()) {
			std::string name = std::to_string(i);
			unpack_compressed_asset(dest.child<ChildAsset>(name.c_str()), src, ranges[i], game, hint);
		}
	}
}


#endif
