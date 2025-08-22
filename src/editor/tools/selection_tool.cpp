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

static void activate();
static void deactivate();
static void update();
static void draw();

ToolInfo g_selection_tool_info = {
	"Selection Tool",
	{
		&activate,
		&deactivate,
		&update,
		&draw
	}
};

static bool selecting;
static ImVec2 selection_begin;

static void activate()
{
	
}

static void deactivate()
{
	
}

static void update()
{
	if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered()) {
		selecting = true;
		selection_begin = ImGui::GetMousePos();
	}
	
	if (selecting) {
		auto draw_list = ImGui::GetWindowDrawList();
		draw_list->AddRect(selection_begin, ImGui::GetMousePos(), 0xffffffff);
	}
	
	if (ImGui::IsMouseReleased(0) && selecting) {
		selecting = false;
		
		ImVec2 p1 = selection_begin;
		ImVec2 p2 = ImGui::GetMousePos();
		if (p1.x > p2.x) {
			std::swap(p1.x, p2.x);
		}
		if (p1.y > p2.y) {
			std::swap(p1.y, p2.y);
		}
		p1.y -= 20;
		p2.y -= 20;
		
		auto in_bounds = [&](glm::vec3 screen_pos) {
			return screen_pos.z > 0 &&
				(screen_pos.x > p1.x && screen_pos.x < p2.x) &&
				(screen_pos.y > p1.y && screen_pos.y < p2.y);
		};
		
		glm::mat4 world_to_clip = g_app->render_settings.projection * g_app->render_settings.view_gl;
		
		Level& lvl = *g_app->get_level();
		lvl.instances().for_each_with(COM_TRANSFORM, [&](Instance& inst) {
			glm::vec3 screen_pos = apply_local_to_screen(world_to_clip, inst.transform().matrix(), g_app->render_settings.view_size);
			inst.selected = in_bounds(screen_pos);
		});
	}
}

static void draw() {
	
}
