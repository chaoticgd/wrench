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

#include <pakrac/asset_unpacker.h>
#include <pakrac/asset_packer.h>

static void unpack_binary_asset(Asset& dest, InputStream& src, Game game, s32 hint, s64 header_offset);
static void pack_binary_asset(OutputStream& dest, std::vector<u8>* header_dest, fs::file_time_type* time_dest, BinaryAsset& src);

on_load(Binary, []() {
	BinaryAsset::funcs.unpack_rac1 = new AssetUnpackerFunc(unpack_binary_asset);
	BinaryAsset::funcs.unpack_rac2 = new AssetUnpackerFunc(unpack_binary_asset);
	BinaryAsset::funcs.unpack_rac3 = new AssetUnpackerFunc(unpack_binary_asset);
	BinaryAsset::funcs.unpack_dl = new AssetUnpackerFunc(unpack_binary_asset);
	
	BinaryAsset::funcs.pack_rac1 = wrap_bin_packer_func<BinaryAsset>(pack_binary_asset);
	BinaryAsset::funcs.pack_rac2 = wrap_bin_packer_func<BinaryAsset>(pack_binary_asset);
	BinaryAsset::funcs.pack_rac3 = wrap_bin_packer_func<BinaryAsset>(pack_binary_asset);
	BinaryAsset::funcs.pack_dl = wrap_bin_packer_func<BinaryAsset>(pack_binary_asset);
})

static void unpack_binary_asset(Asset& dest, InputStream& src, Game game, s32 hint, s64 header_offset) {
	BinaryAsset& binary = dest.as<BinaryAsset>();
	std::string file_name = binary.tag() + (hint == FMT_BINARY_WAD ? ".wad" : ".bin");
	auto [stream, ref] = binary.file().open_binary_file_for_writing(file_name);
	verify(stream.get(), "Failed to open file '%s' for writing binary asset '%s'.",
		file_name.c_str(),
		asset_reference_to_string(binary.reference()).c_str());
	src.seek(0);
	Stream::copy(*stream, src, src.size());
	binary.set_src(ref);
}

static void pack_binary_asset(OutputStream& dest, std::vector<u8>* header_dest, fs::file_time_type* time_dest, BinaryAsset& src) {
	if(g_asset_packer_dry_run) {
		return;
	}
	
	auto stream = src.file().open_binary_file_for_reading(src.src(), time_dest);
	if(header_dest) {
		s32 header_size = stream->read<s32>();
		assert(header_size == header_dest->size());
		s64 padded_header_size = Sector32::size_from_bytes(header_size).bytes();
		
		// Extract the header.
		assert(padded_header_size != 0);
		header_dest->resize(padded_header_size);
		*(s32*) header_dest->data() = header_size;
		stream->read_n(header_dest->data() + 4, padded_header_size - 4);
		
		// Write the header.
		dest.write_n(header_dest->data(), padded_header_size);
		
		// The calling code needs the unpadded header.
		header_dest->resize(header_size);
		
		assert(dest.tell() % SECTOR_SIZE == 0);
		
		// Copy the rest of the file.
		Stream::copy(dest, *stream, stream->size() - padded_header_size);
	} else {
		Stream::copy(dest, *stream, stream->size());
	}
}
