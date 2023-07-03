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

#include <assetmgr/asset_types.h>
#include <instancemgr/instances.h>

#define COL_INSTANCE_TYPE_COUNT 2
#define COL_TIE 0
#define COL_SHRUB 1

struct ColChunk {
	ChunkAsset* asset;
	ColladaScene collision_scene;
	Mesh* collision_mesh;
};

struct ColInstance {
	s32 o_class;
	s32 chunk;
	glm::mat4 inverse_matrix;
};

struct ColLevel {
	LevelWadAsset* asset;
	Opt<ColChunk> chunks[3];
	std::vector<ColInstance> instances[COL_INSTANCE_TYPE_COUNT];
};

struct ColInstanceMapping {
	size_t level;
	size_t instance;
};

struct ColMappings {
	std::map<s32, std::vector<ColInstanceMapping>> classes[COL_INSTANCE_TYPE_COUNT];
};

static std::vector<ColLevel> load_level_data(BuildAsset& build);
static ColMappings generate_instance_mappings(const std::vector<ColLevel>& levels);
static void build_instanced_collision(const ColMappings& mappings, const std::vector<ColLevel>& levels, const char* output_path);

void recover_instanced_collision(BuildAsset& build, const char* output_path) {
	printf("Loading collision data...\n");
	std::vector<ColLevel> levels = load_level_data(build);
	ColMappings mappings = generate_instance_mappings(levels);
	printf("Building instanced collision...\n");
	build_instanced_collision(mappings, levels, output_path);
}

