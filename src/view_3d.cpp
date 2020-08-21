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

#include "view_3d.h"

#include "formats/level_impl.h"

view_3d::view_3d(app* a)
	: _frame_buffer_texture(0),
	  _selecting(false),
	  _renderer(&a->renderer) {}

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

	_viewport_size = ImGui::GetWindowSize();
	_viewport_size.y -= 19;
	
	glm::mat4 world_to_clip = get_world_to_clip();
	_renderer->prepare_frame(*lvl, world_to_clip);
	
	render_to_texture(&_frame_buffer_texture, _viewport_size.x, _viewport_size.y, [&]() {
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, _viewport_size.x, _viewport_size.y);
		
		_renderer->draw_level(*lvl, world_to_clip);
	});
	
	ImGui::Image((void*) (intptr_t) _frame_buffer_texture, _viewport_size);

	draw_overlay_text(*lvl);
	
	ImVec2 cursor_pos = ImGui::GetMousePos();
	ImVec2 rel_pos {
		cursor_pos.x - ImGui::GetWindowPos().x,
		cursor_pos.y - ImGui::GetWindowPos().y - 20
	};
	
	ImGuiIO& io = ImGui::GetIO();
	if(io.MouseClicked[0] && ImGui::IsWindowHovered()) {
		switch(a.active_tool().type) {
			case tool_type::picker:	pick_object(*lvl, world_to_clip, rel_pos); break;
			case tool_type::selection: select_rect(*lvl, cursor_pos); break;
			case tool_type::translate: break;
		}
		io.MouseClicked[0] = false;
	}
	
	auto draw_list = ImGui::GetWindowDrawList();
	if(a.active_tool().type == tool_type::selection && _selecting) {
		draw_list->AddRect(_selection_begin, cursor_pos, 0xffffffff);
	}
}

bool view_3d::has_padding() const {
	return false;
}

void view_3d::draw_overlay_text(level& lvl) const {
	auto draw_list = ImGui::GetWindowDrawList();
	
	glm::mat4 world_to_clip = get_world_to_clip();
	auto draw_text = [&](glm::mat4 mat, std::string text) {
		
		static const float max_distance = glm::pow(100.f, 2); // squared units	
		float distance =
			glm::abs(glm::pow(mat[3].x - _renderer->camera_position.x, 2)) +
			glm::abs(glm::pow(mat[3].y - _renderer->camera_position.y, 2)) +
			glm::abs(glm::pow(mat[3].z - _renderer->camera_position.z, 2));

		if(distance < max_distance) {
			glm::vec3 screen_pos = apply_local_to_screen(world_to_clip, mat);
			if (screen_pos.z > 0 && screen_pos.z < 1) {
				static const int colour = ImColor(1.0f, 1.0f, 1.0f, 1.0f);
				draw_list->AddText(ImVec2(screen_pos.x, screen_pos.y), colour, text.c_str());
			}
		}
	};
	
	for(tie_entity& tie : lvl.ties) {
		draw_text(tie.local_to_world, "t");
	}
	
	for(shrub_entity& shrub : lvl.shrubs) {
		draw_text(shrub.local_to_world, "s");
	}
	
	for(moby_entity& moby : lvl.mobies) {
		static const std::map<uint16_t, const char*> moby_class_names {
			{ 0x1f4, "crate" },
			{ 0x2f6, "swingshot_grapple" },
			{ 0x323, "swingshot_swinging" }
		};
		if(moby_class_names.find(moby.class_num) != moby_class_names.end()) {
			draw_text(moby.local_to_world_cache, moby_class_names.at(moby.class_num));
		} else {
			draw_text(moby.local_to_world_cache, std::to_string(moby.class_num));
		}
	}
}

glm::mat4 view_3d::get_world_to_clip() const {
	ImVec2 size = _viewport_size;
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), size.x / size.y, 0.1f, 10000.0f);

	auto rot = _renderer->camera_rotation;
	glm::mat4 pitch = glm::rotate(glm::mat4(1.0f), rot.x, glm::vec3(1.0f, 0.0f, 0.0f));
	glm::mat4 yaw   = glm::rotate(glm::mat4(1.0f), rot.y, glm::vec3(0.0f, 1.0f, 0.0f));
 
	glm::mat4 translate =
		glm::translate(glm::mat4(1.0f), -_renderer->camera_position);
	static const glm::mat4 yzx {
		0,  0, 1, 0,
		1,  0, 0, 0,
		0, -1, 0, 0,
		0,  0, 0, 1
	};
	glm::mat4 view = pitch * yaw * yzx * translate;

	return projection * view;
}

