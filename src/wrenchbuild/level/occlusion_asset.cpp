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
#include <engine/occlusion.h>
#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>
#include <wrenchbuild/tests.h>

static void unpack_occlusion(OcclusionAsset& dest, InputStream& src, BuildConfig config);
static bool test_occlusion(std::vector<u8>& src, AssetType type, BuildConfig config, const char* hint, AssetTestMode mode);

on_load(Occlusion, []() {
	OcclusionAsset::funcs.unpack_rac1 = wrap_unpacker_func<OcclusionAsset>(unpack_occlusion);
	OcclusionAsset::funcs.unpack_rac2 = wrap_unpacker_func<OcclusionAsset>(unpack_occlusion);
	OcclusionAsset::funcs.unpack_rac3 = wrap_unpacker_func<OcclusionAsset>(unpack_occlusion);
	OcclusionAsset::funcs.unpack_dl = wrap_unpacker_func<OcclusionAsset>(unpack_occlusion);
	
	OcclusionAsset::funcs.test_rac = new AssetTestFunc(test_occlusion);
	OcclusionAsset::funcs.test_gc = new AssetTestFunc(test_occlusion);
	OcclusionAsset::funcs.test_uya = new AssetTestFunc(test_occlusion);
	OcclusionAsset::funcs.test_dl = new AssetTestFunc(test_occlusion);
})

static void unpack_occlusion(OcclusionAsset& dest, InputStream& src, BuildConfig config) {
	std::vector<u8> bytes = src.read_multiple<u8>(0, src.size());
	std::vector<OcclusionOctant> grid = read_occlusion_grid(bytes);
	std::vector<OcclusionVector> vectors(grid.size());
	swap_occlusion(grid, vectors);
	
	std::vector<u8> octants;
	write_occlusion_octants(octants, vectors);
	dest.set_octants(dest.file().write_text_file("occlusion_octants.txt", (const char*) octants.data()));
}

static bool test_occlusion(std::vector<u8>& src, AssetType type, BuildConfig config, const char* hint, AssetTestMode mode) {
	std::vector<OcclusionOctant> grid = read_occlusion_grid(src);
	std::vector<u8> dest;
	write_occlusion_grid(dest, grid);
	strip_trailing_padding_from_lhs(src, dest);
	if(src.size() < 4 || dest.size() < 4) return false;
	s32 masks_offset_src = *(s32*) src.data();
	s32 masks_offset_dest = *(s32*) dest.data();
	if(masks_offset_src != masks_offset_dest) return false;
	return diff_buffers(src, dest, masks_offset_src, DIFF_REST_OF_BUFFER, mode == AssetTestMode::PRINT_DIFF_ON_FAIL);
}
