/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2022 chaoticgd

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

#ifndef GUI_RENDER_MESH_H
#define GUI_RENDER_MESH_H

#include <vector>

#include <core/gltf.h>
#include <core/mesh.h>
#include <core/collada.h>
#include <core/texture.h>
#include <gui/gui.h>

struct RenderSubMesh
{
	GLuint material;
	GlBuffer vertex_buffer;
	s32 vertex_count = 0;
};

struct RenderMaterial
{
	glm::vec4 colour{1.f, 1.f, 1.f, 1.f};
	GlTexture texture;
};

struct RenderMesh
{
	std::vector<RenderSubMesh> submeshes;
};

RenderMesh upload_mesh(const Mesh& mesh, bool generate_normals);
RenderMesh upload_gltf_mesh(const GLTF::Mesh& mesh, bool generate_normals);
std::vector<RenderMaterial> upload_collada_materials(
	const std::vector<ColladaMaterial>& materials, const std::vector<Texture>& textures);
std::vector<RenderMaterial> upload_materials(
	const std::vector<Material>& materials, const std::vector<Texture>& textures);
RenderMaterial upload_collada_material(
	const ColladaMaterial& material, const std::vector<Texture>& textures);
RenderMaterial upload_material(const Material& material, const std::vector<Texture>& textures);

#endif
