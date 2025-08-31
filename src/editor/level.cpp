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

#include "level.h"

#include <core/png.h>
#include <assetmgr/asset_path_gen.h>
#include <assetmgr/material_asset.h>
#include <instancemgr/gameplay.h>
#include <toolwads/wads.h>
#include <gui/render_mesh.h>
#include <editor/app.h>

Level::Level() {}

void Level::read(LevelAsset& asset, Game g)
{
	// TODO: Refactor this mess.
	
	game = g;
	m_asset = &asset;
	m_instances_asset = &level_wad().get_gameplay().as<InstancesAsset>();
		
	std::string text = m_instances_asset->src().read_text_file();
	m_instances = read_instances(text);
	
	const std::map<std::string, CppType>& types = asset.forest().types();
	
	const CollectionAsset& chunk_collection = level_wad().get_chunks();
	for (s32 i = 0; i < 3; i++) {
		if (!chunk_collection.has_child(i)) {
			continue;
		}
		const ChunkAsset& chunk_asset = chunk_collection.get_child(i).as<ChunkAsset>();
		EditorChunk& chunk = chunks.emplace_back();
		
		const CollisionAsset& collision_asset = chunk_asset.get_collision().as<CollisionAsset>();
		
		const MeshAsset& collision_mesh_asset = chunk_asset.get_collision().as<CollisionAsset>().get_mesh();
		std::string collision_xml = collision_mesh_asset.src().read_text_file();
		ColladaScene collision_scene = read_collada((char*) collision_xml.data());
		const Mesh* collision_mesh = collision_scene.find_mesh(collision_mesh_asset.name());
		if (collision_mesh) {
			chunk.collision = upload_mesh(*collision_mesh, true);
		}
		chunk.collision_materials = upload_collada_materials(collision_scene.materials, {});
		
		std::vector<FileReference> hero_group_refs;
		std::vector<std::string> hero_group_names;
		collision_asset.get_hero_groups().for_each_logical_child_of_type<MeshAsset>([&](const MeshAsset& mesh) {
			hero_group_refs.emplace_back(mesh.src());
			hero_group_names.emplace_back(mesh.name());
		});
		
		std::vector<std::unique_ptr<ColladaScene>> hero_group_owners;
		std::vector<ColladaScene*> hero_group_scenes = read_collada_files(hero_group_owners, hero_group_refs);
		for (size_t i = 0; i < hero_group_scenes.size(); i++) {
			Mesh* mesh = hero_group_scenes[i]->find_mesh(hero_group_names[i]);
			if (mesh) {
				// Hero collision doesn't have a type, so make it all the same
				// colour.
				for (SubMesh& submesh : mesh->submeshes) {
					submesh.material = 0;
				}
				chunk.hero_collision.emplace_back(upload_mesh(*mesh, true));
			}
		}
		
		const TfragsAsset& tfrags_asset = chunk_asset.get_tfrags();
		
		if (!tfrags_asset.has_editor_mesh()) {
			continue;
		}
		const MeshAsset& tfrags_mesh_asset = tfrags_asset.get_editor_mesh();
		std::string xml = tfrags_mesh_asset.src().read_text_file();
		ColladaScene scene = read_collada((char*) xml.data());
		Mesh* mesh = scene.find_mesh(tfrags_mesh_asset.name());
		if (!mesh) {
			continue;
		}
		chunk.tfrags = upload_mesh(*mesh, true);
		
		if (i == 0 && tfrags_asset.has_materials()) {
			MaterialSet material_set = read_material_assets(tfrags_asset.get_materials());
			map_lhs_material_indices_to_rhs_list(scene, material_set.materials);
			
			std::vector<Texture> textures;
			for (FileReference ref : material_set.textures) {
				auto stream = ref.open_binary_file_for_reading();
				verify(stream.get(), "Failed to open shrub texture file.");
				Opt<Texture> texture = read_png(*stream.get());
				verify(texture.has_value(), "Failed to read shrub texture.");
				textures.emplace_back(*texture);
			}
			
			tfrag_materials = upload_collada_materials(scene.materials, textures);
		}
	}
	
	level_wad().get_moby_classes().for_each_logical_child_of_type<MobyClassAsset>([&](MobyClassAsset& moby) {
		Opt<EditorClass> ec = load_moby_editor_class(moby);
		if (ec.has_value()) {
			if (moby.has_editor_icon()) {
				TextureAsset& icon_asset = moby.get_editor_icon();
				std::unique_ptr<InputStream> stream = icon_asset.src().open_binary_file_for_reading();
				Opt<Texture> icon = read_png(*stream);
				if (icon.has_value()) {
					std::vector<Texture> textures = { std::move(*icon) };
					ColladaMaterial mat;
					mat.surface = MaterialSurface(0);
					ec->icon = upload_collada_material(mat, textures);
				}
			}
			auto pvar_type = types.find(stringf("update%d", moby.id()));
			if (pvar_type != types.end()) {
				ec->pvar_type = &pvar_type->second;
			}
			moby_classes.emplace(moby.id(), std::move(*ec));
		}
	});
	
	level_wad().get_tie_classes().for_each_logical_child_of_type<TieClassAsset>([&](TieClassAsset& tie) {
		Opt<EditorClass> ec = load_tie_editor_class(tie);
		if (ec.has_value()) {
			tie_classes.emplace(tie.id(), std::move(*ec));
		}
	});
	
	level_wad().get_shrub_classes().for_each_logical_child_of_type<ShrubClassAsset>([&](ShrubClassAsset& shrub) {
		Opt<EditorClass> ec = load_shrub_editor_class(shrub);
		if (ec.has_value()) {
			shrub_classes.emplace(shrub.id(), std::move(*ec));
		}
	});
	
	for (s32 i = 0; i < 100; i++) {
		auto pvar_type = types.find(stringf("camera%d", i));
		if (pvar_type != types.end()) {
			EditorClass& cam_class = camera_classes[i];
			cam_class.pvar_type = &pvar_type->second;
		}
	}
	
	for (s32 i = 0; i < 100; i++) {
		auto pvar_type = types.find(stringf("sound%d", i));
		if (pvar_type != types.end()) {
			EditorClass& sound_class = sound_classes[i];
			sound_class.pvar_type = &pvar_type->second;
		}
	}
}

