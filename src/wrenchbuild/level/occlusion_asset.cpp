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

#include "occlusion_asset.h"

#include <core/collada.h>
#include <engine/occlusion.h>
#include <engine/visibility.h>

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

ByteRange pack_occlusion(OutputStream& dest, const OcclusionAsset& asset, const std::vector<Mesh>& tfrags, BuildConfig config) {
	if(g_asset_packer_dry_run) {
		return {0, 0};
	}
	
	s64 ofs = dest.tell();
	
	std::string octants_txt = asset.file().read_text_file(asset.octants().path);
	std::vector<OcclusionVector> octants = read_occlusion_octants(octants_txt.c_str());
	
	// Plug in all the inputs the visibility algorithm needs.
	VisInput input;
	input.octant_size_x = 4;
	input.octant_size_y = 4;
	input.octant_size_z = 4;
	input.octants = octants;
	for(const Mesh& tfrag : tfrags) {
		VisInstance& instance = input.instances[VIS_TFRAG].emplace_back();
		instance.mesh = (s32) input.meshes.size();
		input.meshes.emplace_back(&tfrag);
	}
	
	// The interesting bit: Compute which objects are visible from each octant!
	VisOutput vis = compute_level_visibility(input);
	
	// Build the lookup tree and write out all the visibility masks.
	std::vector<u8> buffer;
	write_occlusion_grid(buffer, vis.octants);
	dest.write_v(buffer);
	
	s64 end_ofs = dest.tell();
	return {(s32) ofs, (s32) (end_ofs - ofs)};
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
