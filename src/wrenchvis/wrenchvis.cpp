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

#include <core/stdout_thread.h>
#include <assetmgr/asset_types.h>
#include <engine/tfrag_high.h>
#include <engine/occlusion.h>
#include <instancemgr/gameplay_convert.h>
#include <wrenchvis/visibility.h>

struct OcclChunk
{
	std::vector<Mesh> tfrags;
};

struct OcclLevel
{
	std::vector<OcclChunk> chunks;
	std::map<s32, Mesh> moby_classes;
	std::map<s32, Mesh> tie_classes;
	Instances instances;
};

packed_struct(OcclusionMapping,
	s32 bit_index;
	s32 occlusion_id;
)

struct OcclusionMappings
{
	std::vector<OcclusionMapping> tfrag_mappings;
	std::vector<OcclusionMapping> tie_mappings;
	std::vector<OcclusionMapping> moby_mappings;
};

packed_struct(OcclusionMappingsHeader,
	s32 tfrag_mapping_count;
	s32 tie_mapping_count;
	s32 moby_mapping_count;
	s32 pad = 0;
)

static std::vector<OcclChunk> load_chunks(const CollectionAsset& collection, Game game);
static std::map<s32, Mesh> load_moby_classes(const CollectionAsset& collection);
static std::map<s32, Mesh> load_tie_classes(const CollectionAsset& collection);
static Instances load_instances(const Asset& src, Game game);
static void generate_occlusion_data(const OcclusionAsset& asset, const OcclLevel& level);

int main(int argc, char** argv)
{
	if (argc != 4) {
		fprintf(stderr, "usage: %s <game path> <mod path> <asset link of LevelWad asset>\n", (argc > 0) ? argv[0] : "wrenchvis");
		return 1;
	}
	
	std::string game_path = argv[1];
	std::string mod_path = argv[2];
	
	AssetForest forest;
	AssetBank& game_bank = forest.mount<LooseAssetBank>(game_path, false);
	forest.mount<LooseAssetBank>(mod_path, true);
	
	verify(game_bank.game_info.type == AssetBankType::GAME,
		"The asset bank specified for the game is not a game asset bank.");
	
	AssetLink link(argv[3]);
	LevelWadAsset& level_wad = forest.lookup_asset(link, nullptr).as<LevelWadAsset>();
	OcclusionAsset& occlusion_asset = level_wad.get_occlusion();
	verify(occlusion_asset.bank().is_writeable(), "Occlusion asset is in an asset bank that's read only.");
	
	OcclLevel level;
	level.chunks = load_chunks(level_wad.get_chunks(), game_bank.game_info.game.game);
	level.moby_classes = load_moby_classes(level_wad.get_moby_classes());
	level.tie_classes = load_tie_classes(level_wad.get_tie_classes());
	level.instances = load_instances(level_wad.get_gameplay(), game_bank.game_info.game.game);
	
	start_stdout_flusher_thread();
	try {
		generate_occlusion_data(level_wad.get_occlusion(), level);
		stop_stdout_flusher_thread();
	} catch (RuntimeError& e) {
		e.print();
		stop_stdout_flusher_thread();
		return 1;
	} catch (...) {
		stop_stdout_flusher_thread();
		throw;
	}
}

static std::vector<OcclChunk> load_chunks(const CollectionAsset& collection, Game game)
{
	std::vector<OcclChunk> chunks(3);
	for (s32 i = 0; i < 3; i++) {
		if (collection.has_child(i)) {
			const ChunkAsset& chunk_asset = collection.get_child(i).as<ChunkAsset>();
			if (chunk_asset.has_tfrags()) {
				const Asset& core_asset = chunk_asset.get_tfrags().get_core();
				if (const BinaryAsset* binary_asset = core_asset.maybe_as<BinaryAsset>()) {
					std::unique_ptr<InputStream> stream = binary_asset->src().open_binary_file_for_reading();
					std::vector<u8> buffer = stream->read_multiple<u8>(0, stream->size());
					Tfrags tfrags = read_tfrags(buffer, game);
					ColladaScene scene = recover_tfrags(tfrags, TFRAG_SEPARATE_MESHES);
					OcclChunk& dest = chunks.emplace_back();
					dest.tfrags = std::move(scene.meshes);
				} else {
					verify_not_reached_fatal("Tfrags asset is of an invalid type.");
				}
			}
		}
	}
	return chunks;
}

