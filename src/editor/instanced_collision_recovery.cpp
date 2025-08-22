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

#include "instanced_collision_recovery.h"

#include <engine/collision.h>

std::vector<ColLevel> load_instance_collision_data(
	BuildAsset& build, std::function<bool()>&& check_is_still_running)
{
	std::vector<ColLevel> levels;
	
	bool skip = false;
	
	build.get_levels().for_each_logical_child_of_type<LevelAsset>([&](LevelAsset& level) {
		if (skip) {
			return;
		}
		
		ColLevel& dest = levels.emplace_back();
		LevelWadAsset& level_wad = level.get_level().as<LevelWadAsset>();
		dest.asset = &level_wad;
		
		// Load collision meshes.
		CollectionAsset& chunks = level_wad.get_chunks();
		for (s32 i = 0; i < 3; i++) {
			if (chunks.has_child(i)) {
				ColChunk& chunk_dest = dest.chunks[i].emplace();
				ChunkAsset& chunk = chunks.get_child(i).as<ChunkAsset>();
				chunk_dest.asset = &chunk;
				MeshAsset& mesh_asset = chunk.get_collision().as<CollisionAsset>().get_mesh();
				std::string collada_xml = mesh_asset.src().read_text_file();
				verify(!collada_xml.empty(), "Empty collision mesh file.");
				chunk_dest.collision_scene = read_collada(&collada_xml[0]);
				chunk_dest.collision_mesh = chunk_dest.collision_scene.find_mesh(mesh_asset.name());
			}
		}
		
		// Load level settings, tie instances and shrub instances.
		InstancesAsset& gameplay = level_wad.get_gameplay().as<InstancesAsset>();
		std::string instances_str = gameplay.src().read_text_file();
		Instances instances = read_instances(instances_str);
		dest.instances[COL_TIE].reserve(instances.tie_instances.size());
		for (const TieInstance& inst : instances.tie_instances) {
			ColInstance& dest_inst = dest.instances[COL_TIE].emplace_back();
			dest_inst.o_class = inst.o_class();
			dest_inst.chunk = chunk_index_from_position(inst.transform().pos(), instances.level_settings);
			dest_inst.inverse_matrix = inst.transform().inverse_matrix();
			if (!check_is_still_running()) {
				skip = true;
				return;
			}
		}
		dest.instances[COL_SHRUB].reserve(instances.shrub_instances.size());
		for (const ShrubInstance& inst : instances.shrub_instances) {
			ColInstance& dest_inst = dest.instances[COL_SHRUB].emplace_back();
			dest_inst.o_class = inst.o_class();
			dest_inst.chunk = chunk_index_from_position(inst.transform().pos(), instances.level_settings);
			dest_inst.inverse_matrix = inst.transform().inverse_matrix();
			if (!check_is_still_running()) {
				skip = true;
				return;
			}
		}
	});
	
	return levels;
}

ColMappings generate_instance_collision_mappings(const std::vector<ColLevel>& levels)
{
	ColMappings mappings;
	
	for (size_t i = 0; i < levels.size(); i++) {
		const ColLevel& level = levels[i];
		
		for (size_t j = 0; j < level.instances[COL_TIE].size(); j++) {
			const ColInstance& inst = level.instances[COL_TIE][j];
			ColInstanceMapping& mapping = mappings.classes[COL_TIE][inst.o_class].emplace_back();
			mapping.level = i;
			mapping.instance = j;
		}
		
		for (size_t j = 0; j < level.instances[COL_SHRUB].size(); j++) {
			const ColInstance& inst = level.instances[COL_SHRUB][j];
			ColInstanceMapping& mapping = mappings.classes[COL_SHRUB][inst.o_class].emplace_back();
			mapping.level = i;
			mapping.instance = j;
		}
	}
	
	return mappings;
}

struct ColVec3i
{
	s32 x = INT32_MAX;
	s32 y = INT32_MAX;
	s32 z = INT32_MAX;
	
	friend auto operator<=>(const ColVec3i& lhs, const ColVec3i& rhs) = default;
};

struct ColFace
{
	ColVec3i verts[4];
	
	friend auto operator<=>(const ColFace& lhs, const ColFace& rhs) = default;
};

struct ColVal
{
	s32 mapping;
	s32 submesh;
	s32 face;
	s32 hits;
};

