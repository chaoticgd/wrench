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

	ImVec2& viewport_size = a.renderer.viewport_size;
	viewport_size = ImGui::GetWindowContentRegionMax();
	viewport_size.x -= ImGui::GetWindowContentRegionMin().x;
	viewport_size.y -= ImGui::GetWindowContentRegionMin().y;
	
	glm::mat4 world_to_clip = a.renderer.get_world_to_clip();
	a.renderer.prepare_frame(*lvl, world_to_clip);
	
	render_to_texture(&_frame_buffer_texture, viewport_size.x, viewport_size.y, [&]() {
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, viewport_size.x, viewport_size.y);
		
		a.renderer.draw_level(*lvl, world_to_clip);
	});
	
	ImGui::Image((void*) (intptr_t) _frame_buffer_texture, viewport_size);

	draw_overlay_text(a, world_to_clip);
	
	a.active_tool().draw(a, world_to_clip);
}

bool view_3d::has_padding() const {
	return false;
}

void view_3d::draw_overlay_text(app& a, glm::mat4 world_to_clip) const {
	auto draw_list = ImGui::GetWindowDrawList();
	
	auto draw_text = [&](glm::mat4 mat, std::string text) {
		
		static const float max_distance = glm::pow(100.f, 2); // squared units
		float distance =
			glm::abs(glm::pow(mat[3].x - a.renderer.camera_position.x, 2)) +
			glm::abs(glm::pow(mat[3].y - a.renderer.camera_position.y, 2)) +
			glm::abs(glm::pow(mat[3].z - a.renderer.camera_position.z, 2));

		if(distance < max_distance) {
			glm::vec3 screen_pos = a.renderer.apply_local_to_screen(world_to_clip, mat);
			if (screen_pos.z > 0 && screen_pos.z < 1) {
				static const int colour = ImColor(1.0f, 1.0f, 1.0f, 1.0f);
				draw_list->AddText(ImVec2(screen_pos.x, screen_pos.y), colour, text.c_str());
			}
		}
	};
	
	level& lvl = *a.get_level();
	
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