static std::map<s32, Mesh> load_moby_classes(const CollectionAsset& collection)
{
	std::map<s32, Mesh> classes;
	collection.for_each_logical_child_of_type<MobyClassAsset>([&](const MobyClassAsset& child) {
		if (child.has_editor_mesh()) {
			const MeshAsset& editor_mesh = child.get_editor_mesh();
			std::string collada = editor_mesh.src().read_text_file();
			ColladaScene scene = read_collada((char*) collada.c_str());
			Mesh* mesh = scene.find_mesh(editor_mesh.name());
			verify(mesh, "Failed to find mesh '%s'.", editor_mesh.name().c_str());
			classes[child.id()] = std::move(*mesh);
		}
	});
	return classes;
}

static std::map<s32, Mesh> load_tie_classes(const CollectionAsset& collection)
{
	std::map<s32, Mesh> classes;
	collection.for_each_logical_child_of_type<TieClassAsset>([&](const TieClassAsset& child) {
		const MeshAsset& editor_mesh = child.get_editor_mesh();
		std::string collada = editor_mesh.src().read_text_file();
		ColladaScene scene = read_collada((char*) collada.c_str());
		Mesh* mesh = scene.find_mesh(editor_mesh.name());
		verify(mesh, "Failed to find mesh '%s'.", editor_mesh.name().c_str());
		classes[child.id()] = std::move(*mesh);
	});
	return classes;
}

static Instances load_instances(const Asset& src, Game game)
{
	std::vector<u8> gameplay_buffer;
	if (const InstancesAsset* asset = src.maybe_as<InstancesAsset>()) {
		std::string instances_wtf = asset->src().read_text_file();
		return read_instances(instances_wtf);
	} else if (const BinaryAsset* asset = src.maybe_as<BinaryAsset>()) {
		std::unique_ptr<InputStream> gameplay_stream = asset->src().open_binary_file_for_reading();
		std::vector<u8> buffer = gameplay_stream->read_multiple<u8>(gameplay_stream->size());
		Gameplay gameplay;
		read_gameplay(gameplay, buffer, game, *gameplay_block_descriptions_from_game(game));
		Instances instances;
		move_gameplay_to_instances(instances, nullptr, nullptr, nullptr, gameplay, game);
		return instances;
	}
	verify_not_reached("Instances asset is of an invalid type.");
}

