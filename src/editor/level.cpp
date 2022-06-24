/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2022 chaoticgd

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

Level::Level() {}

void Level::read(LevelAsset& asset, Game g) {
	game = g;
	_asset = &asset;
	
	const std::vector<GameplayBlockDescription>* gbd = nullptr;
	switch(game) {
		case Game::RAC: gbd = &RAC_GAMEPLAY_BLOCKS; break;
		case Game::GC: gbd = &GC_UYA_GAMEPLAY_BLOCKS; break;
		case Game::UYA: gbd = &GC_UYA_GAMEPLAY_BLOCKS; break;
		case Game::DL: gbd = &DL_GAMEPLAY_CORE_BLOCKS; break;
		default: verify_not_reached("Invalid game!"); break;
	}
		
	BinaryAsset& gameplay = gameplay_asset();
	auto stream = gameplay.file().open_binary_file_for_reading(gameplay.src());
	std::vector<u8> buffer = stream->read_multiple<u8>(stream->size());
	read_gameplay(_gameplay, _pvar_types, buffer, game, *gbd);
	
	MeshAsset& collision_asset = core().get_collision().as<CollisionAsset>().get_mesh();
	std::string collision_xml = collision_asset.file().read_text_file(collision_asset.src().path);
	ColladaScene collision_scene = read_collada((char*) collision_xml.data());
	for(const Mesh& mesh : collision_scene.meshes) {
		collision.emplace_back(upload_mesh(mesh, true));
	}
	collision_materials = upload_materials(collision_scene.materials, {});
	
	core().get_moby_classes().for_each_logical_child_of_type<MobyClassAsset>([&](MobyClassAsset& moby) {
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
				
				EditorMobyClass ec;
				ec.mesh = *mesh;
				ec.high_lod = upload_mesh(*mesh, true);
				ec.materials = upload_materials(scene.materials, textures);
				mobies.emplace(moby.id(), std::move(ec));
			}
		}
	});
}

void Level::write() {
	std::vector<u8> buffer;
	write_gameplay(_gameplay, _pvar_types, game, GC_UYA_GAMEPLAY_BLOCKS);
	
	BinaryAsset& gameplay = gameplay_asset();
	auto [stream, ref] = gameplay.file().open_binary_file_for_writing(gameplay.src().path);
	stream->write_v(buffer);
	gameplay.set_src(ref);
}

LevelAsset& Level::level() {
	return *_asset;
}

LevelWadAsset& Level::level_wad() {
	return level().get_level().as<LevelWadAsset>();
}

LevelDataWadAsset& Level::data_wad() {
	return level_wad().get_data().as<LevelDataWadAsset>();
}

LevelCoreAsset& Level::core() {
	return data_wad().get_core();
}

Gameplay& Level::gameplay() {
	return _gameplay;
}

BinaryAsset& Level::gameplay_asset() {
	return data_wad().get_gameplay().as<BinaryAsset>();
}
