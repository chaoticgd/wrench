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

#ifndef EDITOR_GUI_MODEL_PREVIEW_H
#define EDITOR_GUI_MODEL_PREVIEW_H

#include <gui/render_mesh.h>

struct ModelPreviewParams
{
	glm::vec2 rot = {0.f, 0.f};
	f32 zoom = 0.5f;
	f32 elevation = 0.f;
	glm::vec3 bounding_box_origin = {0.f, 0.f, 0.f};
	glm::vec3 bounding_box_size = {0.f, 0.f, 0.f};
};

void model_preview(
	GLuint* texture,
	const RenderMesh* mesh,
	const std::vector<RenderMaterial>* materials,
	bool wireframe,
	ModelPreviewParams& params);

#endif
