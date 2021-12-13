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
#include <editor/gui/imgui_includes.h>
#include <editor/formats/level_impl.h>

class app;

// The games use a Z-up coordinate system, OpenGL uses a Y-up coordinate system.
const glm::mat4 RATCHET_TO_OPENGL_MATRIX = {
	0,  0, 1, 0,
	1,  0, 0, 0,
	0, -1, 0, 0,
	0,  0, 0, 1
};;

struct RenderSettings {
	bool camera_control { false };
	glm::vec3 camera_position { 0, 0, 0 };
	glm::vec2 camera_rotation { 0, 0 };
	
	bool draw_ties = true;
	bool draw_shrubs = true;
	bool draw_mobies = true;
	bool draw_cuboids = false;
	bool draw_spheres = false;
	bool draw_cylinders = false;
	bool draw_paths = true;
	bool draw_grind_paths = true;
	bool draw_tfrags = true;
	bool draw_collision = true;
	
	bool flag = false;
	
	ImVec2 view_pos;
	ImVec2 view_size;
};

void init_renderer();
void prepare_frame(level& lvl, const glm::mat4& world_to_clip); // Compute local to world matrices for the instanced renderer.
void draw_level(level& lvl, const glm::mat4& world_to_clip, const RenderSettings& settings);
void draw_pickframe(level& lvl, const glm::mat4& world_to_clip, const RenderSettings& settings);

glm::mat4 compose_world_to_clip(const ImVec2& view_size, const glm::vec3& cam_pos, const glm::vec2& cam_rot);
glm::vec3 apply_local_to_screen(const glm::mat4& world_to_clip, const glm::mat4& local_to_world, const ImVec2& view_size);
glm::vec3 create_ray(const glm::mat4& world_to_clip, const ImVec2& screen_pos, const ImVec2& view_pos, const ImVec2& view_size);

void reset_camera(app* a);

template <typename T>
void render_to_texture(GLuint* target, GLsizei width, GLsizei height, T draw_callback) {
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
