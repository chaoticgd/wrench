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

#include "app.h"
#include "model.h"
#include "window.h"
#include "shaders.h"

# /*
#	Draw 3D view and overlay text.
# */

class view_3d : public window {
public:
	view_3d(const app* a);
	~view_3d();
	const char* title_text() const;
	ImVec2 initial_size() const;
	void render(app& a);

	void draw_current_level(const app& a) const;
	void draw_level(const level& lvl) const;
	void draw_model(const model& mdl, glm::mat4 mvp, glm::vec3 colour) const;

	void draw_overlay_text(const app& a) const;

	glm::mat4 get_view_projection_matrix() const;

	void reset_camera(const app& a);
	bool camera_control;
	glm::vec3 camera_position;
	glm::vec2 camera_rotation;
private:
	GLuint _frame_buffer_texture;
	std::unique_ptr<shader_programs> _shaders;
	ImVec2 _viewport_size;
};

#endif
