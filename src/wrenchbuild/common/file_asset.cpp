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

static void unpack_file_asset(FileAsset& dest, InputStream& src, BuildConfig config);
static void pack_file_asset(
	OutputStream& dest,
	std::vector<u8>* header_dest,
	fs::file_time_type* time_dest,
	const FileAsset& src);

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

static void unpack_file_asset(FileAsset& dest, InputStream& src, BuildConfig config)
{
	auto [stream, ref] = dest.file().open_binary_file_for_writing(fs::path(dest.path()));
	verify(stream.get(), "Failed to open file '%s' for writing file asset '%s'.",
		dest.path().c_str(),
		dest.absolute_link().to_string().c_str());
	src.seek(0);
	Stream::copy(*stream, src, src.size());
	dest.set_src(ref);
}

static void pack_file_asset(
	OutputStream& dest,
	std::vector<u8>* header_dest,
	fs::file_time_type* time_dest,
	const FileAsset& src)
{
	if (g_asset_packer_dry_run) {
		return;
	}
	
	FileReference ref = src.src();
	auto stream = ref.open_binary_file_for_reading(time_dest);
	verify(stream.get(), "Failed to open file '%s' for reading.", ref.path.string().c_str());
	Stream::copy(dest, *stream, stream->size());
}
