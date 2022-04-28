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

static void unpack_file_asset(FileAsset& dest, InputStream& src, Game game);
static void pack_file_asset(OutputStream& dest, std::vector<u8>* header_dest, fs::file_time_type* time_dest, FileAsset& asset);

on_load(File, []() {
	FileAsset::funcs.unpack_rac1 = wrap_unpacker_func<FileAsset>(unpack_file_asset);
	FileAsset::funcs.unpack_rac2 = wrap_unpacker_func<FileAsset>(unpack_file_asset);
	FileAsset::funcs.unpack_rac3 = wrap_unpacker_func<FileAsset>(unpack_file_asset);
	FileAsset::funcs.unpack_dl = wrap_unpacker_func<FileAsset>(unpack_file_asset);
	
	FileAsset::funcs.pack_rac1 = wrap_bin_packer_func<FileAsset>(pack_file_asset);
	FileAsset::funcs.pack_rac2 = wrap_bin_packer_func<FileAsset>(pack_file_asset);
	FileAsset::funcs.pack_rac3 = wrap_bin_packer_func<FileAsset>(pack_file_asset);
	FileAsset::funcs.pack_dl = wrap_bin_packer_func<FileAsset>(pack_file_asset);
})

static void unpack_file_asset(FileAsset& dest, InputStream& src, Game game) {
	auto [stream, ref] = dest.file().open_binary_file_for_writing(fs::path(dest.path()));
	verify(stream.get(), "Failed to open file '%s' for writing file asset '%s'.",
		dest.path().c_str(),
		asset_reference_to_string(dest.reference()).c_str());
	src.seek(0);
	Stream::copy(*stream, src, src.size());
	dest.set_src(ref);
}

static void pack_file_asset(OutputStream& dest, std::vector<u8>* header_dest, fs::file_time_type* time_dest, FileAsset& asset) {
	if(g_asset_packer_dry_run) {
		return;
	}
	
	FileReference ref = asset.src();
	auto src = asset.file().open_binary_file_for_reading(asset.src(), time_dest);
	verify(src.get(), "Failed to open file '%s' for reading.", ref.path.string().c_str());
	Stream::copy(dest, *src, src->size());
}
