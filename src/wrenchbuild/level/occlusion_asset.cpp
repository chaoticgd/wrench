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
static s32 chunk_index_from_position(const glm::vec3& point, const Gameplay& gameplay);
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
	dest.set_memory_budget(src.size());
	dest.set_octants(dest.file().write_text_file("occlusion_octants.txt", (const char*) octants.data()));
}

ByteRange pack_occlusion(OutputStream& dest, Gameplay& gameplay, const OcclusionAsset& asset, const std::vector<LevelChunk>& chunks, const ClassesHigh& high_classes, BuildConfig config) {
	if(g_asset_packer_dry_run) {
		return {0, 0};
	}
	
	s64 ofs = dest.tell();
	
	std::string octants_txt = asset.file().read_text_file(asset.octants().path);
	std::vector<OcclusionVector> octants = read_occlusion_octants(octants_txt.c_str());
	
	for(OcclusionVector& octant : octants) {
		glm::vec3 point = {
			octant.x * 4.f,
			octant.y * 4.f,
			octant.z * 4.f
		};
		octant.chunk = chunk_index_from_position(point, gameplay);
	}
	
	// Plug in all the inputs the visibility algorithm needs.
	VisInput input;
	input.octant_size_x = 4;
	input.octant_size_y = 4;
	input.octant_size_z = 4;
	
	// Take samples at the corners of each octant.
	input.sample_points[0] = {0, 0, 0};
	input.sample_points[1] = {0, 0, 4};
	input.sample_points[2] = {0, 4, 0};
	input.sample_points[3] = {0, 4, 4};
	input.sample_points[4] = {4, 0, 0};
	input.sample_points[5] = {4, 0, 4};
	input.sample_points[6] = {4, 4, 0};
	input.sample_points[7] = {4, 4, 4};
	static_assert(VIS_MAX_SAMPLES_PER_OCTANT >= 8);
	
	input.octants = std::move(octants);
	
	for(s32 i = 0; i < (s32) chunks.size(); i++) {
		for(const Mesh& tfrag_mesh : chunks[i].tfrag_meshes) {
			VisInstance& vis_instance = input.instances[VIS_TFRAG].emplace_back();
			vis_instance.mesh = (s32) input.meshes.size();
			vis_instance.chunk = i;
			vis_instance.matrix = glm::mat4(1.f);
			input.meshes.emplace_back(&tfrag_mesh);
		}
	}
	
	std::map<s32, s32> tie_class_to_index;
	for(const auto& [id, tie_class] : high_classes.tie_classes) {
		verify_fatal(tie_class.mesh);
		tie_class_to_index[id] = (s32) input.meshes.size();
		input.meshes.emplace_back(tie_class.mesh);
	}
	for(const TieInstance& instance : opt_iterator(gameplay.tie_instances)) {
		auto index = tie_class_to_index.find(instance.o_class);
		verify(index != tie_class_to_index.end(), "Cannot find tie model!");
		VisInstance& vis_instance = input.instances[VIS_TIE].emplace_back();
		vis_instance.mesh = index->second;
		vis_instance.matrix = instance.transform().matrix();
		vis_instance.matrix[3][3] = 1.f;
		vis_instance.chunk = chunk_index_from_position(glm::vec3(vis_instance.matrix[3]), gameplay);
	}
	
	std::map<s32, s32> moby_class_to_index;
	for(const auto& [id, moby_class] : high_classes.moby_classes) {
		verify_fatal(moby_class.mesh);
		moby_class_to_index[id] = (s32) input.meshes.size();
		input.meshes.emplace_back(moby_class.mesh);
	}
	for(const MobyInstance& instance : opt_iterator(gameplay.moby_instances)) {
		if(!instance.occlusion) {
			auto index = moby_class_to_index.find(instance.o_class);
			if(index != moby_class_to_index.end()) {
				VisInstance& vis_instance = input.instances[VIS_MOBY].emplace_back();
				vis_instance.mesh = index->second;
				vis_instance.matrix = instance.transform().matrix();
				vis_instance.matrix[3][3] = 1.f;
				vis_instance.chunk = chunk_index_from_position(glm::vec3(vis_instance.matrix[3]), gameplay);
			}
		}
	}
	
	s32 memory_budget = -1;
	if(asset.has_memory_budget()) {
		memory_budget = asset.memory_budget();
	}
	
	s32 memory_budget_for_masks = -1;
	if(memory_budget) {
		memory_budget_for_masks = memory_budget - compute_occlusion_tree_size(input.octants);
	}
	
	// The interesting bit: Compute which objects are visible from each octant!
	VisOutput vis = compute_level_visibility(input, memory_budget_for_masks);
	
	// Build the lookup tree and write out all the visibility masks.
	std::vector<u8> buffer;
	write_occlusion_grid(buffer, vis.octants);
	dest.write_v(buffer);
	
	gameplay.occlusion = OcclusionMappings();
	for(s32 i = 0; i < (s32) vis.mappings[VIS_TFRAG].size(); i++) {
		OcclusionMapping& mapping = gameplay.occlusion->tfrag_mappings.emplace_back();
		mapping.bit_index = vis.mappings[VIS_TFRAG][i];
		mapping.occlusion_id = i;
	}
	
	for(s32 i = 0; i < (s32) vis.mappings[VIS_TIE].size(); i++) {
		OcclusionMapping& mapping = gameplay.occlusion->tie_mappings.emplace_back();
		TieInstance& instance = gameplay.tie_instances->at(i);
		mapping.bit_index = vis.mappings[VIS_TIE][i];
		mapping.occlusion_id = instance.uid;
		instance.occlusion_index = instance.uid;
	}
	
	s32 moby_instance_index = 0;
	for(s32 i = 0; i < (s32) vis.mappings[VIS_MOBY].size(); i++) {
		// Skip past moby instances for which we don't precompute occlusion.
		while(gameplay.moby_instances->at(moby_instance_index).occlusion
			|| moby_class_to_index.find(gameplay.moby_instances->at(moby_instance_index).o_class) == moby_class_to_index.end()) {
			moby_instance_index++;
		}
		
		OcclusionMapping& mapping = gameplay.occlusion->moby_mappings.emplace_back();
		mapping.bit_index = vis.mappings[VIS_MOBY][i];
		mapping.occlusion_id = gameplay.moby_instances->at(moby_instance_index).uid;
		
		moby_instance_index++;
	}
	
	s64 end_ofs = dest.tell();
	return {(s32) ofs, (s32) (end_ofs - ofs)};
}

static s32 chunk_index_from_position(const glm::vec3& point, const Gameplay& gameplay) {
	verify_fatal(gameplay.level_settings.has_value());
	const LevelSettings& level_settings = *gameplay.level_settings;
	if(!level_settings.chunk_planes.empty()) {
		glm::vec3 plane_1_point = level_settings.chunk_planes[0].point;
		glm::vec3 plane_1_normal = level_settings.chunk_planes[0].normal;
		if(glm::dot(plane_1_normal, point - plane_1_point) > 0.f) {
			return 1;
		}
		if(level_settings.chunk_planes.size() > 1) {
			glm::vec3 plane_2_point = level_settings.chunk_planes[1].point;
			glm::vec3 plane_2_normal = level_settings.chunk_planes[1].normal;
			if(glm::dot(plane_2_normal, point - plane_2_point) > 0.f) {
				return 2;
			}
		}
	}
	return 0;
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
