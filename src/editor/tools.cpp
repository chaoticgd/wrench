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
	tools.emplace_back(std::make_unique<spline_tool>());
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
	//ImGui::Begin("Translate Tool");
	//ImGui::Text("Displacement:");
	//ImGui::InputFloat3("##displacement_input", &_displacement.x);
	//if(ImGui::Button("Apply")) {
	//	level* lvl = a.get_level();
	//	std::vector<entity_id> ids = lvl->selected_entity_ids();
	//	glm::vec3 displacement = _displacement;
	//	std::map<entity_id, glm::vec3> old_positions;
	//	lvl->for_each<matrix_entity>([&](matrix_entity& ent) {
	//		if(ent.selected) {
	//			old_positions[ent.id] = ent.local_to_world[3];
	//		}
	//	});
	//	lvl->for_each<euler_entity>([&](euler_entity& ent) {
	//		if(ent.selected) {
	//			old_positions[ent.id] = ent.position;
	//		}
	//	});
	//	
	//	lvl->push_command(
	//		[ids, displacement](level& lvl) {
	//			lvl.for_each<matrix_entity>([&](matrix_entity& ent) {
	//				if(contains(ids, ent.id)) {
	//					ent.local_to_world[3].x += displacement.x;
	//					ent.local_to_world[3].y += displacement.y;
	//					ent.local_to_world[3].z += displacement.z;
	//				}
	//			});
	//			lvl.for_each<euler_entity>([&](euler_entity& ent) {
	//				if(contains(ids, ent.id)) {
	//					ent.position += displacement;
	//				}
	//			});
	//		},
	//		[old_positions](level& lvl) {
	//			lvl.for_each<matrix_entity>([&](matrix_entity& ent) {
	//				if(map_contains(old_positions, ent.id)) {
	//					const glm::vec3& pos = old_positions.at(ent.id);
	//					ent.local_to_world[3].x = pos.x;
	//					ent.local_to_world[3].y = pos.y;
	//					ent.local_to_world[3].z = pos.z;
	//				}
	//			});
	//			lvl.for_each<euler_entity>([&](euler_entity& ent) {
	//				if(map_contains(old_positions, ent.id)) {
	//					ent.position = old_positions.at(ent.id);
	//				}
	//			});
	//		});
	//	_displacement = glm::vec3(0, 0, 0);
	//}
	//ImGui::End();
}

spline_tool::spline_tool() {
	icon = load_icon("data/icons/spline_tool.txt");
}

