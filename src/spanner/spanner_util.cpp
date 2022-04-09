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

#include "spanner_util.h"

Asset& unpack_binary_from_memory(Asset& parent, Buffer src, ByteRange range, const char* child, const char* extension) {
	BinaryAsset& binary = parent.child<BinaryAsset>(child);
	binary.set_src(parent.file().write_binary_file(std::string(child) + extension, src.subbuf(range.offset, range.size)));
	return binary;
}

std::vector<Asset*> unpack_binaries_from_memory(Asset& parent, Buffer src, ByteRange* ranges, s32 count, const char* child) {
	fs::path path = fs::path(child)/child;
	CollectionAsset& collection = parent.asset_file(path).child<CollectionAsset>(child);
	
	std::vector<Asset*> assets;
	for(s32 i = 0; i < count; i++) {
		std::string name = std::to_string(i);
		assets.emplace_back(&unpack_binary_from_memory(collection, src, ranges[i], name.c_str()));
	}
	
	return assets;
}

Asset& unpack_compressed_binary_from_memory(Asset& parent, Buffer src, ByteRange range, const char* child, const char* extension) {
	std::vector<u8> bytes;
	Buffer compressed_bytes = src.subbuf(range.offset, range.size);
	decompress_wad(bytes, WadBuffer{compressed_bytes.lo, compressed_bytes.hi});
	
	BinaryAsset& binary = parent.child<BinaryAsset>(child);
	binary.set_src(parent.file().write_binary_file(std::string(child) + extension, bytes));
	return binary;
}

std::vector<Asset*> unpack_compressed_binaries_from_memory(Asset& parent, Buffer src, ByteRange* ranges, s32 count, const char* child) {
	fs::path path = fs::path(child)/child;
	CollectionAsset& collection = parent.asset_file(path).child<CollectionAsset>(child);
	
	std::vector<Asset*> assets;
	for(s32 i = 0; i < count; i++) {
		std::string name = std::to_string(i);
		assets.emplace_back(&unpack_compressed_binary_from_memory(collection, src, ranges[i], name.c_str()));
	}
	
	return assets;
}
