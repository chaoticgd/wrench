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

struct GizmoTransformInfo
{
	InstanceId id;
	glm::mat4 inst_matrix;
	TransformComponent old_transform;
};

struct GizmoTransformCommand
{
	std::vector<GizmoTransformInfo> instances;
};

enum TransformState
{
	TS_INACTIVE,
	TS_BEGIN,
	TS_DRAGGING,
	TS_END
};

static glm::vec3 origin_position;
static glm::mat4 gizmo_matrix;
static GizmoTransformCommand command;
static TransformState state;

static void push_gizmo_transform_command(Level& lvl, GizmoTransformCommand& command);

static void activate()
{
	
}

static void deactivate()
{
	command = {};
	state = TS_INACTIVE;
	
	Level& lvl = *g_app->get_level();
	
	lvl.instances().for_each([&](Instance& inst) {
		inst.is_dragging = false;
	});
}

static void update()
{
	ImGuizmo::SetDrawlist();
	ImVec2& view_pos = g_app->render_settings.view_pos;
	ImVec2& view_size = g_app->render_settings.view_size;
	ImGuizmo::SetRect(view_pos.x, view_pos.y, view_size.x, view_size.y);
	ImGuizmo::AllowAxisFlip(false);
	ImGuizmo::SetGizmoSizeClipSpace(0.2f);
	
	ImGuizmo::Style& style = ImGuizmo::GetStyle();
	style.TranslationLineThickness = 4.f;
	style.TranslationLineArrowSize = 11.f;
	style.RotationLineThickness = 4.f;
	style.Colors[ImGuizmo::DIRECTION_X] = ImColor(0xff, 0x33, 0x52, 0xff);
	style.Colors[ImGuizmo::DIRECTION_Y] = ImColor(0x8b, 0xdc, 0x00, 0xff);
	style.Colors[ImGuizmo::DIRECTION_Z] = ImColor(0x28, 0x90, 0xff, 0xff);
	style.Colors[ImGuizmo::SELECTION] = ImColor(0xff, 0xff, 0xff, 0xff);
	
	Level& lvl = *g_app->get_level();
	
	static glm::mat4 RATCHET_TO_IMGUIZMO = {
		0.f, 0.f, 1.f, 0.f,
		1.f, 0.f, 0.f, 0.f,
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 0.f, 1.f
	};
	
	glm::mat4 v = RATCHET_TO_IMGUIZMO * g_app->render_settings.view_ratchet;
	glm::mat4 p = g_app->render_settings.projection;
	
	ImGuizmo::Manipulate(&v[0][0], &p[0][0], ImGuizmo::TRANSLATE | ImGuizmo::ROTATE | ImGuizmo::SCALE, ImGuizmo::WORLD, &gizmo_matrix[0][0]);
	
	glm::mat4 difference = gizmo_matrix;
	difference[3].x -= origin_position.x;
	difference[3].y -= origin_position.y;
	difference[3].z -= origin_position.z;
	
	glm::vec3 translation, rotation, scale;
	ImGuizmo::DecomposeMatrixToComponents(&difference[0][0], &translation[0], &rotation[0], &scale[0]);
	
	static bool was_dragging = false;
	bool is_dragging = ImGuizmo::IsUsing();
	if (!was_dragging) {
		if (!is_dragging) {
			state = TS_INACTIVE;
		} else {
			state = TS_BEGIN;
		}
	} else {
		if (is_dragging) {
			state = TS_DRAGGING;
		} else {
			state = TS_END;
		}
	}
	was_dragging = is_dragging;
	
	switch (state) {
		case TS_INACTIVE: {
			origin_position = glm::vec3(0.f, 0.f, 0.f);
			f32 count = 0.f;
			lvl.instances().for_each_with(COM_TRANSFORM, [&](Instance& inst) {
				if (inst.selected) {
					origin_position += inst.transform().pos();
					count++;
				}
			});
			if (count > 0.f) {
				origin_position /= count;
			}
			gizmo_matrix = glm::mat4(1.f);
			gizmo_matrix[3] = glm::vec4(origin_position, 1.f);
			break;
		}
		case TS_BEGIN: {
			command.instances.clear();
			lvl.instances().for_each_with(COM_TRANSFORM, [&](Instance& inst) {
				if (inst.selected) {
					GizmoTransformInfo& info = command.instances.emplace_back();
					info.id = inst.id();
					info.inst_matrix = inst.transform().matrix();
					info.old_transform = inst.transform();
				}
			});
			break;
		}
		case TS_DRAGGING: {
			for (GizmoTransformInfo& info : command.instances) {
				glm::mat4 t1 = glm::translate(glm::mat4(1.f), translation - origin_position);
				glm::mat4 s = glm::scale(glm::mat4(1.f), scale);
				glm::mat4 rz = glm::rotate(glm::mat4(1.f), rotation.z, glm::vec3(0.f, 0.f, 1.f));
				glm::mat4 ry = glm::rotate(glm::mat4(1.f), rotation.y, glm::vec3(0.f, 1.f, 0.f));
				glm::mat4 rx = glm::rotate(glm::mat4(1.f), rotation.x, glm::vec3(1.f, 0.f, 0.f));
				glm::mat4 t2 = glm::translate(glm::mat4(1.f), origin_position);
				info.inst_matrix = t2 * rx * ry * rz * s * t1 * info.old_transform.matrix();
				
				Instance* inst = lvl.instances().from_id(info.id);
				if (inst) {
					inst->is_dragging = true;
					inst->drag_preview_matrix = info.inst_matrix;
				}
			}
			break;
		}
		case TS_END: {
			push_gizmo_transform_command(lvl, command);
			command = {};
			lvl.instances().for_each([&](Instance& inst) {
				inst.is_dragging = false;
			});
			break;
		}
	}
}

static void draw() {
	if (state != TS_DRAGGING) {
		return;
	}
	
	Level* lvl = g_app->get_level();
	verify_fatal(lvl);
	
	static std::vector<InstanceId> instances;
	instances.clear();
	for (const GizmoTransformInfo& info : command.instances) {
		instances.emplace_back(info.id);
	}

	draw_drag_ghosts(*lvl, instances, g_app->render_settings);
}

static void push_gizmo_transform_command(Level& lvl, GizmoTransformCommand& command) {
	lvl.push_command<GizmoTransformCommand>(std::move(command),
		[](Level& lvl, GizmoTransformCommand& command) {
			for (const GizmoTransformInfo& info : command.instances) {
				Instance* inst = lvl.instances().from_id(info.id);
				verify_fatal(inst);
				inst->transform().set_from_matrix(&info.inst_matrix);
			}
		},
		[](Level& lvl, GizmoTransformCommand& command) {
			for (const GizmoTransformInfo& info : command.instances) {
				Instance* inst = lvl.instances().from_id(info.id);
				verify_fatal(inst);
				inst->transform() = info.old_transform;
			}
		}
	);
}
