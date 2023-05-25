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

#include <editor/app.h>

std::vector<std::unique_ptr<Tool>> enumerate_tools() {
	std::vector<std::unique_ptr<Tool>> tools;
	tools.emplace_back(std::make_unique<PickerTool>());
	tools.emplace_back(std::make_unique<SelectionTool>());
	tools.emplace_back(std::make_unique<TranslateTool>());
	return tools;
}

PickerTool::PickerTool() {
	icon = load_icon(0);
}

void PickerTool::draw(app& a, const glm::mat4& view, const glm::mat4& projection) {
	if(ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered()) {
		ImVec2 rel_pos {
			ImGui::GetMousePos().x - ImGui::GetWindowPos().x,
			ImGui::GetMousePos().y - ImGui::GetWindowPos().y - 20
		};
		pick_object(a, view, projection, rel_pos);
	}
}

void PickerTool::pick_object(app& a, const glm::mat4& view, const glm::mat4& projection, ImVec2 position) {
	Level& lvl = *a.get_level();
	
	GLint last_framebuffer;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &last_framebuffer);
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	draw_pickframe(lvl, view, projection, a.render_settings);
	
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
		lvl.instances().clear_selection();
		return;
	}
	
	bool is_multi_selecting = ImGui::GetIO().KeyCtrl;

	u8 picked_type = buffer[smallest_index] & 0xff;
	u16 picked_id = (buffer[smallest_index] & 0x00ffff00) >> 8;
	lvl.instances().for_each([&](Instance& inst) {
		bool same_type = inst.id().type == picked_type;
		bool same_id_value = inst.id().value == picked_id;
		if(is_multi_selecting) {
			inst.selected ^= same_type && same_id_value;
		} else {
			inst.selected = same_type && same_id_value;
		}
	});
	
	glBindFramebuffer(GL_FRAMEBUFFER, last_framebuffer);
}

SelectionTool::SelectionTool() {
	icon = load_icon(1);
}

void SelectionTool::draw(app& a, const glm::mat4& view, const glm::mat4& projection) {
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
		
		glm::mat4 world_to_clip = projection * view;
		
		Level& lvl = *a.get_level();
		lvl.instances().for_each_with(COM_TRANSFORM, [&](Instance& inst) {
			glm::vec3 screen_pos = apply_local_to_screen(world_to_clip, inst.transform().matrix(), a.render_settings.view_size);
			inst.selected = in_bounds(screen_pos);
		});
	}
}

TranslateTool::TranslateTool() {
	icon = load_icon(2);
}

void TranslateTool::draw(app& a, const glm::mat4& view, const glm::mat4& projection) {
	ImGui::Begin("Translate Tool");
	ImGui::Text("Displacement:");
	ImGui::InputFloat3("##displacement_input", &_displacement.x);
	if(ImGui::Button("Apply") && glm::length(_displacement) > 0.001f) {
		verify_fatal(a.get_level());
		Level& lvl = *a.get_level();
		
		struct TranslateCommand {
			std::vector<InstanceId> ids;
			glm::vec3 displacement;
			std::vector<std::pair<InstanceId, glm::vec3>> old_positions;
		};
		
		TranslateCommand data;
		data.ids = lvl.instances().selected_instances();
		data.displacement = _displacement;
		lvl.instances().for_each_with(COM_TRANSFORM, [&](Instance& inst) {
			if(inst.selected) {
				data.old_positions.emplace_back(inst.id(), inst.transform().pos());
			}
		});
		
		lvl.push_command<TranslateCommand>(std::move(data),
			[](Level& lvl, TranslateCommand& data) {
				lvl.instances().for_each_with(COM_TRANSFORM, [&](Instance& inst) {
					if(contains(data.ids, inst.id())) {
						inst.transform().set_from_pos_rot_scale(inst.transform().pos() + data.displacement, inst.transform().rot(), inst.transform().scale());
					}
				});
			},
			[](Level& lvl, TranslateCommand& data) {
				size_t i = 0;
				while(i < data.old_positions.size()) {
					lvl.instances().for_each_with(COM_TRANSFORM, [&](Instance& inst) {
						if(inst.id() == data.old_positions[i].first) {
							inst.transform().set_from_pos_rot_scale(data.old_positions[i].second, inst.transform().rot(), inst.transform().scale());
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
