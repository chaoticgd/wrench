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

void model_preview(
	GLuint* texture,
	const RenderMesh* mesh,
	const std::vector<RenderMaterial>* materials,
	bool wireframe,
	ModelPreviewParams& params)
{
	ImVec2 view_pos = ImGui::GetWindowPos();
	view_pos.x += ImGui::GetWindowContentRegionMin().x;
	view_pos.y += ImGui::GetWindowContentRegionMin().y;
	ImVec2 view_size = ImGui::GetWindowContentRegionMax();
	view_size.x -= ImGui::GetWindowContentRegionMin().x;
	view_size.y -= ImGui::GetWindowContentRegionMin().y;
	
	glm::mat4 matrix(1.f);
	matrix = glm::translate(matrix, params.bounding_box_origin);
	matrix = glm::scale(matrix, params.bounding_box_size * 0.5f);
	
	f32 fov_y = 45.0f;
	
	// Get the largest dimension of the model.
	f32 model_size = std::max({params.bounding_box_size.x, params.bounding_box_size.y, params.bounding_box_size.z});
	
	f32 tan = tanf(glm::radians(fov_y * 0.5f));
	f32 focal_length = (view_size.y * 0.5f) / (tan == 0.f ? 1.f : tan);
	
	// Get a ratio of how wide the largest dimension of the model is compared to the render window width.
	f32 zoom_ratio;
	if (view_size.x < view_size.y) {
		zoom_ratio = model_size / view_size.x;
	} else { // Same as above, but use height if the render window is shorter than wide.
		zoom_ratio = model_size / view_size.y;
	}
	
	// Fit the camera to the model bounding box.
	f32 camera_distance = focal_length * zoom_ratio;
	
	glm::vec3 eye = glm::vec3((camera_distance * (2.f - params.zoom * 1.9f)), 0, 0);
	glm::mat4 view_fixed = glm::lookAt(eye, glm::vec3(0), glm::vec3(0, 1, 0));
	glm::mat4 view_pitched = glm::rotate(view_fixed, params.rot.x, glm::vec3(0, 0, 1));
	glm::mat4 view_yawed = glm::rotate(view_pitched, params.rot.y, glm::vec3(0, 1, 0));
	glm::mat4 view = glm::translate(view_yawed * RATCHET_TO_OPENGL_MATRIX, -params.bounding_box_origin);
	glm::mat4 projection = compose_projection_matrix(view_size);
	
	render_to_texture(texture, view_size.x, view_size.y, [&]() {
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, view_size.x, view_size.y);
		
		if (mesh && materials) {
			draw_model_preview(*mesh, *materials, &matrix, view, projection, wireframe);
		}
	});
	
	ImGui::Image((ImTextureID) *texture, view_size);
	bool image_hovered = ImGui::IsItemHovered();
	
	static bool is_dragging = false;
	
	ImGuiIO& io = ImGui::GetIO();
	glm::vec2 mouse_delta = glm::vec2(io.MouseDelta.y, io.MouseDelta.x) * 0.01f;
	
	if (image_hovered || is_dragging) {
		if (ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
			is_dragging = true;
			params.rot += mouse_delta;
		}
		
		params.zoom *= io.MouseWheel * g_app->delta_time * 0.0001 + 1;
		if (params.zoom < 0.f) params.zoom = 0.f;
		if (params.zoom > 1.f) params.zoom = 1.f;
	}
	
	if (ImGui::IsMouseReleased(0)) {
		is_dragging = false;
	}
}