Opt<ColladaScene> build_instanced_collision(
	s32 type,
	s32 o_class,
	const ColParams& params,
	const ColMappings& mappings,
	const std::vector<ColLevel>& levels,
	std::function<bool()>&& check_is_still_running) {
	auto iter = mappings.classes[type].find(o_class);
	if (iter == mappings.classes[type].end()) {
		return std::nullopt;
	}
	const std::vector<ColInstanceMapping>& inst_mappings = iter->second;
	
	f32 quant_factor;
	if (params.merge_dist != 0.f) {
		quant_factor = 1.f / params.merge_dist;
	} else {
		quant_factor = 1.f;
	}
	
	std::map<ColFace, ColVal> state;
	
	glm::vec3 bb_min = params.bounding_box_origin - params.bounding_box_size * 0.5f;
	glm::vec3 bb_max = params.bounding_box_origin + params.bounding_box_size * 0.5f;
	
	// Count the number of occurences of each face.
	for (s32 m = 0; m < (s32) inst_mappings.size(); m++) {
		const ColInstanceMapping& mapping = inst_mappings[m];
		const ColLevel& level = levels[mapping.level];
		const ColInstance& inst = level.instances[type][mapping.instance];
		const Opt<ColChunk>& chunk = level.chunks[inst.chunk];
		if (!chunk.has_value()) continue;
		Mesh& mesh = *chunk->collision_mesh;
		for (s32 s = 0; s < (s32) mesh.submeshes.size(); s++) {
			SubMesh& submesh = mesh.submeshes[s];
			for (s32 f = 0; f < (s32) submesh.faces.size(); f++) {
				Face& face = submesh.faces[f];
				ColFace key;
				s32 faces[3] = {face.v0, face.v1, face.v2};
				bool accept = false;
				for (s32 v = 0; v < 3; v++) {
					glm::vec3 pos = inst.inverse_matrix * glm::vec4(mesh.vertices[faces[v]].pos, 1.f);
					key.verts[v].x = (s32) roundf(pos.x * quant_factor);
					key.verts[v].y = (s32) roundf(pos.y * quant_factor);
					key.verts[v].z = (s32) roundf(pos.z * quant_factor);
					accept |= pos.x > bb_min.x && pos.y > bb_min.y && pos.z > bb_min.z
						&& pos.x < bb_max.x && pos.y < bb_max.y && pos.z < bb_max.z;
				}
				if (face.v3 > -1) {
					glm::vec3 pos = inst.inverse_matrix * glm::vec4(mesh.vertices[face.v3].pos, 1.f);
					key.verts[3].x = (s32) roundf(pos.x * quant_factor);
					key.verts[3].y = (s32) roundf(pos.y * quant_factor);
					key.verts[3].z = (s32) roundf(pos.z * quant_factor);
					accept |= pos.x > bb_min.x && pos.y > bb_min.y && pos.z > bb_min.z
						&& pos.x < bb_max.x && pos.y < bb_max.y && pos.z < bb_max.z;
				}
				if (!params.reject_faces_outside_bb || accept) {
					auto iter = state.find(key);
					if (iter == state.end()) {
						ColVal val;
						val.mapping = m;
						val.submesh = s;
						val.face = f;
						val.hits = 1;
						state.emplace(key, val);
					} else {
						iter->second.hits++;
					}
				}
			}
		}
		if (!check_is_still_running()) {
			return std::nullopt;
		}
	}
	
	// Generate the scene.
	ColladaScene scene;
	Mesh& mesh = scene.meshes.emplace_back();
	mesh.name = "collision";
	mesh.flags |= MESH_HAS_QUADS;
	std::vector<s32> submesh_indices(256, -1);
	for (auto& [key, value] : state) {
		if (value.hits >= params.min_hits) {
			const ColInstanceMapping& mapping = inst_mappings[value.mapping];
			const ColLevel& level = levels[mapping.level];
			const ColInstance& inst = level.instances[type][mapping.instance];
			const Opt<ColChunk>& chunk = level.chunks[inst.chunk];
			if (!chunk.has_value()) continue;
			Mesh& mesh_src = *chunk->collision_mesh;
			const SubMesh& submesh_src = mesh_src.submeshes[value.submesh];
			const Face& face_src = submesh_src.faces[value.face];
			if (submesh_src.material < 0 || submesh_src.material > 255) {
				fprintf(stderr, "Invalid source texture index.\n");
				return std::nullopt;
			}
			if (submesh_indices.at(submesh_src.material) == -1) {
				submesh_indices.at(submesh_src.material) = (s32) mesh.submeshes.size();
				mesh.submeshes.emplace_back().material = submesh_src.material;
			}
			SubMesh& submesh_dest = mesh.submeshes[submesh_indices.at(submesh_src.material)];
			Face& face_dest = submesh_dest.faces.emplace_back();
			face_dest.v0 = (s32) mesh.vertices.size();
			mesh.vertices.emplace_back(mesh_src.vertices.at(face_src.v0));
			mesh.vertices.back().pos = inst.inverse_matrix * glm::vec4(mesh.vertices.back().pos, 1.f);
			face_dest.v1 = (s32) mesh.vertices.size();
			mesh.vertices.emplace_back(mesh_src.vertices.at(face_src.v1));
			mesh.vertices.back().pos = inst.inverse_matrix * glm::vec4(mesh.vertices.back().pos, 1.f);
			face_dest.v2 = (s32) mesh.vertices.size();
			mesh.vertices.emplace_back(mesh_src.vertices.at(face_src.v2));
			mesh.vertices.back().pos = inst.inverse_matrix * glm::vec4(mesh.vertices.back().pos, 1.f);
			if (face_src.v3 > -1) {
				face_dest.v3 = (s32) mesh.vertices.size();
				mesh.vertices.emplace_back(mesh_src.vertices.at(face_src.v3));
				mesh.vertices.back().pos = inst.inverse_matrix * glm::vec4(mesh.vertices.back().pos, 1.f);
			}
		}
	}
	
	// Deduplicate vertices.
	std::vector<size_t> mapping(mesh.vertices.size());
	for (size_t i = 0; i < mesh.vertices.size(); i++) {
		mapping[i] = i;
	}
	for (size_t i = 0; i < mesh.vertices.size(); i++) {
		for (size_t j = i + 1; j < mesh.vertices.size(); j++) {
			glm::vec3 d = mesh.vertices[i].pos - mesh.vertices[j].pos;
			if (d.x * d.x + d.y * d.y + d.z * d.z < params.merge_dist * params.merge_dist) {
				if (mapping[j] == j) {
					mapping[j] = i;
				}
			}
		}
	}
	for (SubMesh& submesh : mesh.submeshes) {
		for (Face& face : submesh.faces) {
			face.v0 = mapping.at(face.v0);
			face.v1 = mapping.at(face.v1);
			face.v2 = mapping.at(face.v2);
			if (face.v3 > -1) {
				face.v3 = mapping.at(face.v3);
			}
		}
	}
	
	scene.meshes[0] = deduplicate_faces(std::move(mesh));
	scene.materials = create_collision_materials();
	
	return scene;
}
