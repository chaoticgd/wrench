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

void view_3d() {
	app& a = *g_app;
	
	auto lvl = a.get_level();
	if(lvl == nullptr) {
		ImGui::TextWrapped("%s", "");
		ImGui::TextWrapped("   No level open. To open a level, use the 'Tree' menu.");
		return;
	}

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
	glm::mat4 world_to_clip = compose_world_to_clip(*view_size, cam_pos, cam_rot);
	prepare_frame(*lvl, world_to_clip);
	
	render_to_texture(&frame_buffer_texture, view_size->x, view_size->y, [&]() {
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, view_size->x, view_size->y);
		
		draw_level(*lvl, world_to_clip, a.render_settings);
		
		ImGuiContext& g = *GImGui;
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, g.Style.WindowPadding * 2.0f);
		a.active_tool().draw(a, world_to_clip);
		ImGui::PopStyleVar();
	});
	
	ImGui::Image((void*) (intptr_t) frame_buffer_texture, *view_size);
}
