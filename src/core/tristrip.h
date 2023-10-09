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

#ifndef CORE_TRISTRIP_H
#define CORE_TRISTRIP_H

#include <core/gltf.h>
#include <core/material.h>

enum class GeometryType {
	TRIANGLE_LIST,
	TRIANGLE_STRIP,
	TRIANGLE_FAN
};

struct GeometryPrimitive {
	GeometryType type;
	s32 index_begin;
	s32 index_count;
	s32 effective_material = 0;
};

struct GeometryPrimitives {
	std::vector<GeometryPrimitive> primitives;
	std::vector<s32> indices;
};

// Generates a set of triangle strips and triangle lists that cover a given mesh.
GeometryPrimitives weave_tristrips(const GLTF::Mesh& mesh, const std::vector<EffectiveMaterial>& effectives);

#endif