std::string Level::save()
{
	verify_fatal(m_instances_asset);
	
	std::string message;
	
	// Setup the file structure so that the new instances file can be written
	// out in the new asset bank.
	if (&m_instances_asset->bank() != g_app->mod_bank && m_instances_asset->parent()) {
		s32 level_id = level_wad().id();
		std::string path = generate_level_asset_path(level_id, *level().parent());
		
		AssetFile& instances_file = g_app->mod_bank->asset_file(path);
		AssetLink link = level_wad().get_gameplay().absolute_link();
		m_instances_asset = &instances_file.asset_from_link(InstancesAsset::ASSET_TYPE, link).as<InstancesAsset>();
	
		message += stringf("Written file: %s\n", path.c_str());
	}
	
	fs::path gameplay_path;
	if (m_instances_asset->src().path.empty()) {
		// Make sure we're not overwriting another gameplay.instances file.
		verify(!m_instances_asset->file().file_exists("gameplay.instances"), "A gameplay.instances file already exists in that folder.");
		gameplay_path = "gameplay.instances";
	} else {
		gameplay_path = m_instances_asset->src().path;
	}
	
	// Write out the gameplay.bin file.
	const char* application_version;
	if (strlen(wadinfo.build.version_string) != 0) {
		application_version = wadinfo.build.version_string;
	} else {
		application_version = wadinfo.build.commit_string;
	}
	std::string text = write_instances(m_instances, "Wrench Editor", application_version);
	FileReference ref = m_instances_asset->file().write_text_file(gameplay_path, text.c_str());
	m_instances_asset->set_src(ref);
	
	m_instances_asset->file().write();
	
	message += stringf("Written file: %s\n", gameplay_path.string().c_str());
	
	return message;
}

LevelAsset& Level::level()
{
	return *m_asset;
}

LevelWadAsset& Level::level_wad()
{
	return level().get_level().as<LevelWadAsset>();
}

Instances& Level::instances()
{
	return m_instances;
}


const Instances& Level::instances() const
{
	return m_instances;
}

