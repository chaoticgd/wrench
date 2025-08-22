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

#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>

static void unpack_binary_asset(
	Asset& dest,
	InputStream& src,
	const std::vector<u8>* header_src,
	BuildConfig config,
	const char* hint);
static void pack_binary_asset(
	OutputStream& dest,
	std::vector<u8>* header_dest,
	fs::file_time_type* time_dest,
	const BinaryAsset& src);

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

static void unpack_binary_asset(
	Asset& dest,
	InputStream& src,
	const std::vector<u8>* header_src,
	BuildConfig config,
	const char* hint)
{
	BinaryAsset& binary = dest.as<BinaryAsset>();
	const char* type = next_hint(&hint);
	const char* extension = "bin";
	if (strcmp(type, "ext") == 0) {
		extension = next_hint(&hint);
	}
	std::string file_name = binary.tag() + "." + extension;
	auto [stream, ref] = binary.file().open_binary_file_for_writing(file_name);
	verify(stream.get(), "Failed to open file '%s' for writing while unpacking binary asset '%s'.",
		file_name.c_str(), binary.absolute_link().to_string().c_str());
	if (header_src && config.game() == Game::RAC) {
		s64 padded_header_size = Sector32::size_from_bytes(header_src->size()).bytes();
		stream->write_v(*header_src);
		stream->seek(padded_header_size);
		src.seek(padded_header_size);
		Stream::copy(*stream, src, src.size() - padded_header_size);
	} else {
		src.seek(0);
		Stream::copy(*stream, src, src.size());
	}
	binary.set_src(ref);
}

static void pack_binary_asset(
	OutputStream& dest,
	std::vector<u8>* header_dest,
	fs::file_time_type* time_dest,
	const BinaryAsset& src)
{
	if (g_asset_packer_dry_run) {
		return;
	}
	
	auto stream = src.src().open_binary_file_for_reading(time_dest);
	verify(stream.get(), "Failed to open '%s' for reading while packing binary asset '%s'.",
		src.src().path.string().c_str(), src.absolute_link().to_string().c_str());
	if (header_dest) {
		s32 header_size = stream->read<s32>();
		verify_fatal(header_size == header_dest->size());
		s64 padded_header_size = Sector32::size_from_bytes(header_size).bytes();
		
		// Extract the header.
		verify_fatal(padded_header_size >= 4);
		header_dest->resize(padded_header_size);
		*(s32*) header_dest->data() = header_size;
		stream->read_n(header_dest->data() + 4, padded_header_size - 4);
		
		// Write the header.
		dest.write_n(header_dest->data(), padded_header_size);
		
		// The calling code needs the unpadded header.
		header_dest->resize(header_size);
		
		verify_fatal(dest.tell() % SECTOR_SIZE == 0);
		
		// Copy the rest of the file.
		Stream::copy(dest, *stream, stream->size() - padded_header_size);
	} else {
		Stream::copy(dest, *stream, stream->size());
	}
}
