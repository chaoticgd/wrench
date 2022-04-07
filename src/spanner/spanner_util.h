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
Asset& unpack_binary(Asset& parent, InputStream& src, Range range, const char* child, fs::path path, bool compressed = false) {
	src.seek(range.bytes().offset);
	std::vector<u8> bytes = src.read_multiple<u8>(range.bytes().size);
	if(compressed) {
		std::vector<u8> compressed_bytes = std::move(bytes);
		bytes = std::vector<u8>();
		decompress_wad(bytes, compressed_bytes);
	}
	BinaryAsset& binary = parent.child<BinaryAsset>(child);
	if(bytes.size() > 0) {
		binary.set_src(parent.file().write_binary_file(path, bytes));
	}
	return binary;
}

template <typename Range>
Range pack_binary(OutputStream& dest, Asset* asset, s64 base) {
	BinaryAsset* binary = static_cast<BinaryAsset*>(asset);
	verify(binary, "Asset must be a binary.");
	if(!binary->has_src()) {
		return Range::from_bytes(0, 0);
	}
	FileReference ref = binary->src();
	auto stream = binary->file().open_binary_file_for_reading(ref);
	s64 begin = dest.tell();
	Stream::copy(dest, *stream, stream->size());
	s64 end = dest.tell();
	return Range::from_bytes(begin - base, end - begin);
}

template <typename Range>
Asset& unpack_compressed_binary(Asset& parent, InputStream& src, Range range, const char* child, fs::path path) {
	return unpack_binary(parent, src, range, child, path, true);
}

template <typename Range>
std::vector<Asset*> unpack_binaries_impl(Asset& parent, InputStream& src, Range* ranges, s32 count, const char* child, bool compressed = false, const char* extension = ".bin") {
	fs::path path = fs::path(child)/child;
	CollectionAsset& collection = parent.asset_file(path).child<CollectionAsset>(child);
	
	std::vector<Asset*> assets;
	for(s32 i = 0; i < count; i++) {
		std::string name = std::to_string(i);
		assets.emplace_back(&unpack_binary(collection, src, ranges[i], name.c_str(), name + extension, compressed));
	}
	
	return assets;
}

template <typename Range>
std::vector<Asset*> unpack_binaries(Asset& parent, InputStream& src, Range* ranges, s32 count, const char* child, const char* extension = ".bin") {
	return unpack_binaries_impl(parent, src, ranges, count, child, false, extension);
}

template <typename Range>
Range pack_binaries(OutputStream& dest, Range* ranges, s64 count, const std::vector<Asset*>& assets, s64 base) {
	verify(assets.size() <= count, "Too many assets in list.");
	for(size_t i = 0; i < assets.size(); i++) {
		ranges[i] = pack_binary<Range>(dest, assets[i], base);
	}
}

template <typename Range>
std::vector<Asset*> unpack_compressed_binaries(Asset& parent, InputStream& src, Range* ranges, s32 count, const char* child, const char* extension = ".bin") {
	return unpack_binaries_impl(parent, src, ranges, count, child, true, extension);
}

Asset& unpack_binary_from_memory(Asset& parent, Buffer src, ByteRange range, const char* child, const char* extension = ".bin");
Asset& unpack_compressed_binary_from_memory(Asset& parent, Buffer src, ByteRange range, const char* child, const char* extension = ".bin");
std::vector<Asset*> unpack_compressed_binaries_from_memory(Asset& parent, Buffer src, ByteRange* ranges, s32 count, const char* child);

template <typename Header>
std::pair<std::unique_ptr<InputStream>, Header> open_wad_file(BinaryAsset& asset) {
	std::unique_ptr<InputStream> file = asset.file().open_binary_file_for_reading(asset.src());
	s32 header_size = file->read<s32>(0);
	file->seek(0);
	Header header = file->read<Header>();
	return {std::move(file), header};
}

#endif
