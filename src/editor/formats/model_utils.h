/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2020 chaoticgd

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

#ifndef FORMATS_MODEL_UTILS_H
#define FORMATS_MODEL_UTILS_H

#include <string>
#include <vector>
#include <glm/glm.hpp>

struct ply_vertex {
	float x, y, z, nx, ny, nz, s, t;
};

// This is far from a full-featured implementation of the PLY file format and
// is mainly just useful for testing. I'll replace this at some point.
//
// It assumes that each vertex is laid out like the above ply_vertex struct, and
// that the index buffer is equivalent to using GL_TRIANGLES without indices.
std::vector<ply_vertex> read_ply_model(std::string path);

#endif