static std::vector<ColLevel> load_level_data(BuildAsset& build) {
	std::vector<ColLevel> levels;
	
	build.get_levels().for_each_logical_child_of_type<LevelAsset>([&](LevelAsset& level) {
		ColLevel& dest = levels.emplace_back();
		LevelWadAsset& level_wad = level.get_level().as<LevelWadAsset>();
		dest.asset = &level_wad;
		
		// Load collision meshes.
		CollectionAsset& chunks = level_wad.get_chunks();
		for(s32 i = 0; i < 3; i++) {
			if(chunks.has_child(i)) {
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
		for(const TieInstance& inst : instances.tie_instances) {
			ColInstance& dest_inst = dest.instances[COL_TIE].emplace_back();
			dest_inst.o_class = inst.o_class();
			dest_inst.chunk = chunk_index_from_position(inst.transform().pos(), instances.level_settings);
			dest_inst.inverse_matrix = inst.transform().inverse_matrix();
		}
		dest.instances[COL_SHRUB].reserve(instances.shrub_instances.size());
		for(const ShrubInstance& inst : instances.shrub_instances) {
			ColInstance& dest_inst = dest.instances[COL_SHRUB].emplace_back();
			dest_inst.o_class = inst.o_class();
			dest_inst.chunk = chunk_index_from_position(inst.transform().pos(), instances.level_settings);
			dest_inst.inverse_matrix = inst.transform().inverse_matrix();
		}
	});
	
	return levels;
}

static ColMappings generate_instance_mappings(const std::vector<ColLevel>& levels) {
	ColMappings mappings;
	
	for(size_t i = 0; i < levels.size(); i++) {
		const ColLevel& level = levels[i];
		
		for(size_t j = 0; j < level.instances[COL_TIE].size(); j++) {
			const ColInstance& inst = level.instances[COL_TIE][j];
			ColInstanceMapping& mapping = mappings.classes[COL_TIE][inst.o_class].emplace_back();
			mapping.level = i;
			mapping.instance = j;
		}
		
		for(size_t j = 0; j < level.instances[COL_SHRUB].size(); j++) {
			const ColInstance& inst = level.instances[COL_SHRUB][j];
			ColInstanceMapping& mapping = mappings.classes[COL_SHRUB][inst.o_class].emplace_back();
			mapping.level = i;
			mapping.instance = j;
		}
	}
	
	return mappings;
}

struct ColVec3i {
	s32 x = INT32_MAX;
	s32 y = INT32_MAX;
	s32 z = INT32_MAX;
	
	friend auto operator<=>(const ColVec3i& lhs, const ColVec3i& rhs) = default;
};

struct ColFace {
	ColVec3i verts[4];
	
	friend auto operator<=>(const ColFace& lhs, const ColFace& rhs) = default;
};

struct ColVal {
	s32 mapping;
	s32 submesh;
	s32 face;
	s32 hits;
};

#define COL_QUANT_FACTOR 8.f

static void build_instanced_collision(const ColMappings& mappings, const std::vector<ColLevel>& levels, const char* output_path) {
	for(s32 t = 0; t < COL_INSTANCE_TYPE_COUNT; t++) {
		if(t==COL_SHRUB) continue;
		for(const auto& [o_class, mappings] : mappings.classes[t]) {
			std::map<ColFace, ColVal> state;
			if(o_class!=565)continue;
			for(s32 m = 0; m < (s32) mappings.size(); m++) {
				const ColInstanceMapping& mapping = mappings[m];
				const ColLevel& level = levels[mapping.level];
				const ColInstance& inst = level.instances[t][mapping.instance];
				const Opt<ColChunk>& chunk = level.chunks[inst.chunk];
				if(!chunk.has_value()) continue;
				Mesh& mesh = *chunk->collision_mesh;
				for(s32 s = 0; s < (s32) mesh.submeshes.size(); s++) {
					SubMesh& submesh = mesh.submeshes[s];
					for(s32 f = 0; f < (s32) submesh.faces.size(); f++) {
						Face& face = submesh.faces[f];
						ColFace key;
						s32 faces[3] = {face.v0, face.v1, face.v2};
						for(s32 v = 0; v < 3; v++) {
							glm::vec3 local_pos = inst.inverse_matrix * glm::vec4(mesh.vertices[faces[v]].pos, 1.f);
							key.verts[v].x = (s32) roundf(local_pos.x * COL_QUANT_FACTOR);
							key.verts[v].y = (s32) roundf(local_pos.y * COL_QUANT_FACTOR);
							key.verts[v].z = (s32) roundf(local_pos.z * COL_QUANT_FACTOR);
						}
						if(face.v3 > -1) {
							glm::vec3 local_pos = inst.inverse_matrix * glm::vec4(mesh.vertices[face.v3].pos, 1.f);
							key.verts[3].x = (s32) roundf(local_pos.x * COL_QUANT_FACTOR);
							key.verts[3].y = (s32) roundf(local_pos.y * COL_QUANT_FACTOR);
							key.verts[3].z = (s32) roundf(local_pos.z * COL_QUANT_FACTOR);
						}
						auto iter = state.find(key);
						if(iter == state.end()) {
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
			
			s32 accepted = 0, discarded = 0;
			for(auto& [key, value] : state) {
				if(value.hits > 2) {
					accepted++;
				} else {
					discarded++;
				}
			}
			printf("%d / %d\n", accepted, discarded);
			
			fs::path out_path = fs::path(stringf("%s/%s_%d.dae", output_path, t==0?"tie":"shrub", o_class));
			ColladaScene scene;
			ColladaMaterial& mat = scene.materials.emplace_back();
			mat.name = "temp";
			mat.surface = MaterialSurface(glm::vec4(1.f, 1.f, 0.f, 1.f));
			Mesh& mesh = scene.meshes.emplace_back();
			mesh.name = "temp";
			SubMesh& sm = mesh.submeshes.emplace_back();
			sm.material = 0;
			bool valid = false;
			for(auto& [key, value] : state) {
				if(value.hits > 2) {
					const ColInstanceMapping& mapping = mappings[value.mapping];
					const ColLevel& level = levels[mapping.level];
					const ColInstance& inst = level.instances[t][mapping.instance];
					const Opt<ColChunk>& chunk = level.chunks[inst.chunk];
					if(!chunk.has_value()) continue;
					Mesh& mesh_src = *chunk->collision_mesh;
					const SubMesh& sm_src = mesh_src.submeshes[value.submesh];
					const Face& face_src = sm_src.faces[value.face];
					Face& face_dest = sm.faces.emplace_back();
					face_dest.v0 = (s32) mesh.vertices.size();
					mesh.vertices.emplace_back(mesh_src.vertices.at(face_src.v0));
					mesh.vertices.back().pos = inst.inverse_matrix * glm::vec4(mesh.vertices.back().pos, 1.f);
					face_dest.v1 = (s32) mesh.vertices.size();
					mesh.vertices.emplace_back(mesh_src.vertices.at(face_src.v1));
					mesh.vertices.back().pos = inst.inverse_matrix * glm::vec4(mesh.vertices.back().pos, 1.f);
					face_dest.v2 = (s32) mesh.vertices.size();
					mesh.vertices.emplace_back(mesh_src.vertices.at(face_src.v2));
					mesh.vertices.back().pos = inst.inverse_matrix * glm::vec4(mesh.vertices.back().pos, 1.f);
					if(face_src.v3 > -1) {
						face_dest.v3 = (s32) mesh.vertices.size();
						mesh.vertices.emplace_back(mesh_src.vertices.at(face_src.v3));
						mesh.vertices.back().pos = inst.inverse_matrix * glm::vec4(mesh.vertices.back().pos, 1.f);
					}
					valid = true;
				}
			}
			if(valid) {
				std::vector<u8> collada_xml = write_collada(scene);
				write_file(out_path, collada_xml, true);
			}
		}
	}
}
