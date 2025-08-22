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

#ifndef WRENCHBUILD_ASSET_PACKER_H
#define WRENCHBUILD_ASSET_PACKER_H

#include <assetmgr/asset.h>
#include <assetmgr/asset_types.h>
#include <engine/compression.h>

extern s32 g_asset_packer_max_assets_processed;
extern s32 g_asset_packer_num_assets_processed;
extern bool g_asset_packer_dry_run;
extern bool g_asset_packer_quiet;
extern s32 g_asset_packer_current_level_id;

// Packs asset into a binary and writes it out to dest, using hint to determine
// details of the expected output format if necessary.
void pack_asset_impl(OutputStream& dest, std::vector<u8>* header_dest, fs::file_time_type* time_dest, const Asset& src, BuildConfig config, const char* hint = FMT_NO_HINT);

template <typename Range>
Range pack_asset(
	OutputStream& dest,
	const Asset& src,
	BuildConfig config,
	s64 alignment,
	const char* hint = FMT_NO_HINT,
	Range* empty_range = nullptr)
{
	if(src.logical_type() == BinaryAsset::ASSET_TYPE && !(src.as<BinaryAsset>().has_src())) {
		return empty_range ? *empty_range : Range::from_bytes(0, 0);
	}
	dest.seek(dest.size());
	dest.pad(alignment, 0);
	s64 begin = dest.tell();
	pack_asset_impl(dest, nullptr, nullptr, src, config, hint);
	s64 end = dest.tell();
	return Range::from_bytes(begin, end - begin);
}

template <typename Range>
void pack_assets(
	OutputStream& dest,
	Range* ranges_dest,
	s32 count,
	const CollectionAsset& src,
	BuildConfig config,
	s32 alignment,
	const char* hint = FMT_NO_HINT,
	Range* empty_range = nullptr)
{
	for(size_t i = 0; i < count; i++) {
		if(src.has_child(i)) {
			ranges_dest[i] = pack_asset<Range>(dest, src.get_child(i), config, alignment, hint, empty_range);
		} else {
			ranges_dest[i] = empty_range ? *empty_range : Range::from_bytes(0, 0);
		}
	}
}

// Sector aligned version.
template <typename Range>
Range pack_asset_sa(
	OutputStream& dest, const Asset& asset, BuildConfig config, const char* hint = FMT_NO_HINT)
{
	return pack_asset<Range>(dest, asset, config, SECTOR_SIZE, hint);
}

template <typename Range>
void pack_assets_sa(
	OutputStream& dest,
	Range* ranges_dest,
	s32 count,
	const CollectionAsset& src,
	BuildConfig config,
	const char* hint = FMT_NO_HINT)
{
	for(size_t i = 0; i < count; i++) {
		if(src.has_child(i)) {
			ranges_dest[i] = pack_asset_sa<Range>(dest, src.get_child(i), config, hint);
		}
	}
}

template <typename Range>
Range pack_compressed_asset(
	OutputStream& dest,
	const Asset& src,
	BuildConfig config,
	s64 alignment,
	const char* muffin,
	const char* hint = FMT_NO_HINT)
{
	dest.seek(dest.size());
	dest.pad(alignment, 0);
	std::vector<u8> bytes;
	MemoryOutputStream stream(bytes);
	pack_asset<Range>(stream, src, config, 0x10, hint);
	std::vector<u8> compressed_bytes;
	compress_wad(compressed_bytes, bytes, muffin, 8);
	s64 begin = dest.tell();
	dest.write_n(compressed_bytes.data(), compressed_bytes.size());
	s64 end = dest.tell();
	return Range::from_bytes(begin, end - begin);
}

template <typename Range>
void pack_compressed_assets(
	OutputStream& dest,
	Range* ranges_dest,
	s32 count,
	const CollectionAsset& src,
	BuildConfig config,
	s64 alignment,
	const char* muffin,
	const char* hint = FMT_NO_HINT)
{
	for(size_t i = 0; i < count; i++) {
		if(src.has_child(i)) {
			ranges_dest[i] = pack_compressed_asset<Range>(dest, src.get_child(i), config, alignment, muffin, hint);
		}
	}
}

template <typename Range>
Range pack_compressed_asset_sa(
	OutputStream& dest,
	const Asset& src,
	BuildConfig config,
	const char* muffin,
	const char* hint = FMT_NO_HINT)
{
	return pack_compressed_asset<Range>(dest, src, config, SECTOR_SIZE, muffin, hint);
}

template <typename Range>
void pack_compressed_assets_sa(
	OutputStream& dest,
	Range* ranges_dest,
	s32 count,
	const CollectionAsset& src,
	BuildConfig config,
	const char* muffin,
	const char* hint = FMT_NO_HINT)
{
	for(size_t i = 0; i < count; i++) {
		if(src.has_child(i)) {
			ranges_dest[i] = pack_compressed_asset_sa<Range>(dest, src.get_child(i), config, muffin, hint);
		}
	}
}

#endif