static void generate_occlusion_data(const OcclusionAsset& asset, const OcclLevel& level)
{
	std::string octants_txt = asset.octants().read_text_file();
	std::vector<OcclusionVector> octants = read_occlusion_octants(octants_txt.c_str());
	
	for (OcclusionVector& octant : octants) {
		glm::vec3 point = {
			octant.x * 4.f,
			octant.y * 4.f,
			octant.z * 4.f
		};
		octant.chunk = chunk_index_from_position(point, level.instances.level_settings);
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
	
	for (s32 i = 0; i < (s32) level.chunks.size(); i++) {
		for (const Mesh& tfrag_mesh : level.chunks[i].tfrags) {
			VisInstance& vis_instance = input.instances[VIS_TFRAG].emplace_back();
			vis_instance.mesh = (s32) input.meshes.size();
			vis_instance.chunk = i;
			vis_instance.matrix = glm::mat4(1.f);
			input.meshes.emplace_back(&tfrag_mesh);
		}
	}
	
	std::map<s32, s32> tie_class_to_index;
	for (const auto& [id, mesh] : level.tie_classes) {
		tie_class_to_index[id] = (s32) input.meshes.size();
		input.meshes.emplace_back(&mesh);
	}
	for (const TieInstance& instance : level.instances.tie_instances) {
		auto index = tie_class_to_index.find(instance.o_class());
		verify(index != tie_class_to_index.end(), "Cannot find tie model!");
		VisInstance& vis_instance = input.instances[VIS_TIE].emplace_back();
		vis_instance.mesh = index->second;
		vis_instance.matrix = instance.transform().matrix();
		vis_instance.chunk = chunk_index_from_position(glm::vec3(vis_instance.matrix[3]), level.instances.level_settings);
	}
	
	std::map<s32, s32> moby_class_to_index;
	for (const auto& [id, mesh] : level.moby_classes) {
		moby_class_to_index[id] = (s32) input.meshes.size();
		input.meshes.emplace_back(&mesh);
	}
	for (const MobyInstance& instance : level.instances.moby_instances) {
		if (!instance.occlusion) {
			auto index = moby_class_to_index.find(instance.o_class());
			if (index != moby_class_to_index.end()) {
				VisInstance& vis_instance = input.instances[VIS_MOBY].emplace_back();
				vis_instance.mesh = index->second;
				vis_instance.matrix = instance.transform().matrix();
				vis_instance.chunk = chunk_index_from_position(glm::vec3(vis_instance.matrix[3]), level.instances.level_settings);
			}
		}
	}
	
	s32 memory_budget = -1;
	if (asset.has_memory_budget()) {
		memory_budget = asset.memory_budget();
	}
	
	s32 memory_budget_for_masks = -1;
	if (memory_budget) {
		memory_budget_for_masks = memory_budget - compute_occlusion_tree_size(input.octants);
	}
	
	// The interesting bit: Compute which objects are visible from each octant!
	VisOutput vis = compute_level_visibility(input, memory_budget_for_masks);
	
	// Open output files for writing.
	auto [grid_dest, grid_ref] = asset.file().open_binary_file_for_writing(asset.grid().path);
	auto [mappings_dest, mappings_ref] = asset.file().open_binary_file_for_writing(asset.mappings().path);
	
	// Build the lookup tree and write out all the visibility masks.
	std::vector<u8> buffer;
	write_occlusion_grid(buffer, vis.octants);
	grid_dest->write_v(buffer);
	
	OcclusionMappingsHeader mappings_header = {};
	mappings_header.tfrag_mapping_count = (s32) vis.mappings[VIS_TFRAG].size();
	mappings_header.tie_mapping_count = (s32) vis.mappings[VIS_TIE].size();
	mappings_header.moby_mapping_count = (s32) vis.mappings[VIS_MOBY].size();
	mappings_dest->write(mappings_header);
	
	for (s32 i = 0; i < (s32) vis.mappings[VIS_TFRAG].size(); i++) {
		OcclusionMapping mapping;
		mapping.bit_index = vis.mappings[VIS_TFRAG][i];
		mapping.occlusion_id = i;
		mappings_dest->write(mapping);
	}
	
	for (s32 i = 0; i < (s32) vis.mappings[VIS_TIE].size(); i++) {
		OcclusionMapping mapping;
		const TieInstance& instance = level.instances.tie_instances[i];
		mapping.bit_index = vis.mappings[VIS_TIE][i];
		mapping.occlusion_id = instance.occlusion_index;
		mappings_dest->write(mapping);
	}
	
	s32 moby_instance_index = 0;
	for (s32 i = 0; i < (s32) vis.mappings[VIS_MOBY].size(); i++) {
		// Skip past moby instances for which we don't precompute occlusion.
		while (level.instances.moby_instances[moby_instance_index].occlusion
			|| moby_class_to_index.find(level.instances.moby_instances[moby_instance_index].o_class()) == moby_class_to_index.end()) {
			moby_instance_index++;
		}
		
		OcclusionMapping mapping;
		mapping.bit_index = vis.mappings[VIS_MOBY][i];
		mapping.occlusion_id = level.instances.moby_instances[moby_instance_index].uid;
		mappings_dest->write(mapping);
		
		moby_instance_index++;
	}
}
