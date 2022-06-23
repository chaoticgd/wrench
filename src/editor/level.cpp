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

Level::Level(LevelAsset& asset, Game g)
	: game(g)
	, history(*this)
	, _asset(asset) {
	BinaryAsset& gameplay = gameplay_asset();
	auto stream = gameplay.file().open_binary_file_for_reading(gameplay.src());
	std::vector<u8> buffer = stream->read_multiple<u8>(stream->size());
	read_gameplay(_gameplay, _pvar_types, buffer, game, GC_UYA_GAMEPLAY_BLOCKS);
	
	MeshAsset& collision_asset = core().get_collision().as<CollisionAsset>().get_mesh();
	auto collision_stream = collision_asset.file().open_binary_file_for_reading(collision_asset.src());
	std::vector<u8> collision_bytes = collision_stream->read_multiple<u8>(0, collision_stream->size());
	ColladaScene collision_scene = read_collada(collision_bytes);
	for(const Mesh& mesh : collision_scene.meshes) {
		collision.emplace_back(upload_mesh(mesh, true));
	}
	collision_materials = upload_materials(collision_scene.materials, {});
}

LevelAsset& Level::level() {
	return _asset;
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

void Level::write() {
	std::vector<u8> buffer;
	write_gameplay(_gameplay, _pvar_types, game, GC_UYA_GAMEPLAY_BLOCKS);
	
	BinaryAsset& gameplay = gameplay_asset();
	auto [stream, ref] = gameplay.file().open_binary_file_for_writing(gameplay.src().path);
	stream->write_v(buffer);
	gameplay.set_src(ref);
}

BinaryAsset& Level::gameplay_asset() {
	return data_wad().get_gameplay().as<BinaryAsset>();
}
