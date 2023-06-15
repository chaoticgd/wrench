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
#include <assetmgr/material_asset.h>
#include <instancemgr/gameplay.h>
#include <gui/render_mesh.h>
#include <editor/app.h>

Level::Level() {}

void Level::read(LevelAsset& asset, Game g) {
	game = g;
	_asset = &asset;
	_instances_asset = &level_wad().get_gameplay().as<InstancesAsset>();
		
	std::string text = _instances_asset->file().read_text_file(_instances_asset->src().path);
	_instances = read_instances(text);
	
	const std::map<std::string, CppType>& types = asset.forest().types();
	
	const CollectionAsset& chunk_collection = level_wad().get_chunks();
	for(s32 i = 0; i < 3; i++) {
		if(!chunk_collection.has_child(i)) {
			continue;
		}
		const ChunkAsset& chunk_asset = chunk_collection.get_child(i).as<ChunkAsset>();
		EditorChunk& chunk = chunks.emplace_back();
		
		const MeshAsset& collision_asset = chunk_asset.get_collision().as<CollisionAsset>().get_mesh();
		std::string collision_xml = collision_asset.file().read_text_file(collision_asset.src().path);
		ColladaScene collision_scene = read_collada((char*) collision_xml.data());
		for(const Mesh& mesh : collision_scene.meshes) {
			chunk.collision.emplace_back(upload_mesh(mesh, true));
		}
		chunk.collision_materials = upload_materials(collision_scene.materials, {});
		
		const TfragsAsset& tfrags_asset = chunk_asset.get_tfrags();
		
		if(!tfrags_asset.has_editor_mesh()) {
			continue;
		}
		const MeshAsset& tfrags_mesh_asset = tfrags_asset.get_editor_mesh();
		std::string xml = tfrags_mesh_asset.file().read_text_file(tfrags_mesh_asset.src().path);
		ColladaScene scene = read_collada((char*) xml.data());
		Mesh* mesh = scene.find_mesh(tfrags_mesh_asset.name());
		if(!mesh) {
			continue;
		}
		chunk.tfrags = upload_mesh(*mesh, true);
		
		if(i == 0 && tfrags_asset.has_materials()) {
			MaterialSet material_set = read_material_assets(tfrags_asset.get_materials());
			map_lhs_material_indices_to_rhs_list(scene, material_set.materials);
			
			std::vector<Texture> textures;
			for(FileReference ref : material_set.textures) {
				auto stream = ref.owner->open_binary_file_for_reading(ref);
				verify(stream.get(), "Failed to open shrub texture file.");
				Opt<Texture> texture = read_png(*stream.get());
				verify(texture.has_value(), "Failed to read shrub texture.");
				textures.emplace_back(*texture);
			}
			
			tfrag_materials = upload_materials(scene.materials, textures);
		}
	}
	
	level_wad().get_moby_classes().for_each_logical_child_of_type<MobyClassAsset>([&](MobyClassAsset& moby) {
		EditorClass& ec = moby_classes[moby.id()];
		if(moby.has_editor_mesh()) {
			MeshAsset& asset = moby.get_editor_mesh();
			std::string xml = asset.file().read_text_file(asset.src().path);
			ColladaScene scene = read_collada((char*) xml.data());
			Mesh* mesh = scene.find_mesh(asset.name());
			if(mesh) {
				std::vector<Texture> textures;
				moby.get_materials().for_each_logical_child_of_type<TextureAsset>([&](TextureAsset& texture) {
					auto stream = texture.file().open_binary_file_for_reading(texture.src());
					Opt<Texture> tex = read_png(*stream);
					if(tex) {
						textures.emplace_back(*tex);
					}
				});
				
				ec.mesh = *mesh;
				ec.render_mesh = upload_mesh(*mesh, true);
				ec.materials = upload_materials(scene.materials, textures);
			}
		}
		if(moby.has_editor_icon()) {
			TextureAsset& icon_asset = moby.get_editor_icon();
			std::unique_ptr<InputStream> stream = icon_asset.file().open_binary_file_for_reading(icon_asset.src());
			Opt<Texture> icon = read_png(*stream);
			if(icon.has_value()) {
				std::vector<Texture> textures = { std::move(*icon) };
				ColladaMaterial mat;
				mat.surface = MaterialSurface(0);
				ec.icon = upload_material(mat, textures);
			}
		}
		auto pvar_type = types.find(stringf("update%d", moby.id()));
		if(pvar_type != types.end()) {
			ec.pvar_type = &pvar_type->second;
		}
	});
	
	level_wad().get_tie_classes().for_each_logical_child_of_type<TieClassAsset>([&](TieClassAsset& tie) {
		if(!tie.has_editor_mesh()) {
			return;
		}
		MeshAsset& asset = tie.get_editor_mesh();
		std::string xml = asset.file().read_text_file(asset.src().path);
		ColladaScene scene = read_collada((char*) xml.data());
		Mesh* mesh = scene.find_mesh(asset.name());
		if(!mesh) {
			return;
		}
		
		MaterialSet material_set = read_material_assets(tie.get_materials());
		map_lhs_material_indices_to_rhs_list(scene, material_set.materials);
		
		std::vector<Texture> textures;
		for(FileReference ref : material_set.textures) {
			auto stream = ref.owner->open_binary_file_for_reading(ref);
			verify(stream.get(), "Failed to open shrub texture file.");
			Opt<Texture> texture = read_png(*stream.get());
			verify(texture.has_value(), "Failed to read shrub texture.");
			textures.emplace_back(*texture);
		}
		
		EditorClass et;
		et.mesh = *mesh;
		et.render_mesh = upload_mesh(*mesh, true);
		et.materials = upload_materials(scene.materials, textures);
		tie_classes.emplace(tie.id(), std::move(et));
	});
	
	level_wad().get_shrub_classes().for_each_logical_child_of_type<ShrubClassAsset>([&](ShrubClassAsset& shrub) {
		if(!shrub.has_core()) {
			return;
		}
		Asset& core_asset = shrub.get_core();
		if(core_asset.logical_type() != ShrubClassCoreAsset::ASSET_TYPE) {
			return;
		}
		ShrubClassCoreAsset& core = core_asset.as<ShrubClassCoreAsset>();
		if(!core.has_mesh()) {
			return;
		}
		MeshAsset& asset = core.get_mesh();
		std::string xml = asset.file().read_text_file(asset.src().path);
		ColladaScene scene = read_collada((char*) xml.data());
		Mesh* mesh = scene.find_mesh(asset.name());
		if(!mesh) {
			return;
		}
		
		MaterialSet material_set = read_material_assets(shrub.get_materials());
		map_lhs_material_indices_to_rhs_list(scene, material_set.materials);
		
		std::vector<Texture> textures;
		for(FileReference ref : material_set.textures) {
			auto stream = ref.owner->open_binary_file_for_reading(ref);
			verify(stream.get(), "Failed to open shrub texture file.");
			Opt<Texture> texture = read_png(*stream.get());
			verify(texture.has_value(), "Failed to read shrub texture.");
			textures.emplace_back(*texture);
		}
		
		EditorClass es;
		es.mesh = *mesh;
		es.render_mesh = upload_mesh(*mesh, true);
		es.materials = upload_materials(scene.materials, textures);
		shrub_classes.emplace(shrub.id(), std::move(es));
	});
	
	for(s32 i = 0; i < 100; i++) {
		auto pvar_type = types.find(stringf("camera%d", i));
		if(pvar_type != types.end()) {
			EditorClass& cam_class = camera_classes[i];
			cam_class.pvar_type = &pvar_type->second;
		}
	}
	
	for(s32 i = 0; i < 100; i++) {
		auto pvar_type = types.find(stringf("sound%d", i));
		if(pvar_type != types.end()) {
			EditorClass& sound_class = sound_classes[i];
			sound_class.pvar_type = &pvar_type->second;
		}
	}
}

