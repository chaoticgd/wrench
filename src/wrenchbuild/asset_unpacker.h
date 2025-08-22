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

#ifndef WRENCHBUILD_ASSET_UNPACKER_H
#define WRENCHBUILD_ASSET_UNPACKER_H

#include <assetmgr/asset.h>
#include <assetmgr/asset_types.h>
#include <engine/compression.h>

struct AssetUnpackerGlobals
{
	bool skip_globals = false;
	bool skip_levels = false;
	
	bool dump_wads = false;
	bool dump_binaries = false;
	bool dump_flat = false;
	
	bool quiet = false;
	
	InputStream* input_file = nullptr;
	s64 current_file_offset;
	s64 total_file_size;
	
	s32 current_level_id = -1;
};

extern AssetUnpackerGlobals g_asset_unpacker;

void unpack_asset_impl(
	Asset& dest,
	InputStream& src,
	const std::vector<u8>* header_src,
	BuildConfig config,
	const char* hint = FMT_NO_HINT);

template <typename ThisAsset, typename Range>
void unpack_asset(
	ThisAsset& dest,
	InputStream& src,
	Range range,
	BuildConfig config,
	const char* hint = FMT_NO_HINT)
{
	if(!range.empty()) {
		SubInputStream stream(src, range.bytes());
		unpack_asset_impl(dest, stream, nullptr, config, hint);
	}
}

template <typename ThisAsset, typename Range>
void unpack_compressed_asset(
	ThisAsset& dest,
	InputStream& src,
	Range range,
	BuildConfig config,
	const char* hint = FMT_NO_HINT)
{
	if(!range.empty()) {
		src.seek(range.bytes().offset);
		std::vector<u8> compressed_bytes = src.read_multiple<u8>(range.bytes().size);
		
		std::vector<u8> bytes;
		decompress_wad(bytes, compressed_bytes);
		
		MemoryInputStream stream(bytes);
		unpack_asset_impl(dest, stream, nullptr, config, hint);
	}
}

template <typename ChildAsset, typename Range>
void unpack_assets(
	CollectionAsset& dest,
	InputStream& src,
	const Range* ranges,
	s32 count,
	BuildConfig config,
	const char* hint = FMT_NO_HINT,
	bool switch_files = false)
{
	for(s32 i = 0; i < count; i++) {
		if(!ranges[i].empty()) {
			ChildAsset* asset;
			if(switch_files) {
				asset = &dest.foreign_child<ChildAsset>(i);
			} else {
				asset = &dest.child<ChildAsset>(i);
			}
			unpack_asset(*asset, src, ranges[i], config, hint);
		}
	}
}

template <typename ChildAsset, typename Range>
void unpack_compressed_assets(
	CollectionAsset& dest,
	InputStream& src,
	const Range* ranges,
	s32 count,
	BuildConfig config,
	const char* hint = FMT_NO_HINT,
	bool switch_files = false)
{
	for(s32 i = 0; i < count; i++) {
		if(!ranges[i].empty()) {
			ChildAsset* asset;
			if(switch_files) {
				asset = &dest.foreign_child<ChildAsset>(i);
			} else {
				asset = &dest.child<ChildAsset>(i);
			}
			unpack_compressed_asset(*asset, src, ranges[i], config, hint);
		}
	}
}

#endif
