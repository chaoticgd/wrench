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

#ifndef EDITOR_INSTANCED_COLLISION_RECOVERY_H
#define EDITOR_INSTANCED_COLLISION_RECOVERY_H

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

std::vector<ColLevel> load_instance_collision_data(BuildAsset& build, std::function<bool()>&& check_is_still_running);
ColMappings generate_instance_collision_mappings(const std::vector<ColLevel>& levels);
Opt<Mesh> build_instanced_collision(s32 type, s32 o_class, const ColMappings& mappings, const std::vector<ColLevel>& levels, std::function<bool()>&& check_is_still_running);

#endif
