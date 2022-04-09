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

#ifndef SPANNER_ASSET_PACKER_H
#define SPANNER_ASSET_PACKER_H

#include <assetmgr/asset.h>
#include <assetmgr/asset_types.h>
#include <engine/compression.h>

enum AssetFormatHint {
	FMT_NO_HINT,
	FMT_TEXTURE_PIF_IDTEX8,
	FMT_TEXTURE_RGBA
};

// Packs asset into a binary and writes it out to dest, using hint to determine
// details of the expected output format if necessary.
void pack_asset_impl(OutputStream& dest, Asset& asset, Game game, AssetFormatHint hint = FMT_NO_HINT);

template <typename Range>
Range pack_asset(OutputStream& dest, Asset& asset, Game game, s64 base, AssetFormatHint hint = FMT_NO_HINT) {
	if(asset.type() == BinaryAsset::ASSET_TYPE && !static_cast<BinaryAsset&>(asset).has_src()) {
		return Range::from_bytes(0, 0);
	}
	s64 begin = dest.tell();
	pack_asset_impl(dest, asset, game, hint);
	s64 end = dest.tell();
	return Range::from_bytes(begin - base, end - begin);
}

template <typename Range>
Range pack_asset_aligned(OutputStream& dest, Asset* asset, Game game, s64 base, s64 alignment, AssetFormatHint hint = FMT_NO_HINT) {
	dest.pad(alignment, 0);
	return pack_asset<Range>(dest, *asset, game, base, hint);
}

// Sector aligned version.
template <typename Range>
Range pack_asset_sa(OutputStream& dest, Asset* asset, Game game, s64 base, AssetFormatHint hint = FMT_NO_HINT) {
	dest.pad(SECTOR_SIZE, 0);
	return pack_asset<Range>(dest, *asset, game, base, hint);
}

template <typename Range>
void pack_assets_sa(OutputStream& dest, Range* ranges_dest, s32 count, std::vector<Asset*> assets, Game game, s64 base, const char* name, AssetFormatHint hint = FMT_NO_HINT) {
	verify(assets.size() <= count, "Too many assets in %s list.");
	for(size_t i = 0; i < assets.size(); i++) {
		ranges_dest[i] = pack_asset_sa<Range>(dest, assets[i], game, base, hint);
	}
}

template <typename Range>
Range compress_asset(OutputStream& dest, Asset& asset, Game game, s64 base, AssetFormatHint hint = FMT_NO_HINT) {
	std::vector<u8> bytes;
	MemoryOutputStream stream(bytes);
	pack_asset<Range>(stream, asset, game, base, hint);
	std::vector<u8> compressed_bytes;
	compress_wad(compressed_bytes, bytes, 8);
	s64 begin = dest.tell();
	dest.write(compressed_bytes.data(), compressed_bytes.size());
	s64 end = dest.tell();
	return Range::from_bytes(begin - base, end - begin);
}

template <typename Range>
Range compress_asset_aligned(OutputStream& dest, Asset& asset, Game game, s64 base, s64 alignment, AssetFormatHint hint = FMT_NO_HINT) {
	dest.pad(alignment, 0);
	return compress_asset<Range>(dest, asset, game, base, hint);
}

template <typename Range>
void compress_assets_aligned(OutputStream& dest, Range* ranges_dest, s32 count, std::vector<Asset*> assets, Game game, s64 base, const char* name, s64 alignment, AssetFormatHint hint = FMT_NO_HINT) {
	verify(assets.size() <= count, "Too many assets in %s list.");
	for(size_t i = 0; i < assets.size(); i++) {
		ranges_dest[i] = compress_asset_aligned<Range>(dest, *assets[i], game, base, alignment, hint);
	}
}

template <typename Range>
Range compress_asset_sa(OutputStream& dest, Asset& asset, Game game, s64 base, AssetFormatHint hint = FMT_NO_HINT) {
	dest.pad(SECTOR_SIZE, 0);
	return compress_asset<Range>(dest, asset, game, base, hint);
}

template <typename Range>
void compress_assets_sa(OutputStream& dest, Range* ranges_dest, s32 count, std::vector<Asset*> assets, Game game, s64 base, const char* name, AssetFormatHint hint = FMT_NO_HINT) {
	verify(assets.size() <= count, "Too many assets in %s list.");
	for(size_t i = 0; i < assets.size(); i++) {
		ranges_dest[i] = compress_asset_sa<Range>(dest, *assets[i], game, base, hint);
	}
}

#endif
