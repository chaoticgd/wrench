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

#ifndef ENGINE_COLLISION_H
#define ENGINE_COLLISION_H

#include <core/buffer.h>
#include <core/collada.h>

struct CollisionOutput
{
	ColladaScene scene;
	std::string main_mesh;
	std::vector<std::string> hero_group_meshes;
};

CollisionOutput read_collision(Buffer src);

struct CollisionInput
{
	const ColladaScene* main_scene;
	std::string main_mesh;
	std::vector<const Mesh*> hero_groups;
};

void write_collision(OutBuffer dest, const CollisionInput& input);
std::vector<ColladaMaterial> create_collision_materials();

#endif
