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

view_3d::view_3d(const app* a)
	: camera_control(false),
	  camera_position(0, 0, 0),
	  camera_rotation(0, 0),
	  _frame_buffer_texture(0) {
	reset_camera(*a);
}

view_3d::~view_3d() {
	if(_frame_buffer_texture != 0) {
		glDeleteTextures(1, &_frame_buffer_texture);
	}
}

const char* view_3d::title_text() const {
	return "3D View";
}

ImVec2 view_3d::initial_size() const {
	return ImVec2(800, 600);
}

void view_3d::render(app& a) {
	auto lvl = a.get_level();
	
	if(lvl == nullptr) {
		return;
	}
	
	if(_shaders.get() == nullptr) {
		_shaders = std::make_unique<shader_programs>();
	}

	_viewport_size = ImGui::GetWindowSize();
	_viewport_size.y -= 19;

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

	draw_level(*lvl);

	glDeleteFramebuffers(1, &fb_id);

	// Tell ImGui to draw that texture.
	ImGui::Image((void*) (intptr_t) _frame_buffer_texture, _viewport_size);

	draw_overlay_text(a);
	
	// Allow users to select objects in the 3D view.
	ImGuiIO& io = ImGui::GetIO();
	if(io.MouseClicked[0] && ImGui::IsWindowHovered()) {
		ImVec2 clicked_pos = io.MouseClickedPos[0];
		clicked_pos.x -= ImGui::GetWindowPos().x;
		clicked_pos.y -= ImGui::GetWindowPos().y + 20;
		pick_object(*lvl, clicked_pos);
		io.MouseClicked[0] = false;
	}
}

bool view_3d::has_padding() const {
	return false;
}

void view_3d::reset_camera(const app& a) {
	bool has_level = false;
	if(auto lvl = a.get_level()) {
		const auto& mobies = lvl->mobies();
		glm::vec3 sum(0, 0, 0);
		for(const auto& moby : mobies) {
			sum += moby.second->position();
		}
		camera_position = sum / static_cast<float>(mobies.size());
		has_level = true;
	}
	if(!has_level) {
		camera_position = glm::vec3(0, 0, 0);
	}

	camera_rotation = glm::vec3(0, 0, 0);
}

void view_3d::draw_level(const level& lvl) const {
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glm::mat4 projection_view = get_view_projection_matrix();

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glUseProgram(_shaders->solid_colour.id());

	for(auto& object : lvl.ties()) {
		glm::mat4 model = glm::translate(glm::mat4(1.f), object->position());
		glm::mat4 mvp = projection_view * model;
		glm::vec3 colour = lvl.is_selected(object) ?
			glm::vec3(1, 0, 0) : glm::vec3(0.5, 0, 1);
		draw_tris(object->object_model().triangles(), mvp, colour);
	}
	
	for(auto& object : lvl.mobies()) {
		glm::mat4 model = glm::translate(glm::mat4(1.f), object.second->position());
		glm::mat4 mvp = projection_view * model;
		glm::vec3 colour = lvl.is_selected(object.second) ?
			glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);
		draw_tris(object.second->object_model().triangles(), mvp, colour);
	}

	for(auto object : lvl.splines()) {
		glm::vec3 colour = lvl.is_selected(object) ?
			glm::vec3(1, 0, 0) : glm::vec3(1, 0.5, 0);
		draw_spline(object->points(), projection_view, colour);
	}

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void view_3d::draw_spline(const std::vector<glm::vec3>& points, glm::mat4 mvp, glm::vec3 colour) const {
	
	if(points.size() < 1) {
		return;
	}
	
	std::vector<float> vertex_data;
	for(std::size_t i = 0; i < points.size() - 1; i++) {
		vertex_data.push_back(points[i].x);
		vertex_data.push_back(points[i].y);
		vertex_data.push_back(points[i].z);
		
		vertex_data.push_back(points[i].x + 0.5);
		vertex_data.push_back(points[i].y);
		vertex_data.push_back(points[i].z);
		
		vertex_data.push_back(points[i + 1].x);
		vertex_data.push_back(points[i + 1].y);
		vertex_data.push_back(points[i + 1].z);
	}
	draw_tris(vertex_data, mvp, colour);
}

