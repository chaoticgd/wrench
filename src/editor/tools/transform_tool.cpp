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

#include <editor/app.h>
#include <editor/tools.h>

#include <imguizmo/ImGuizmo.h>

static void activate();
static void deactivate();
static void update();
static void draw();

ToolInfo g_transform_tool_info = {
	"Transform Tool",
	{
		&activate,
		&deactivate,
		&update,
		&draw
	}
};

struct GizmoTransformInfo {
	InstanceId id;
	glm::mat4 inst_matrix;
	TransformComponent old_transform;
};

struct GizmoTransformCommand {
	glm::mat4 base_matrix;
	std::vector<GizmoTransformInfo> instances;
};

enum TransformState {
	TS_INACTIVE,
	TS_BEGIN,
	TS_DRAGGING,
	TS_END
};

static GizmoTransformCommand command;
static TransformState state;

static void push_gizmo_transform_command(Level& lvl, GizmoTransformCommand& command);

static void activate() {
	
}

static void deactivate() {
	command = {};
	state = TS_INACTIVE;
}

static void update() {
	ImGuizmo::SetDrawlist();
	ImVec2& view_pos = g_app->render_settings.view_pos;
	ImVec2& view_size = g_app->render_settings.view_size;
	ImGuizmo::SetRect(view_pos.x, view_pos.y, view_size.x, view_size.y);
	ImGuizmo::AllowAxisFlip(false);
	ImGuizmo::SetGizmoSizeClipSpace(0.2f);
	
	Level& lvl = *g_app->get_level();
	
	static glm::mat4 mtx(0.f);
	
	static glm::mat4 RATCHET_TO_IMGUIZMO = {
		0.f, 0.f, 1.f, 0.f,
		1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 0.f, 1.f
	};
	
	glm::mat4 v = RATCHET_TO_IMGUIZMO * g_app->render_settings.view_ratchet;
	glm::mat4 p = g_app->render_settings.projection;
	ImGuizmo::Manipulate(&v[0][0], &p[0][0], ImGuizmo::TRANSLATE | ImGuizmo::ROTATE | ImGuizmo::SCALE, ImGuizmo::WORLD, &command.base_matrix[0][0]);
	
	static bool was_dragging = false;
	bool is_dragging = ImGuizmo::IsUsing();
	if(!was_dragging) {
		if(!is_dragging) {
			state = TS_INACTIVE;
		} else {
			state = TS_BEGIN;
		}
	} else {
		if(is_dragging) {
			state = TS_DRAGGING;
		} else {
			state = TS_END;
		}
	}
	was_dragging = is_dragging;
	
	switch(state) {
		case TS_INACTIVE: {
			command.base_matrix = glm::mat4(0.f);
			f32 count = 0.f;
			lvl.instances().for_each_with(COM_TRANSFORM, [&](Instance& inst) {
				if(inst.selected) {
					command.base_matrix += inst.transform().matrix();
					count++;
				}
			});
			command.base_matrix /= count;
			break;
		}
		case TS_BEGIN: {
			command.instances.clear();
			glm::mat4 inverse_base = glm::inverse(command.base_matrix);
			lvl.instances().for_each_with(COM_TRANSFORM, [&](Instance& inst) {
				if(inst.selected) {
					GizmoTransformInfo& info = command.instances.emplace_back();
					info.id = inst.id();
					info.inst_matrix = inst.transform().matrix() * inverse_base;
					info.old_transform = inst.transform();
				}
			});
			break;
		}
		case TS_DRAGGING: {
			break;
		}
		case TS_END: {
			push_gizmo_transform_command(lvl, command);
			command = {};
			break;
		}
	}
}

static void draw() {
	if(state != TS_DRAGGING) {
		return;
	}
	
	Level* lvl = g_app->get_level();
	verify_fatal(lvl);
	
	static std::vector<std::pair<InstanceId, glm::mat4>> instances;
	instances.clear();
	for(const GizmoTransformInfo& info : command.instances) {
		instances.emplace_back(info.id, info.inst_matrix * command.base_matrix);
	}

	draw_drag_preview(*lvl, instances, g_app->render_settings);
}

static void push_gizmo_transform_command(Level& lvl, GizmoTransformCommand& command) {
	lvl.push_command<GizmoTransformCommand>(std::move(command),
		[](Level& lvl, GizmoTransformCommand& command) {
			for(const GizmoTransformInfo& info : command.instances) {
				Instance* inst = lvl.instances().from_id(info.id);
				verify_fatal(inst);
				glm::mat4 matrix = info.inst_matrix * command.base_matrix;
				inst->transform().set_from_matrix(&matrix);
			}
		},
		[](Level& lvl, GizmoTransformCommand& command) {
			for(const GizmoTransformInfo& info : command.instances) {
				Instance* inst = lvl.instances().from_id(info.id);
				verify_fatal(inst);
				inst->transform() = info.old_transform;
			}
		}
	);
}
