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
static void pick_object(const glm::mat4& view, const glm::mat4& projection, ImVec2 position);

ToolInfo g_picker_tool_info = {
	"Picker Tool",
	{
		&activate,
		&deactivate,
		&update,
		&draw
	}
};

static void activate()
{
	
}

static void deactivate()
{
	
}

static void update()
{
	if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered()) {
		ImVec2 rel_pos {
			ImGui::GetMousePos().x - ImGui::GetWindowPos().x,
			ImGui::GetMousePos().y - ImGui::GetWindowPos().y - 20
		};
		pick_object(g_app->render_settings.view_gl, g_app->render_settings.projection, rel_pos);
	}
}

static void draw()
{
	
}

// Allows the user to select an object by clicking on it. See:
// https://www.opengl-tutorial.org/miscellaneous/clicking-on-objects/picking-with-an-opengl-hack/
static void pick_object(const glm::mat4& view, const glm::mat4& projection, ImVec2 position)
{
	Level& lvl = *g_app->get_level();
	
	GLint last_framebuffer;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &last_framebuffer);
	
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	draw_pickframe(lvl, view, projection, g_app->render_settings);
	
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
	for (int i = 0; i < size; i++) {
		if (buffer[i] > 0) {
			auto current_value = glm::abs(middle - i % select_size) + glm::abs(middle - i / select_size);
			if (current_value < smallest_value) {
				smallest_index = i;
				smallest_value = current_value;
			}
		}
	}

	if (smallest_value == -1) {
		lvl.instances().clear_selection();
		return;
	}
	
	bool is_multi_selecting = ImGui::GetIO().KeyCtrl;

	u8 picked_type = buffer[smallest_index] & 0xff;
	u16 picked_id = (buffer[smallest_index] & 0x00ffff00) >> 8;
	lvl.instances().for_each([&](Instance& inst) {
		bool same_type = inst.id().type == picked_type;
		bool same_id_value = inst.id().value == picked_id;
		if (is_multi_selecting) {
			inst.selected ^= same_type && same_id_value;
		} else {
			inst.selected = same_type && same_id_value;
		}
	});
	
	glBindFramebuffer(GL_FRAMEBUFFER, last_framebuffer);
}
