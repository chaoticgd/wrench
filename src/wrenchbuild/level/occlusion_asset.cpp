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

#include <core/collada.h>
#include <engine/occlusion_grid.h>
#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>

static void unpack_occlusion_asset(OcclusionAsset& dest, InputStream& src, BuildConfig config);

on_load(Occlusion, []() {
	OcclusionAsset::funcs.unpack_rac1 = wrap_unpacker_func<OcclusionAsset>(unpack_occlusion_asset);
	OcclusionAsset::funcs.unpack_rac2 = wrap_unpacker_func<OcclusionAsset>(unpack_occlusion_asset);
	OcclusionAsset::funcs.unpack_rac3 = wrap_unpacker_func<OcclusionAsset>(unpack_occlusion_asset);
	OcclusionAsset::funcs.unpack_dl = wrap_unpacker_func<OcclusionAsset>(unpack_occlusion_asset);
})

static void unpack_occlusion_asset(OcclusionAsset& dest, InputStream& src, BuildConfig config) {
	std::vector<u8> bytes = src.read_multiple<u8>(0, src.size());
	std::vector<OcclusionOctant> grid = read_occlusion_grid(bytes);
	std::vector<OcclusionVector> vectors(grid.size());
	swap_occlusion(grid, vectors);
	
	std::vector<u8> octants;
	write_occlusion_octants(octants, vectors);
	dest.set_octants(dest.file().write_text_file("occlusion_octants.txt", (const char*) octants.data()));
	
}
