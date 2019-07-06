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

#include "renderer.h"

#include "level.h"

three_d_view::three_d_view(const app* a)
	: camera_control(false), _frame_buffer_texture(0) {
	reset_camera(*a);
}

three_d_view::~three_d_view() {
	if(_frame_buffer_texture != 0) {
		glDeleteTextures(1, &_frame_buffer_texture);
	}
}

const char* three_d_view::title_text() const {
	return "3D View";
}

ImVec2 three_d_view::initial_size() const {
	return ImVec2(800, 600);
}

void three_d_view::render(app& a) {
	if(_shaders.get() == nullptr) {
		_shaders = std::make_unique<shader_programs>();
	}

	_viewport_size = ImGui::GetWindowSize();
	_viewport_size.x -= 32;
	_viewport_size.y -= 48;

	if(_frame_buffer_texture != 0) {
		glDeleteTextures(1, &_frame_buffer_texture);
	}

	// Draw the 3D scene onto a texture.
	glGenTextures(1, &_frame_buffer_texture);
	glBindTexture(GL_TEXTURE_2D, _frame_buffer_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _viewport_size.x, _viewport_size.y, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	GLuint fb_id;
	glGenFramebuffers(1, &fb_id);
	glBindFramebuffer(GL_FRAMEBUFFER, fb_id);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _frame_buffer_texture, 0);

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, _viewport_size.x, _viewport_size.y);

	draw_current_level(a);

	glDeleteFramebuffers(1, &fb_id);

	// Tell ImGui to draw that texture.
	ImGui::Image((void*) (intptr_t) _frame_buffer_texture, _viewport_size);

	draw_overlay_text(a);
}

void three_d_view::reset_camera(const app& a) {
	bool has_level = false;
	if(auto lvl = a.get_level()) {
		const auto& mobies = lvl->mobies();
		glm::vec3 sum(0, 0, 0);
		for(const auto& [uid, moby] : mobies) {
			sum += moby->position();
		}
		camera_position = sum / static_cast<float>(mobies.size());
		has_level = true;
	}
	if(!has_level) {
		camera_position = glm::vec3(0, 0, 0);
	}

	camera_rotation = glm::vec3(0, 0, 0);
}

void three_d_view::draw_current_level(const app& a) const {
	if(auto lvl = a.get_level()) {
		draw_level(*lvl);
	}
}

void three_d_view::draw_level(const level& lvl) const {
	glm::mat4 projection_view = get_view_projection_matrix();

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	for(auto& [uid, moby] : lvl.mobies()) {
		glm::mat4 model = glm::translate(glm::mat4(1.f), moby->position());
		glm::mat4 mvp = projection_view * model;
		glm::vec3 colour = glm::vec3(1, 0, 0);
		//	lvl.is_selected(uid) ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);
		draw_test_tri(mvp, colour);
	}

	// Draw ship.
	{
		//glm::mat4 model = glm::translate(glm::mat4(1.f), lvl.ship.position());
		//glm::mat4 mvp = projection_view * model;
		//draw_test_tri(mvp, glm::vec3(0, 0, 1));
	}
}

void three_d_view::draw_test_tri(glm::mat4 mvp, glm::vec3 colour) const {
	using v = glm::vec3;
	static const vertex_list vertex_data {
		{ v(1, 1, 0), v(1, 0, 1), v(0, 1, 0) }
	};
	
	glUseProgram(_shaders->solid_colour.id());

	glUniformMatrix4fv(_shaders->solid_colour_transform, 1, GL_FALSE, &mvp[0][0]);
	glUniform4f(_shaders->solid_colour_rgb, colour.x, colour.y, colour.z, 1);

	GLuint vertex_buffer;
	glGenBuffers(1, &vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER,
		vertex_data.size() * sizeof(vertex_list::value_type),
		&vertex_data[0][0].x, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	glDisableVertexAttribArray(0);
	glDeleteBuffers(1, &vertex_buffer);
}

void three_d_view::draw_overlay_text(const app& a) const {
	// Draw floating text over each moby showing its class name.
	if(auto lvl = a.get_level()) {
		ImDrawList* draw_list = ImGui::GetWindowDrawList();
		ImVec2 window_pos = ImGui::GetWindowPos();
		for(const auto& object : lvl->point_objects()) {
			glm::mat4 model = glm::translate(glm::mat4(1.f), object->position());
			glm::vec4 homogeneous_pos = get_view_projection_matrix() * model * glm::vec4(0, 0, 0, 1);
			glm::vec3 gl_pos = {
				homogeneous_pos.x / homogeneous_pos.w,
				homogeneous_pos.y / homogeneous_pos.w,
				homogeneous_pos.z / homogeneous_pos.w
			};
			if(gl_pos.z > 0 && gl_pos.z < 1) {
				ImVec2 position(
					window_pos.x + (1 + gl_pos.x) * _viewport_size.x / 2.0,
					window_pos.y + (1 + gl_pos.y) * _viewport_size.y / 2.0
				);
				static const int colour = ImColor(1.0f, 1.0f, 1.0f, 1.0f);
				draw_list->AddText(position, colour, object->label().c_str());
			}
		}
	}
}

glm::mat4 three_d_view::get_view_projection_matrix() const {
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), 4.0f / 3.0f, 0.1f, 100.0f);

	auto rot = camera_rotation;
	glm::mat4 pitch = glm::rotate(glm::mat4(1.0f), rot.x, glm::vec3(1.0f, 0.0f, 0.0f));
	glm::mat4 yaw   = glm::rotate(glm::mat4(1.0f), rot.y, glm::vec3(0.0f, 1.0f, 0.0f));
 
	glm::mat4 translate =
		glm::translate(glm::mat4(1.0f), -camera_position);
	static const glm::mat4 yzx {
		0,  0, 1, 0,
		1,  0, 0, 0,
		0, -1, 0, 0,
		0,  0, 0, 1
	};
	glm::mat4 view = pitch * yaw * yzx * translate;

	return projection * view;
}
