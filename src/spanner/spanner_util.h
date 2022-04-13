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

#ifndef SPANNER_UTIL_H
#define SPANNER_UTIL_H

#include <assetmgr/asset.h>
#include <assetmgr/asset_types.h>
#include <engine/compression.h>

template <typename Range>
void unpack_binary(BinaryAsset& dest, InputStream& src, Range range, fs::path path, bool compressed = false) {
	assert(range.bytes().size < 1024 * 1024 * 1024);
	if(!range.empty()) {
		src.seek(range.bytes().offset);
		std::vector<u8> bytes = src.read_multiple<u8>(range.bytes().size);
		if(compressed) {
			std::vector<u8> compressed_bytes = std::move(bytes);
			bytes = std::vector<u8>();
			decompress_wad(bytes, compressed_bytes);
		}
		if(bytes.size() > 0) {
			dest.set_src(dest.file().write_binary_file(path, bytes));
		}
	}
}

template <typename Range>
void unpack_compressed_binary(BinaryAsset& dest, InputStream& src, Range range, fs::path path) {
	unpack_binary(dest, src, range, path, true);
}

template <typename Range>
void unpack_binaries_impl(CollectionAsset& dest, InputStream& src, Range* ranges, s32 count, bool compressed = false, const char* extension = ".bin") {
	for(s32 i = 0; i < count; i++) {
		std::string name = std::to_string(i);
		unpack_binary(dest.child<BinaryAsset>(name.c_str()), src, ranges[i], name + extension, compressed);
	}
}

template <typename Range>
void unpack_binaries(CollectionAsset& dest, InputStream& src, Range* ranges, s32 count, const char* extension = ".bin") {
	unpack_binaries_impl(dest, src, ranges, count, false, extension);
}

template <typename Range>
void unpack_compressed_binaries(CollectionAsset& dest, InputStream& src, Range* ranges, s32 count, const char* extension = ".bin") {
	unpack_binaries_impl(dest, src, ranges, count, true, extension);
}

template <typename Header>
std::pair<std::unique_ptr<InputStream>, Header> open_wad_file(BinaryAsset& asset) {
	std::unique_ptr<InputStream> file = asset.file().open_binary_file_for_reading(asset.src());
	Header header = file->read<Header>(0);
	return {std::move(file), header};
}

#endif