Opt<EditorClass> load_moby_editor_class(const MobyClassAsset& moby)
{
	if (!moby.has_editor_mesh()) {
		return std::nullopt;
	}
	const MeshAsset& asset = moby.get_editor_mesh();
	std::unique_ptr<InputStream> stream = asset.src().open_binary_file_for_reading();
	std::vector<u8> glb = stream->read_multiple<u8>(stream->size());
	GLTF::ModelFile gltf = GLTF::read_glb(glb);
	GLTF::Node* node = GLTF::lookup_node(gltf, asset.name().c_str());
	if (node == nullptr || !node->mesh.has_value() || *node->mesh < 0 || *node->mesh >= gltf.meshes.size()) {
		if (node==nullptr)
		return std::nullopt;
	}
	GLTF::Mesh& mesh = gltf.meshes[*node->mesh];
	
	MaterialSet material_set = read_material_assets(moby.get_materials());
	GLTF::map_gltf_materials_to_wrench_materials(gltf, material_set.materials);
	
	std::vector<Texture> textures;
	for (FileReference ref : material_set.textures) {
		auto stream = ref.open_binary_file_for_reading();
		verify(stream.get(), "Failed to open shrub texture file.");
		Opt<Texture> texture = read_png(*stream.get());
		verify(texture.has_value(), "Failed to read shrub texture.");
		textures.emplace_back(*texture);
	}
	
	EditorClass editor_moby;
	editor_moby.render_mesh = upload_gltf_mesh(mesh, true);
	editor_moby.materials = upload_materials(material_set.materials, textures);
	return editor_moby;
}

Opt<EditorClass> load_tie_editor_class(const TieClassAsset& tie)
{
	if (!tie.has_editor_mesh()) {
		return std::nullopt;
	}
	const MeshAsset& asset = tie.get_editor_mesh();
	std::string xml = asset.src().read_text_file();
	ColladaScene scene = read_collada((char*) xml.data());
	Mesh* mesh = scene.find_mesh(asset.name());
	if (!mesh) {
		return std::nullopt;
	}
	
	MaterialSet material_set = read_material_assets(tie.get_materials());
	map_lhs_material_indices_to_rhs_list(scene, material_set.materials);
	
	std::vector<Texture> textures;
	for (FileReference ref : material_set.textures) {
		auto stream = ref.open_binary_file_for_reading();
		verify(stream.get(), "Failed to open shrub texture file.");
		Opt<Texture> texture = read_png(*stream.get());
		verify(texture.has_value(), "Failed to read shrub texture.");
		textures.emplace_back(*texture);
	}
	
	EditorClass editor_tie;
	editor_tie.mesh = std::move(*mesh);
	editor_tie.render_mesh = upload_mesh(*editor_tie.mesh, true);
	editor_tie.materials = upload_collada_materials(scene.materials, textures);
	return editor_tie;
}

Opt<EditorClass> load_shrub_editor_class(const ShrubClassAsset& shrub)
{
	if (!shrub.has_core()) {
		return std::nullopt;
	}
	const Asset& core_asset = shrub.get_core();
	if (core_asset.logical_type() != ShrubClassCoreAsset::ASSET_TYPE) {
		return std::nullopt;
	}
	const ShrubClassCoreAsset& core = core_asset.as<ShrubClassCoreAsset>();
	if (!core.has_mesh()) {
		return std::nullopt;
	}
	const MeshAsset& asset = core.get_mesh();
	std::unique_ptr<InputStream> stream = asset.src().open_binary_file_for_reading();
	std::vector<u8> glb = stream->read_multiple<u8>(stream->size());
	GLTF::ModelFile gltf = GLTF::read_glb(glb);
	GLTF::Node* node = GLTF::lookup_node(gltf, asset.name().c_str());
	if (node == nullptr || !node->mesh.has_value() || *node->mesh < 0 || *node->mesh >= gltf.meshes.size()) {
		return std::nullopt;
	}
	GLTF::Mesh& mesh = gltf.meshes[*node->mesh];
	
	MaterialSet material_set = read_material_assets(shrub.get_materials());
	GLTF::map_gltf_materials_to_wrench_materials(gltf, material_set.materials);
	
	std::vector<Texture> textures;
	for (FileReference ref : material_set.textures) {
		auto stream = ref.open_binary_file_for_reading();
		verify(stream.get(), "Failed to open shrub texture file.");
		Opt<Texture> texture = read_png(*stream.get());
		verify(texture.has_value(), "Failed to read shrub texture.");
		textures.emplace_back(*texture);
	}
	
	EditorClass editor_shrub;
	editor_shrub.render_mesh = upload_gltf_mesh(mesh, true);
	editor_shrub.materials = upload_materials(material_set.materials, textures);
	return editor_shrub;
}
