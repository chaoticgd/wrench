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

#ifndef WRENCH_BUILD_ASSET_PACKER_H
#define WRENCH_BUILD_ASSET_PACKER_H

#include <assetmgr/asset.h>
#include <assetmgr/asset_types.h>
#include <engine/compression.h>

enum AssetFormatHint {
	FMT_NO_HINT = 0,
	FMT_TEXTURE_PIF_IDTEX8,
	FMT_TEXTURE_RGBA
};

// Packs asset into a binary and writes it out to dest, using hint to determine
// details of the expected output format if necessary.
void pack_asset_impl(OutputStream& dest, std::vector<u8>* header_dest, fs::file_time_type* time_dest, Asset& asset, Game game, u32 hint = FMT_NO_HINT);

template <typename Range>
Range pack_asset(OutputStream& dest, Asset& src, Game game, s64 base, AssetFormatHint hint = FMT_NO_HINT) {
	if(src.type() == BinaryAsset::ASSET_TYPE && !static_cast<BinaryAsset&>(src).has_src()) {
		return Range::from_bytes(0, 0);
	}
	s64 begin = dest.tell();
	pack_asset_impl(dest, nullptr, nullptr, src, game, hint);
	s64 end = dest.tell();
	return Range::from_bytes(begin - base, end - begin);
}

template <typename Range>
Range pack_asset_aligned(OutputStream& dest, Asset& asset, Game game, s64 base, s64 alignment, AssetFormatHint hint = FMT_NO_HINT) {
	dest.pad(alignment, 0);
	return pack_asset<Range>(dest, asset, game, base, hint);
}

// Sector aligned version.
template <typename Range>
Range pack_asset_sa(OutputStream& dest, Asset& asset, Game game, s64 base, AssetFormatHint hint = FMT_NO_HINT) {
	dest.pad(SECTOR_SIZE, 0);
	return pack_asset<Range>(dest, asset, game, base, hint);
}

template <typename Range>
void pack_assets_sa(OutputStream& dest, Range* ranges_dest, s32 count, CollectionAsset& src, Game game, s64 base, AssetFormatHint hint = FMT_NO_HINT) {
	for(size_t i = 0; i < count; i++) {
		if(src.has_child(i)) {
			ranges_dest[i] = pack_asset_sa<Range>(dest, src.get_child(i), game, base, hint);
		}
	}
}

template <typename Range>
Range pack_compressed_asset(OutputStream& dest, Asset& src, Game game, s64 base, AssetFormatHint hint = FMT_NO_HINT) {
	std::vector<u8> bytes;
	MemoryOutputStream stream(bytes);
	pack_asset<Range>(stream, src, game, base, hint);
	std::vector<u8> compressed_bytes;
	compress_wad(compressed_bytes, bytes, 8);
	s64 begin = dest.tell();
	dest.write(compressed_bytes.data(), compressed_bytes.size());
	s64 end = dest.tell();
	return Range::from_bytes(begin - base, end - begin);
}

template <typename Range>
Range pack_compressed_asset_aligned(OutputStream& dest, Asset& asset, Game game, s64 base, s64 alignment, AssetFormatHint hint = FMT_NO_HINT) {
	dest.pad(alignment, 0);
	return pack_compressed_asset<Range>(dest, asset, game, base, hint);
}

template <typename Range>
void pack_compressed_assets_aligned(OutputStream& dest, Range* ranges_dest, s32 count, CollectionAsset& src, Game game, s64 base, s64 alignment, AssetFormatHint hint = FMT_NO_HINT) {
	for(size_t i = 0; i < count; i++) {
		if(src.has_child(i)) {
			ranges_dest[i] = pack_compressed_asset_aligned<Range>(dest, src.get_child(i), game, base, alignment, hint);
		}
	}
}

template <typename Range>
Range pack_compressed_asset_sa(OutputStream& dest, Asset& asset, Game game, s64 base, AssetFormatHint hint = FMT_NO_HINT) {
	dest.pad(SECTOR_SIZE, 0);
	return pack_compressed_asset<Range>(dest, asset, game, base, hint);
}

template <typename Range>
void pack_compressed_assets_sa(OutputStream& dest, Range* ranges_dest, s32 count, CollectionAsset& src, Game game, s64 base, AssetFormatHint hint = FMT_NO_HINT) {
	for(size_t i = 0; i < count; i++) {
		if(src.has_child(i)) {
			ranges_dest[i] = pack_compressed_asset_sa<Range>(dest, src.get_child(i), game, base, hint);
		}
	}
}

#endif
