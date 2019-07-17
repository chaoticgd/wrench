/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019 chaoticgd

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

#ifndef FORMATS_VEC3F_H
#define FORMATS_VEC3F_H

#include <glm/glm.hpp>

#include "../stream.h"

packed_struct(vec3f,
	vec3f() { x = 0; y = 0; z = 0; }
	vec3f(glm::vec3 g) { x = g.x; y = g.y; z = g.z; }

	float x;
	float y;
	float z;

	glm::vec3 glm() {
		glm::vec3 result;
		result.x = x;
		result.y = y;
		result.z = z;
		return result;
	}
)

#endif
