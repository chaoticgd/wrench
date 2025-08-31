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

#ifndef GUI_RENDERER_H
#define GUI_RENDERER_H

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/glm.hpp>
#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <core/util.h>
#include <editor/level.h>
#include <editor/gui/imgui_includes.h>

class app;

// The games use a Z-up coordinate system, OpenGL uses a Y-up coordinate system.
const glm::mat4 RATCHET_TO_OPENGL_MATRIX = {
	0,  0, 1, 0,
	1,  0, 0, 0,
	0, -1, 0, 0,
	0,  0, 0, 1
};

struct RenderSettings
{
	bool camera_control { false };
	glm::vec3 camera_position { 0, 0, 0 };
	glm::vec2 camera_rotation { 0, 0 };
	
	bool draw_tfrags = true;
	bool draw_moby_instances = true;
	bool draw_moby_groups = true;
	bool draw_tie_instances = true;
	bool draw_tie_groups = true;
	bool draw_shrub_instances = true;
	bool draw_shrub_groups = true;
	bool draw_point_lights = true;
	bool draw_env_sample_points = true;
	bool draw_env_transitions = true;
	bool draw_cuboids = true;
	bool draw_spheres = true;
	bool draw_cylinders = true;
	bool draw_pills = true;
	bool draw_cameras = true;
	bool draw_sound_instances = true;
	bool draw_paths = true;
	bool draw_grind_paths = true;
	bool draw_areas = true;
	bool draw_collision = false;
	bool draw_hero_collision = false;
	
	bool draw_selected_instance_normals = false;
	
	ImVec2 view_pos;
	ImVec2 view_size;
	
	glm::mat4 view_ratchet;
	glm::mat4 view_gl;
	glm::mat4 projection;
};

void init_renderer();
void shutdown_renderer();
void prepare_frame(Level& lvl);
void draw_level(
	Level& lvl, const glm::mat4& view, const glm::mat4& projection, const RenderSettings& settings);
void draw_pickframe(
	Level& lvl, const glm::mat4& view, const glm::mat4& projection, const RenderSettings& settings);
void draw_model_preview(
	const RenderMesh& mesh,
	const std::vector<RenderMaterial>& materials,
	const glm::mat4* bb,
	const glm::mat4& view,
	const glm::mat4& projection,
	bool wireframe);
void draw_drag_ghosts(Level& lvl, const std::vector<InstanceId>& ids, const RenderSettings& settings);

glm::mat4 compose_view_matrix(const glm::vec3& cam_pos, const glm::vec2& cam_rot);
glm::mat4 compose_projection_matrix(const ImVec2& view_size);
glm::vec3 apply_local_to_screen(
	const glm::mat4& world_to_clip, const glm::mat4& local_to_world, const ImVec2& view_size);
glm::vec3 create_ray(
	const glm::mat4& world_to_clip, const ImVec2& screen_pos, const ImVec2& view_pos, const ImVec2& view_size);

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