void view_3d::draw_tris(const std::vector<float>& vertex_data, glm::mat4 mvp, glm::vec3 colour) const {
	glUniformMatrix4fv(_shaders->solid_colour_transform, 1, GL_FALSE, &mvp[0][0]);
	glUniform4f(_shaders->solid_colour_rgb, colour.x, colour.y, colour.z, 1);

	GLuint vertex_buffer;
	glGenBuffers(1, &vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER,
		vertex_data.size() * sizeof(float),
		vertex_data.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	glDrawArrays(GL_TRIANGLES, 0, vertex_data.size() * 3);

	glDisableVertexAttribArray(0);
	glDeleteBuffers(1, &vertex_buffer);
}

void view_3d::draw_overlay_text(const app& a) const {
	// Draw floating text over each moby showing its class name.
	auto lvl = a.get_level();
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

glm::mat4 view_3d::get_view_projection_matrix() const {
	ImVec2 size = _viewport_size;
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), size.x / size.y, 0.1f, 100.0f);

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

void view_3d::pick_object(level& lvl, ImVec2 position) {
	draw_pickframe(lvl);
	
	glFlush();
	glFinish();
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	
	unsigned char coded_object[4];
	glReadPixels(position.x, position.y, 1 , 1, GL_RGBA, GL_UNSIGNED_BYTE, coded_object);
	
	uint8_t type = coded_object[0];
	uint16_t id = coded_object[1] + (coded_object[2] << 8);
	
	if(type == 1) { // Ties
		auto ties = lvl.ties();
		if(ties.size() > id) {
			lvl.selection = { ties[id]->base() };
			return;
		}
	} else if(type == 2) { // Mobies
		auto mobies = lvl.mobies();
		auto moby = mobies.begin();
		for(std::size_t i = 0; i < id; i++) {
			moby++;
			if(moby == mobies.end()) {
				return; // Error!
			}
		}
		lvl.selection = { moby->second->base() };
		return;
	} else if(type == 3) { // Splines
		auto splines = lvl.splines();
		if(splines.size() > id) {
			lvl.selection = { splines[id]->base() };
			return;
		}
	}
	
	lvl.selection = {};
}


void view_3d::draw_pickframe(const level& lvl) const {
	glm::mat4 projection_view = get_view_projection_matrix();

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	
	glUseProgram(_shaders->solid_colour.id());

	auto encode_pick_colour = [](uint8_t type, uint16_t i) {
		glm::vec3 colour;
		colour.r = type / 255.f;
		colour.g = (i & 0xff) / 255.f;
		colour.b = ((i & 0xff00) >> 8) / 255.0f;
		return colour;
	};

	auto ties = lvl.ties();
	for(auto iter = ties.begin(); iter != ties.end(); iter++) {
		auto object = *iter;
		std::size_t i = std::distance(ties.begin(), iter);
		
		glm::mat4 model = glm::translate(glm::mat4(1.f), object->position());
		glm::mat4 mvp = projection_view * model;
		glm::vec3 colour = encode_pick_colour(1, i);
		
		draw_tris(object->object_model().triangles(), mvp, colour);
	}

	auto mobies = lvl.mobies();
	for(auto iter = mobies.begin(); iter != mobies.end(); iter++) {
		auto object = iter->second;
		std::size_t i = std::distance(mobies.begin(), iter);
		
		glm::mat4 model = glm::translate(glm::mat4(1.f), object->position());
		glm::mat4 mvp = projection_view * model;
		glm::vec3 colour = encode_pick_colour(2, i);
		
		draw_tris(object->object_model().triangles(), mvp, colour);
	}

	auto splines = lvl.splines();
	for(auto iter = splines.begin(); iter != splines.end(); iter++) {
		auto object = *iter;
		
		std::size_t i = std::distance(splines.begin(), iter);
		glm::vec3 colour = encode_pick_colour(3, i);
		
		draw_spline(object->points(), projection_view, colour);
	}
}
