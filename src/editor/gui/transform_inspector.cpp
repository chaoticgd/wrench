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

#include "transform_inspector.h"

#include <editor/gui/inspector.h>

static void pos_rot_scale_inspector(Level& lvl);
static void matrix_inspector(Level& lvl);
static bool string_to_float(f32* dest, const char* src);

void transform_inspector(Level& lvl)
{
	bool selected = false;
	lvl.instances().for_each_with(COM_TRANSFORM, [&](Instance& inst) {
		if (inst.selected) {
			selected = true;
		}
	});
	
	if (!selected) {
		return;
	}
	
	if (ImGui::CollapsingHeader("Transform")) {
		if (ImGui::BeginTabBar("##transform_modes")) {
			if (ImGui::BeginTabItem("Pos/Rot/Scale")) {
				pos_rot_scale_inspector(lvl);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Matrix")) {
				matrix_inspector(lvl);
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
	}
}

static void pos_rot_scale_inspector(Level& lvl)
{
	bool pos_equal[3] = {true, true, true};
	bool rot_equal[3] = {true, true, true};
	bool scale_equal = true;
	Instance* last = nullptr;
	lvl.instances().for_each_with(COM_TRANSFORM, [&](Instance& inst) {
		if (inst.selected) {
			if (last) {
				const glm::vec3& pos = inst.transform().pos();
				const glm::vec3& rot = inst.transform().rot();
				f32 scale = inst.transform().scale();
				const glm::vec3& last_pos = last->transform().pos();
				const glm::vec3& last_rot = last->transform().rot();
				f32 last_scale = last->transform().scale();
				pos_equal[0] &= pos.x == last_pos.x;
				pos_equal[1] &= pos.y == last_pos.y;
				pos_equal[2] &= pos.z == last_pos.z;
				rot_equal[0] &= rot.x == last_rot.x;
				rot_equal[1] &= rot.y == last_rot.y;
				rot_equal[2] &= rot.z == last_rot.z;
				scale_equal &= scale == last_scale;
			}
			last = &inst;
		}
	});
	
	if (!last) {
		return;
	}
	
	bool push_command = false;
	
	const glm::vec3& pos = last->transform().pos();
	std::array<std::string, MAX_LANES> pos_strings = {
		pos_equal[0] ? std::to_string(pos.x) : "",
		pos_equal[1] ? std::to_string(pos.y) : "",
		pos_equal[2] ? std::to_string(pos.z) : "",
	};
	std::array<bool, MAX_LANES> pos_changed;
	ImGui::PushID("pos");
	push_command |= inspector_input_text_n(pos_strings, pos_changed, 3);
	ImGui::PopID();
	
	const glm::vec3& rot = last->transform().rot();
	std::array<std::string, MAX_LANES> rot_strings = {
		rot_equal[0] ? std::to_string(rot.x) : "",
		rot_equal[1] ? std::to_string(rot.y) : "",
		rot_equal[2] ? std::to_string(rot.z) : "",
	};
	std::array<bool, MAX_LANES> rot_changed;
	ImGui::PushID("rot");
	push_command |= inspector_input_text_n(rot_strings, rot_changed, 3);
	ImGui::PopID();
	
	f32 scale = last->transform().scale();
	std::array<std::string, MAX_LANES> scale_strings = {
		scale_equal ? std::to_string(scale) : ""
	};
	std::array<bool, MAX_LANES> scale_changed;
	ImGui::PushID("scale");
	push_command |= inspector_input_text_n(scale_strings, scale_changed, 1);
	ImGui::PopID();
	
	if (!push_command) {
		return;
	}
	
	// Convert the strings that have changed back into floats.
	glm::vec3 new_pos;
	glm::vec3 new_rot;
	f32 new_scale;
	for (s32 i = 0; i < 3; i++) {
		if (pos_changed[i]) {
			if (!string_to_float(&new_pos[i], pos_strings[i].c_str())) {
				pos_changed[i] = false;
			}
		} else {
			new_pos[i] = -1.f; // Don't care.
		}
	}
	for (s32 i = 0; i < 3; i++) {
		if (rot_changed[i]) {
			if (!string_to_float(&new_rot[i], rot_strings[i].c_str())) {
				rot_changed[i] = false;
			}
		} else {
			new_rot[i] = -1.f; // Don't care.
		}
	}
	if (scale_changed[0]) {
		if (!string_to_float(&new_scale, scale_strings[0].c_str())) {
			scale_changed[0] = false;
		}
	} else {
		new_scale = -1.f; // Don't care.
	}
	
	// Check if any fields have still been changed, only counting fields
	// that have been successfully parsed to a float.
	bool still_changed = false;
	for (s32 i = 0; i < 3; i++) {
		still_changed |= pos_changed[i];
	}
	for (s32 i = 0; i < 3; i++) {
		still_changed |= rot_changed[i];
	}
	still_changed |= scale_changed[0];
	
	if (!still_changed) {
		return;
	}
	
	struct PosRotScaleTransformCommand
	{
		glm::vec3 new_pos;
		glm::vec3 new_rot;
		f32 new_scale;
		std::array<bool, MAX_LANES> pos_changed;
		std::array<bool, MAX_LANES> rot_changed;
		std::array<bool, MAX_LANES> scale_changed;
		std::vector<std::pair<InstanceId, TransformComponent>> instances;
	};
	
	PosRotScaleTransformCommand command;
	command.new_pos = new_pos;
	command.new_rot = new_rot;
	command.new_scale = new_scale;
	command.pos_changed = pos_changed;
	command.rot_changed = rot_changed;
	command.scale_changed = scale_changed;
	lvl.instances().for_each_with(COM_TRANSFORM, [&](Instance& inst) {
		if (inst.selected) {
			auto& [id, component] = command.instances.emplace_back();
			id = inst.id();
			component = inst.transform();
		}
	});
	
	lvl.push_command<PosRotScaleTransformCommand>(std::move(command),
		[](Level& lvl, PosRotScaleTransformCommand& command) {
			for (auto& [id, transform] : command.instances) {
				glm::vec3 pos = transform.pos();
				glm::vec3 rot = transform.rot();
				f32 scale = transform.scale();
				for (s32 i = 0; i < 3; i++) {
					if (command.pos_changed[i]) {
						pos[i] = command.new_pos[i];
					}
				}
				for (s32 i = 0; i < 3; i++) {
					if (command.rot_changed[i]) {
						rot[i] = command.new_rot[i];
					}
				}
				if (command.scale_changed[0]) {
					scale = command.new_scale;
				}
				Instance* inst = lvl.instances().from_id(id);
				verify_fatal(inst);
				inst->transform().set_from_pos_rot_scale(pos, rot, scale);
			}
		},
		[](Level& lvl, PosRotScaleTransformCommand& command) {
			for (auto& [id, transform] : command.instances) {
				Instance* inst = lvl.instances().from_id(id);
				verify_fatal(inst);
				inst->transform() = transform;
			}
		}
	);
}

static void matrix_inspector(Level& lvl)
{
	bool matrix_equal[4][4];
	for (s32 i = 0; i < 4; i++) {
		for (s32 j = 0; j < 4; j++) {
			matrix_equal[i][j] = true;
		}
	}
	Instance* last = nullptr;
	lvl.instances().for_each_with(COM_TRANSFORM, [&](Instance& inst) {
		if (inst.selected) {
			if (last) {
				const glm::mat4& matrix = inst.transform().matrix();
				const glm::mat4& last_matrix = last->transform().matrix();
				for (s32 i = 0; i < 4; i++) {
					for (s32 j = 0; j < 4; j++) {
						matrix_equal[i][j] &= matrix[i][j] == last_matrix[i][j];
					}
				}
			}
			last = &inst;
		}
	});
	
	if (!last) {
		return;
	}
	
	bool push_command = false;
	std::array<std::string, MAX_LANES> strings[4];
	std::array<bool, MAX_LANES> changed[4];
	
	for (s32 i = 0; i < 4; i++) {
		ImGui::PushID(i);
		const glm::vec4& row = last->transform().matrix()[i];
		strings[i] = {
			matrix_equal[i][0] ? std::to_string(row.x) : "",
			matrix_equal[i][1] ? std::to_string(row.y) : "",
			matrix_equal[i][2] ? std::to_string(row.z) : "",
			matrix_equal[i][3] ? std::to_string(row.w) : ""
		};
		push_command |= inspector_input_text_n(strings[i], changed[i], 4);
		ImGui::PopID();
	}
	
	if (!push_command) {
		return;
	}
	
	// Convert the strings that have changed back into floats.
	glm::mat4 new_matrix;
	for (s32 i = 0; i < 4; i++) {
		for (s32 j = 0; j < 4; j++) {
			if (changed[i][j]) {
				const char* str = strings[i][j].c_str();
				if (!string_to_float(&new_matrix[i][j], str)) {
					changed[i][j] = false;
				}
			} else {
				new_matrix[i][j] = -1.f; // Don't care.
			}
		}
	}
	
	// Check if any fields have still been changed, only counting fields
	// that have been successfully parsed to a float.
	bool still_changed = false;
	for (s32 i = 0; i < 4; i++) {
		for (s32 j = 0; j < 4; j++) {
			if (changed[i][j]) {
				still_changed = true;
			}
		}
	}
	
	if (!still_changed) {
		return;
	}
	
	struct MatrixTransformCommand
	{
		glm::mat4 new_matrix;
		std::array<bool, MAX_LANES> changed[4];
		std::vector<std::pair<InstanceId, TransformComponent>> instances;
	};
	
	MatrixTransformCommand command;
	command.new_matrix = new_matrix;
	command.changed[0] = changed[0];
	command.changed[1] = changed[1];
	command.changed[2] = changed[2];
	command.changed[3] = changed[3];
	lvl.instances().for_each_with(COM_TRANSFORM, [&](Instance& inst) {
		if (inst.selected) {
			auto& [id, component] = command.instances.emplace_back();
			id = inst.id();
			component = inst.transform();
		}
	});
	
	lvl.push_command<MatrixTransformCommand>(std::move(command),
		[](Level& lvl, MatrixTransformCommand& command) {
			for (auto& [id, transform] : command.instances) {
				glm::mat4 matrix = transform.matrix();
				for (s32 i = 0; i < 4; i++) {
					for (s32 j = 0; j < 4; j++) {
						if (command.changed[i][j]) {
							matrix[i][j] = command.new_matrix[i][j];
						}
					}
				}
				Instance* inst = lvl.instances().from_id(id);
				verify_fatal(inst);
				inst->transform().set_from_matrix(&matrix);
			}
		},
		[](Level& lvl, MatrixTransformCommand& command) {
			for (auto& [id, transform] : command.instances) {
				Instance* inst = lvl.instances().from_id(id);
				verify_fatal(inst);
				inst->transform() = transform;
			}
		}
	);
}

static bool string_to_float(f32* dest, const char* src)
{
	char* end = nullptr;
	f32 f = strtof(src, &end);
	if (end == src) {
		*dest = -1.f;
		return false;
	}
	*dest = f;
	return true;
}
