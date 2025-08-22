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

static GLuint frame_buffer_texture = 0;

static void enumerate_instances_referenced_by_selected(Instances& instances);

void view_3d()
{
	app& a = *g_app;
	
	auto lvl = a.get_level();
	if (lvl == nullptr) {
		ImGui::TextWrapped("%s", "");
		ImGui::TextWrapped("   No level open. To open a level, use the level selector in the menu bar.");
		return;
	}
	
	enumerate_instances_referenced_by_selected(lvl->instances());
	
	ImVec2* view_pos = &a.render_settings.view_pos;
	*view_pos = ImGui::GetWindowPos();
	view_pos->x += ImGui::GetWindowContentRegionMin().x;
	view_pos->y += ImGui::GetWindowContentRegionMin().y;
	ImVec2* view_size = &a.render_settings.view_size;
	*view_size = ImGui::GetWindowContentRegionMax();
	view_size->x -= ImGui::GetWindowContentRegionMin().x;
	view_size->y -= ImGui::GetWindowContentRegionMin().y;
	
	auto& cam_pos = a.render_settings.camera_position;
	auto& cam_rot = a.render_settings.camera_rotation;
	a.render_settings.view_ratchet = compose_view_matrix(cam_pos, cam_rot);
	a.render_settings.view_gl = RATCHET_TO_OPENGL_MATRIX * a.render_settings.view_ratchet;
	a.render_settings.projection = compose_projection_matrix(*view_size);
	prepare_frame(*lvl);
	
	render_to_texture(&frame_buffer_texture, view_size->x, view_size->y, [&]() {
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, view_size->x, view_size->y);
		
		draw_level(*lvl, a.render_settings.view_gl, a.render_settings.projection, a.render_settings);
		
		g_tools[g_active_tool]->funcs.draw();
	});
	
	ImGui::Image((ImTextureID) frame_buffer_texture, *view_size);
	
	g_tools[g_active_tool]->funcs.update();
}

static void enumerate_instances_referenced_by_selected(Instances& instances)
{
	instances.for_each([&](Instance& instance) {
		instance.referenced_by_selected = false;
	});
	
	for (MobyGroupInstance& group : instances.moby_groups) {
		if (group.selected) {
			for (mobylink link : group.members) {
				if (MobyInstance* inst = instances.moby_instances.from_id(link.id)) {
					inst->referenced_by_selected = true;
				}
			}
		}
	}
	
	for (TieGroupInstance& group : instances.tie_groups) {
		if (group.selected) {
			for (tielink link : group.members) {
				if (TieInstance* inst = instances.tie_instances.from_id(link.id)) {
					inst->referenced_by_selected = true;
				}
			}
		}
	}
	
	for (ShrubGroupInstance& group : instances.shrub_groups) {
		if (group.selected) {
			for (shrublink link : group.members) {
				if (ShrubInstance* inst = instances.shrub_instances.from_id(link.id)) {
					inst->referenced_by_selected = true;
				}
			}
		}
	}
	
	for (AreaInstance& area : instances.areas) {
		if (area.selected) {
			for (pathlink link : area.paths) {
				if (PathInstance* inst = instances.paths.from_id(link.id)) {
					inst->referenced_by_selected = true;
				}
			}
			for (cuboidlink link : area.cuboids) {
				if (CuboidInstance* inst = instances.cuboids.from_id(link.id)) {
					inst->referenced_by_selected = true;
				}
			}
			for (spherelink link : area.spheres) {
				if (SphereInstance* inst = instances.spheres.from_id(link.id)) {
					inst->referenced_by_selected = true;
				}
			}
			for (cylinderlink link : area.cylinders) {
				if (CylinderInstance* inst = instances.cylinders.from_id(link.id)) {
					inst->referenced_by_selected = true;
				}
			}
			for (cuboidlink link : area.negative_cuboids) {
				if (CuboidInstance* inst = instances.cuboids.from_id(link.id)) {
					inst->referenced_by_selected = true;
				}
			}
		}
	}
}
