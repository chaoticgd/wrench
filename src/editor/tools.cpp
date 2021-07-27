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

#include "tools.h"

#include <glm/gtx/intersect.hpp>

#include "app.h"
#include "meshes.h"

std::vector<std::unique_ptr<tool>> enumerate_tools() {
	std::vector<std::unique_ptr<tool>> tools;
	tools.emplace_back(std::make_unique<picker_tool>());
	tools.emplace_back(std::make_unique<selection_tool>());
	tools.emplace_back(std::make_unique<translate_tool>());
	return tools;
}

picker_tool::picker_tool() {
	icon = load_icon("data/icons/picker_tool.txt");
}

void picker_tool::draw(app& a, glm::mat4 world_to_clip) {
	if(ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered()) {
		ImVec2 rel_pos {
			ImGui::GetMousePos().x - ImGui::GetWindowPos().x,
			ImGui::GetMousePos().y - ImGui::GetWindowPos().y - 20
		};
		pick_object(a, world_to_clip, rel_pos);
	}
}

void picker_tool::pick_object(app& a, glm::mat4 world_to_clip, ImVec2 position) {
	level& lvl = *a.get_level();
	
	GLint last_framebuffer;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &last_framebuffer);
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	a.renderer.draw_pickframe(lvl, world_to_clip);
	
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
		lvl.gameplay().clear_selection();
		return;
	}

	u8 picked_type = buffer[smallest_index] & 0xff;
	u16 picked_id = (buffer[smallest_index] & 0x00ffff00) >> 8;
	lvl.gameplay().for_each_instance([&](Instance& inst) {
		bool same_type = inst.id().type == picked_type;
		bool same_id_value = inst.id().value == picked_id;
		inst.selected = same_type && same_id_value;
	});
	
	glBindFramebuffer(GL_FRAMEBUFFER, last_framebuffer);
}

selection_tool::selection_tool() {
	icon = load_icon("data/icons/selection_tool.txt");
}

void selection_tool::draw(app& a, glm::mat4 world_to_clip) {
	if(ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered()) {
		_selecting = true;
		_selection_begin = ImGui::GetMousePos();
	}
	
	if(_selecting) {
		auto draw_list = ImGui::GetWindowDrawList();
		draw_list->AddRect(_selection_begin, ImGui::GetMousePos(), 0xffffffff);
	}
	
	if(ImGui::IsMouseReleased(0) && _selecting) {
		_selecting = false;
		
		ImVec2 p1 = _selection_begin;
		ImVec2 p2 = ImGui::GetMousePos();
		if(p1.x > p2.x) {
			std::swap(p1.x, p2.x);
		}
		if(p1.y > p2.y) {
			std::swap(p1.y, p2.y);
		}
		p1.y -= 20;
		p2.y -= 20;
		
		auto in_bounds = [&](glm::vec3 screen_pos) {
			return screen_pos.z > 0 &&
				(screen_pos.x > p1.x && screen_pos.x < p2.x) &&
				(screen_pos.y > p1.y && screen_pos.y < p2.y);
		};
		
		level& lvl = *a.get_level();
		lvl.gameplay().for_each_instance_with(COM_TRANSFORM, [&](Instance& inst) {
			glm::vec3 screen_pos = a.renderer.apply_local_to_screen(world_to_clip, inst.matrix());
			inst.selected = in_bounds(screen_pos);
		});
	}
}

translate_tool::translate_tool() {
	icon = load_icon("data/icons/translate_tool.txt");
}

void translate_tool::draw(app& a, glm::mat4 world_to_clip) {
	ImGui::Begin("Translate Tool");
	ImGui::Text("Displacement:");
	ImGui::InputFloat3("##displacement_input", &_displacement.x);
	if(ImGui::Button("Apply") && glm::length(_displacement) > 0.001f) {
		assert(a.get_level());
		level& lvl = *a.get_level();
		std::vector<InstanceId> ids = lvl.gameplay().selected_instances();
		glm::vec3 displacement = _displacement;
		std::vector<std::pair<InstanceId, glm::vec3>> old_positions;
		lvl.gameplay().for_each_instance_with(COM_TRANSFORM, [&](Instance& inst) {
			if(inst.selected) {
				old_positions.emplace_back(inst.id(), inst.position());
			}
		});
		
		lvl.push_command(
			[ids, displacement](level& lvl) {
				lvl.gameplay().for_each_instance_with(COM_TRANSFORM, [&](Instance& inst) {
					if(contains(ids, inst.id())) {
						inst.set_position(inst.position() + displacement);
					}
				});
			},
			[old_positions](level& lvl) {
				size_t i = 0;
				while(i < old_positions.size()) {
					lvl.gameplay().for_each_instance_with(COM_TRANSFORM, [&](Instance& inst) {
						if(inst.id() == old_positions[i].first) {
							inst.set_position(old_positions[i].second);
							i++;
						}
					});
				}
			}
		);
		_displacement = glm::vec3(0, 0, 0);
	}
	ImGui::End();
}