glm::mat4 view_3d::get_local_to_clip(glm::mat4 world_to_clip, glm::vec3 position, glm::vec3 rotation) const {
	glm::mat4 model = glm::translate(glm::mat4(1.f), position);
	model = glm::rotate(model, rotation.x, glm::vec3(1, 0, 0));
	model = glm::rotate(model, rotation.y, glm::vec3(0, 1, 0));
	model = glm::rotate(model, rotation.z, glm::vec3(0, 0, 1));
	return world_to_clip * model;
}

glm::vec3 view_3d::apply_local_to_screen(glm::mat4 world_to_clip, glm::mat4 local_to_world) const {
	glm::mat4 local_to_clip = get_local_to_clip(world_to_clip, glm::vec3(1.f), glm::vec3(0.f));
	glm::vec4 homogeneous_pos = local_to_clip * glm::vec4(glm::vec3(local_to_world[3]), 1);
	glm::vec3 gl_pos {
			homogeneous_pos.x / homogeneous_pos.w,
			homogeneous_pos.y / homogeneous_pos.w,
			homogeneous_pos.z / homogeneous_pos.w
	};
	ImVec2 window_pos = ImGui::GetWindowPos();
	glm::vec3 screen_pos(
			window_pos.x + (1 + gl_pos.x) * _viewport_size.x / 2.0,
			window_pos.y + (1 + gl_pos.y) * _viewport_size.y / 2.0,
			gl_pos.z
	);
	return screen_pos;
}

void view_3d::pick_object(level& lvl, glm::mat4 world_to_clip, ImVec2 position) {
	_renderer->draw_pickframe(lvl, world_to_clip);
	
	glFlush();
	glFinish();
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	
	// Select the object under the cursor, with a few pixels of leeway.
	constexpr int select_size = 9;
	constexpr int size = select_size * select_size;
	constexpr int middle = select_size / 2;
	
	uint32_t buffer[size];
	glReadPixels(position.x - middle, position.y - middle, select_size, select_size, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

	int smallest_index = -1;
	int smallest_value = size;
	for(int i = 0; i < size; i++) {
		if(buffer[i] > 0) {
			auto current_value = glm::abs(middle - i % select_size) + glm::abs(middle - i / select_size);
			if(current_value < smallest_value) {
				smallest_index = i;
				smallest_value = current_value;
			}
		}
	}

	if(smallest_value == -1) {
		lvl.clear_selection();
		return;
	}

	entity_id id { *(uint32_t*) &buffer[smallest_index] };
	lvl.for_each<entity>([&](entity& ent) {
		ent.selected = id == ent.id;
	});
}

void view_3d::select_rect(level& lvl, ImVec2 position) {
	if(!_selecting) {
		_selection_begin = position;
	} else {
		_selection_end = position;
		if(_selection_begin.x > _selection_end.x) {
			std::swap(_selection_begin.x, _selection_end.x);
		}
		if(_selection_begin.y > _selection_end.y) {
			std::swap(_selection_begin.y, _selection_end.y);
		}
		
		_selection_begin.y -= 20;
		_selection_end.y -= 20;
		
		auto in_bounds = [&](glm::vec3 screen_pos) {
			return screen_pos.z > 0 &&
				(screen_pos.x > _selection_begin.x && screen_pos.x < _selection_end.x) &&
				(screen_pos.y > _selection_begin.y && screen_pos.y < _selection_end.y);
		};
		
		glm::mat4 world_to_clip = get_world_to_clip();
		lvl.for_each<matrix_entity>([&](matrix_entity& ent) {
			glm::vec3 screen_pos = apply_local_to_screen(world_to_clip, ent.local_to_world);
			ent.selected = in_bounds(screen_pos);
		});
		lvl.for_each<euler_entity>([&](euler_entity& ent) {
			glm::vec3 screen_pos = apply_local_to_screen(world_to_clip, ent.local_to_world_cache);
			ent.selected = in_bounds(screen_pos);
		});
	}
	_selecting = !_selecting;
}
