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

#ifndef RENDERER_H
#define RENDERER_H

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "formats/game_model.h"
#include "shaders.h"

# /*
#	Rendering functions.
# */

class app;

struct gl_renderer {
	void draw_spline(const std::vector<glm::vec3>& points, const glm::mat4& vp, const glm::vec3& colour) const;
	void draw_lines (const std::vector<glm::vec3>& points, const glm::mat4& vp, const glm::vec3& colour) const;
	void draw_tris  (const std::vector<float>& vertex_data, const glm::mat4& mvp, const glm::vec3& colour) const;
	void draw_model (const game_model& mdl, const glm::mat4& mvp, const glm::vec3& colour) const;
	void draw_cube  (const glm::mat4& mvp, const glm::vec3& colour) const;

	shader_programs shaders;
	
	void reset_camera(app* a);
	
	// TODO: Put these somewhere else.
	bool camera_control { false };
	glm::vec3 camera_position { 0, 0, 0 };
	glm::vec2 camera_rotation { 0, 0 };
};

#endif
