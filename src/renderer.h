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

#include "util.h"
#include "shaders.h"
#include "formats/game_model.h"

# /*
#	Rendering functions.
# */

class app;

enum class view_mode {
	WIREFRAME = 0,
	TEXTURED_POLYGONS = 1
};

struct gl_renderer {
	void draw_spline(const std::vector<glm::vec3>& points, const glm::mat4& vp, const glm::vec4& colour) const;
	void draw_tris  (const std::vector<float>& vertex_data, const glm::mat4& mvp, const glm::vec4& colour) const;
	void draw_model (const model& mdl, const glm::mat4& mvp, const glm::vec4& colour) const;
	void draw_cube  (const glm::mat4& mvp, const glm::vec4& colour) const;

	void draw_moby_model(
		moby_model& model,
		glm::mat4 local_to_clip,
		array_view<GLuint> textures,
		view_mode mode) const;
	
	static glm::vec4 colour_coded_submodel_index(std::size_t index, std::size_t submodel_count);
	
	shader_programs shaders;
	
	void reset_camera(app* a);
	
	// TODO: Put these somewhere else.
	bool camera_control { false };
	glm::vec3 camera_position { 0, 0, 0 };
	glm::vec2 camera_rotation { 0, 0 };
};

template <typename T>
void render_to_texture(GLuint* target, int width, int height, T draw_callback) {
	glDeleteTextures(1, target);
	
	glGenTextures(1, target);
	glBindTexture(GL_TEXTURE_2D, *target);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	static GLuint zbuffer_texture;
	glGenTextures(1, &zbuffer_texture);
	glBindTexture(GL_TEXTURE_2D, zbuffer_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	GLuint fb_id;
	glGenFramebuffers(1, &fb_id);
	glBindFramebuffer(GL_FRAMEBUFFER, fb_id);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *target, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, zbuffer_texture, 0);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, width, height);
	
	draw_callback();
	
	glDeleteFramebuffers(1, &fb_id);
	glDeleteTextures(1, &zbuffer_texture);
}

#endif
