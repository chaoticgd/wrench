/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2023 chaoticgd

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
#include <engine/occlusion.h>

static void unpack_occlusion(OcclusionAsset& dest, InputStream& src, BuildConfig config);
static void pack_occlusion(OutputStream& dest, const OcclusionAsset& src, BuildConfig config);

on_load(Occlusion, []() {
	OcclusionAsset::funcs.unpack_rac1 = wrap_unpacker_func<OcclusionAsset>(unpack_occlusion);
	OcclusionAsset::funcs.unpack_rac2 = wrap_unpacker_func<OcclusionAsset>(unpack_occlusion);
	OcclusionAsset::funcs.unpack_rac3 = wrap_unpacker_func<OcclusionAsset>(unpack_occlusion);
	OcclusionAsset::funcs.unpack_dl = wrap_unpacker_func<OcclusionAsset>(unpack_occlusion);
	
	OcclusionAsset::funcs.pack_rac1 = wrap_packer_func<OcclusionAsset>(pack_occlusion);
	OcclusionAsset::funcs.pack_rac2 = wrap_packer_func<OcclusionAsset>(pack_occlusion);
	OcclusionAsset::funcs.pack_rac3 = wrap_packer_func<OcclusionAsset>(pack_occlusion);
	OcclusionAsset::funcs.pack_dl = wrap_packer_func<OcclusionAsset>(pack_occlusion);
})

static void unpack_occlusion(OcclusionAsset& dest, InputStream& src, BuildConfig config)
{
	dest.set_memory_budget(src.size());
	
	std::vector<u8> grid_buffer = src.read_multiple<u8>(0, src.size());
	std::vector<OcclusionOctant> grid = read_occlusion_grid(grid_buffer);
	std::vector<OcclusionVector> octants(grid.size());
	swap_occlusion(grid, octants);
	
	std::vector<u8> octants_buffer;
	write_occlusion_octants(octants_buffer, octants);
	std::string octants_file_name = "occlusion_octants.csv";
	dest.set_octants(dest.file().write_text_file(octants_file_name, (const char*) octants_buffer.data()));
	
	std::string grid_file_name = "occlusion_grid.bin";
	auto [stream, ref] = dest.file().open_binary_file_for_writing(grid_file_name);
	verify(stream.get(), "Failed to open file '%s' for writing while unpacking binary asset '%s'.",
		grid_file_name.c_str(), dest.absolute_link().to_string().c_str());
	src.seek(0);
	Stream::copy(*stream, src, src.size());
	dest.set_grid(ref);
}

static void pack_occlusion(OutputStream& dest, const OcclusionAsset& src, BuildConfig config)
{
	if (g_asset_packer_dry_run) {
		return;
	}
	
	auto stream = src.grid().open_binary_file_for_reading();
	verify(stream.get(), "Failed to open '%s' for reading while packing occlusion asset '%s'.",
		src.grid().path.string().c_str(), src.absolute_link().to_string().c_str());
	Stream::copy(dest, *stream, stream->size());
}