void spline_tool::draw(app& a, glm::mat4 world_to_clip) {
	//level& lvl = *a.get_level();
	//
	//if(ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered()) {
	//	glm::vec3 ray = a.renderer.create_ray(world_to_clip, ImGui::GetMousePos());
	//	
	//	// Test if we clicked on a spline vertex.
	//	float selected_distance = 0.f;
	//	lvl.for_each<spline_entity>([&](spline_entity& spline) {
	//		for(size_t i = 0; i < spline.vertices.size(); i++) {
	//			glm::vec3 vertex_pos(spline.vertices[i]);
	//			glm::vec3 intersect_pos, normal;
	//			if(glm::intersectRaySphere(a.renderer.camera_position, ray, vertex_pos, 4.f, intersect_pos, normal)) {
	//				float dist = glm::distance(a.renderer.camera_position, intersect_pos);
	//				if(_selected_spline.value == 0 || dist < selected_distance || spline.selected) {
	//					selected_distance = dist;
	//					_selected_spline = spline.id;
	//					_selected_vertex = i;
	//				}
	//			}
	//		}
	//	});
	//}
	//
	//auto calculate_new_point = [&](spline_entity* spline, glm::vec3 ray) -> std::optional<glm::vec3> {
	//	if(spline != nullptr && spline->vertices.size() > _selected_vertex) {
	//		glm::vec4& vertex = spline->vertices[_selected_vertex];
	//		float distance = 0;
	//		glm::intersectRayPlane(a.renderer.camera_position, ray, glm::vec3(vertex), glm::normalize(_plane_normal), distance);
	//		if(distance > 0) {
	//			glm::vec3 new_pos(a.renderer.camera_position + ray * distance);
	//			return new_pos;
	//		}
	//	}
	//	return std::nullopt;
	//};
	//
	//if(ImGui::IsMouseDown(0) && _selected_spline != NULL_ENTITY_ID) {
	//	glm::vec3 ray = a.renderer.create_ray(world_to_clip, ImGui::GetMousePos());
	//	spline_entity* spline = lvl.entity_from_id<spline_entity>(_selected_spline);
	//	auto point = calculate_new_point(spline, ray);
	//	if(point) {
	//		spline_entity preview;
	//		if(_selected_vertex >= 1) {
	//			preview.vertices.push_back(spline->vertices[_selected_vertex - 1]);
	//		}
	//		preview.vertices.push_back(glm::vec4(*point, -1.f));
	//		if(_selected_vertex < spline->vertices.size() - 1) {
	//			preview.vertices.push_back(spline->vertices[_selected_vertex + 1]);
	//		}
	//		a.renderer.draw_spline(preview, world_to_clip, glm::vec4(1.f, 0.5f, 0.7f, 1.f));
	//	}
	//}
	//
	//if(ImGui::IsMouseReleased(0)) {
	//	glm::vec3 ray = a.renderer.create_ray(world_to_clip, ImGui::GetMousePos());
	//	spline_entity* spline = lvl.entity_from_id<spline_entity>(_selected_spline);
	//	auto point = calculate_new_point(spline, ray);
	//	if(point) {
	//		auto id = _selected_spline;
	//		auto vertex = _selected_vertex;
	//		glm::vec4 old_point = spline->vertices[_selected_vertex];
	//		lvl.push_command(
	//			[id, vertex, old_point, point](level& lvl) {
	//				spline_entity* spline = lvl.entity_from_id<spline_entity>(id);
	//				assert(spline != nullptr);
	//				assert(spline->vertices.size() > vertex);
	//				spline->vertices[vertex] = glm::vec4(*point, old_point.w);
	//			},
	//			[id, vertex, old_point](level& lvl) {
	//				spline_entity* spline = lvl.entity_from_id<spline_entity>(id);
	//				assert(spline != nullptr);
	//				assert(spline->vertices.size() > vertex);
	//				spline->vertices[vertex] = old_point;
	//			});
	//	}
	//	_selected_spline = NULL_ENTITY_ID;
	//}
	//
	//static const glm::mat4 SPLINE_VERTEX_SCALE = glm::scale(glm::mat4(1.f), glm::vec3(0.25f));
	//lvl.gameplay().for_each_instance_with(COM_SPLINE, [&](Instance& inst) {
	//	for(glm::vec4 vertex : inst.spline()) {
	//		a.renderer.draw_static_mesh(
	//			SPHERE_MESH_VERTICES, sizeof(SPHERE_MESH_VERTICES),
	//			world_to_clip * glm::translate(glm::mat4(1.f), glm::vec3(vertex)) * SPLINE_VERTEX_SCALE,
	//			glm::vec4(1.f, 0.f, 0.f, 1.f));
	//	}
	//});
	//
	//ImGui::Begin("Spline Tool");
	//ImGui::Text("Normal");
	//bool i = _plane_normal == glm::vec3(1.f, 0.f, 0.f);
	//bool j = _plane_normal == glm::vec3(0.f, 1.f, 0.f);
	//bool k = _plane_normal == glm::vec3(0.f, 0.f, 1.f);
	//if(ImGui::RadioButton("I (X)", i) || ImGui::IsKeyPressed('X')) {
	//	_plane_normal = glm::vec3(1.f, 0.f, 0.f);
	//}
	//if(ImGui::RadioButton("J (C)", j) || ImGui::IsKeyPressed('C')) {
	//	_plane_normal = glm::vec3(0.f, 1.f, 0.f);
	//}
	//if(ImGui::RadioButton("K (V)", k) || ImGui::IsKeyPressed('V')) {
	//	_plane_normal = glm::vec3(0.f, 0.f, 1.f);
	//}
	//ImGui::RadioButton("Custom", !i && !j && !k);
	//ImGui::SameLine();
	//ImGui::SliderFloat3("##custom_normal", &_plane_normal[0], 0.f, 1.f);
	//ImGui::End();
}
