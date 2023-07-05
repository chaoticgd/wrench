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

#include "model_preview.h"

#include <editor/app.h>
#include <editor/renderer.h>

void model_preview(GLuint* texture, const RenderMesh* mesh, const std::vector<RenderMaterial>* materials, GLenum mesh_mode, ModelPreviewParams& params) {
	ImVec2 view_pos = ImGui::GetWindowPos();
	view_pos.x += ImGui::GetWindowContentRegionMin().x;
	view_pos.y += ImGui::GetWindowContentRegionMin().y;
	ImVec2 view_size = ImGui::GetWindowContentRegionMax();
	view_size.x -= ImGui::GetWindowContentRegionMin().x;
	view_size.y -= ImGui::GetWindowContentRegionMin().y;
	
	glm::vec3 eye = glm::vec3((12.f * (2.f - params.zoom)), 0, 0);
	glm::mat4 view_fixed = glm::lookAt(eye, glm::vec3(0), glm::vec3(0, 1, 0));
	glm::mat4 view_pitched = glm::rotate(view_fixed, params.rot.x, glm::vec3(0, 0, 1));
	glm::mat4 view_yawed = glm::rotate(view_pitched, params.rot.y, glm::vec3(0, 1, 0));
	glm::mat4 view = glm::translate(view_yawed * RATCHET_TO_OPENGL_MATRIX, -glm::vec3(0, 0, 2.f));
	glm::mat4 projection = compose_projection_matrix(view_size);
	
	render_to_texture(texture, view_size.x, view_size.y, [&]() {
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, view_size.x, view_size.y);
		
		if(mesh && materials) {
			draw_single_mesh(*mesh, *materials, view, projection, mesh_mode);
		}
	});
	
	ImGui::Image((void*) (intptr_t) *texture, view_size);
	bool image_hovered = ImGui::IsItemHovered();
	
	static bool is_dragging = false;
	
	ImGuiIO& io = ImGui::GetIO();
	glm::vec2 mouse_delta = glm::vec2(io.MouseDelta.y, io.MouseDelta.x) * 0.01f;
	
	if(image_hovered || is_dragging) {
		if(ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
			is_dragging = true;
			params.rot += mouse_delta;
		}
		
		params.zoom *= io.MouseWheel * g_app->delta_time * 0.0001 + 1;
		if(params.zoom < 0.f) params.zoom = 0.f;
		if(params.zoom > 1.f) params.zoom = 1.f;
	}
	
	if(ImGui::IsMouseReleased(0)) {
		is_dragging = false;
	}
}