void Level::save(const fs::path& path) {
	verify_fatal(_instances_asset);
	
	// If the gamplay asset isn't currently part of the mod, create a new .asset
	// file for it. Throwing the first time with retry=true will open a save
	// dialog and then the path argument will be populated with the chosen path.
	if(_instances_asset->bank().game_info.type != AssetBankType::MOD) {
		if(path.empty()) {
			throw SaveError{true, "No path specified."};
		}
		AssetFile& gameplay_file = g_app->mod_bank->asset_file(path);
		Asset& new_asset = gameplay_file.asset_from_link(InstancesAsset::ASSET_TYPE, _instances_asset->absolute_link());
		if(new_asset.logical_type() != InstancesAsset::ASSET_TYPE) {
			throw SaveError{false, "An asset of a different type already exists."};
		}
		_instances_asset = &new_asset.as<InstancesAsset>();
	}
	
	fs::path gameplay_path;
	if(_instances_asset->src().path.empty()) {
		// Make sure we're not overwriting another gameplay.bin file.
		if(!_instances_asset->file().file_exists("gameplay.bin")) {
			gameplay_path = "gameplay.bin";
		} else {
			throw SaveError{false, "A gameplay.bin file already exists in that folder."};
		}
	} else {
		gameplay_path = _instances_asset->src().path;
	}
	
	// Write out the gameplay.bin file.
	std::string text = write_instances(_instances);
	FileReference ref = _instances_asset->file().write_text_file(gameplay_path, text.c_str());
	_instances_asset->set_src(ref);
	
	_instances_asset->file().write();
}

LevelAsset& Level::level() {
	return *_asset;
}

LevelWadAsset& Level::level_wad() {
	return level().get_level().as<LevelWadAsset>();
}

Instances& Level::instances() {
	return _instances;
}


const Instances& Level::instances() const {
	return _instances;
}
