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

#include "render_mesh.h"

RenderMesh upload_mesh(const Mesh& mesh, bool generate_normals)
{
	RenderMesh render_mesh;
	
	for (const SubMesh& submesh : mesh.submeshes) {
		RenderSubMesh render_submesh;
		
		render_submesh.material = submesh.material;
		
		std::vector<Vertex> vertices;
		for (const Face& face : submesh.faces) {
			if (face.is_quad()) {
				Vertex v0 = mesh.vertices[face.v0];
				Vertex v1 = mesh.vertices[face.v1];
				Vertex v2 = mesh.vertices[face.v2];
				Vertex v3 = mesh.vertices[face.v3];
				
				if (generate_normals) {
					glm::vec3 normal = glm::normalize(glm::cross(v2.pos - v0.pos, v1.pos - v0.pos));
					v0.normal = normal;
					v1.normal = normal;
					v2.normal = normal;
					v3.normal = normal;
				}
				
				vertices.push_back(v0);
				vertices.push_back(v1);
				vertices.push_back(v2);
				vertices.push_back(v2);
				vertices.push_back(v3);
				vertices.push_back(v0);
			} else {
				Vertex v0 = mesh.vertices[face.v0];
				Vertex v1 = mesh.vertices[face.v1];
				Vertex v2 = mesh.vertices[face.v2];
				
				if (generate_normals) {
					glm::vec3 normal = glm::normalize(glm::cross(v2.pos - v0.pos, v1.pos - v0.pos));
					v0.normal = normal;
					v1.normal = normal;
					v2.normal = normal;
				}
				
				vertices.push_back(v0);
				vertices.push_back(v1);
				vertices.push_back(v2);
			}
		}
		
		glGenBuffers(1, &render_submesh.vertex_buffer.id);
		glBindBuffer(GL_ARRAY_BUFFER, render_submesh.vertex_buffer.id);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
		render_submesh.vertex_count = vertices.size();
		
		render_mesh.submeshes.emplace_back(std::move(render_submesh));
	}
	
	return render_mesh;
}

RenderMesh upload_gltf_mesh(const GLTF::Mesh& mesh, bool generate_normals)
{
	RenderMesh render_mesh;
	
	for (const GLTF::MeshPrimitive& primitive : mesh.primitives) {
		RenderSubMesh render_submesh;
		
		render_submesh.material = primitive.material.has_value() ? *primitive.material : -1;
		
		std::vector<Vertex> vertices;
		switch (primitive.mode.has_value() ? *primitive.mode : GLTF::TRIANGLES) {
			case GLTF::TRIANGLES: {
				for (size_t i = 0; i < primitive.indices.size() / 3; i++) {
					vertices.push_back(mesh.vertices.at(primitive.indices.at(i * 3 + 0)));
					vertices.push_back(mesh.vertices.at(primitive.indices.at(i * 3 + 1)));
					vertices.push_back(mesh.vertices.at(primitive.indices.at(i * 3 + 2)));
				}
				break;
			}
			case GLTF::TRIANGLE_STRIP: {
				if (primitive.indices.size() < 3) {
					break;
				}
				for (size_t i = 0; i < primitive.indices.size() - 2; i++) {
					vertices.push_back(mesh.vertices.at(primitive.indices.at(i + 0)));
					vertices.push_back(mesh.vertices.at(primitive.indices.at(i + 1)));
					vertices.push_back(mesh.vertices.at(primitive.indices.at(i + 2)));
				}
				break;
			}
			default: {
			}
		}
		
		if (generate_normals) {
			for (size_t i = 0; i < vertices.size() / 3; i++) {
				Vertex& v0 = vertices[i * 3 + 0];
				Vertex& v1 = vertices[i * 3 + 1];
				Vertex& v2 = vertices[i * 3 + 2];
				
				glm::vec3 normal = glm::normalize(glm::cross(v2.pos - v0.pos, v1.pos - v0.pos));
				v0.normal = normal;
				v1.normal = normal;
				v2.normal = normal;
			}
		}
		
		glGenBuffers(1, &render_submesh.vertex_buffer.id);
		glBindBuffer(GL_ARRAY_BUFFER, render_submesh.vertex_buffer.id);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
		render_submesh.vertex_count = vertices.size();
		
		render_mesh.submeshes.emplace_back(std::move(render_submesh));
	}
	
	return render_mesh;
}

std::vector<RenderMaterial> upload_collada_materials(
	const std::vector<ColladaMaterial>& materials, const std::vector<Texture>& textures)
{
	std::vector<RenderMaterial> rms;
	for (const ColladaMaterial& material : materials) {
		rms.emplace_back(upload_collada_material(material, textures));
	}
	return rms;
}

std::vector<RenderMaterial> upload_materials(
	const std::vector<Material>& materials, const std::vector<Texture>& textures)
{
	std::vector<RenderMaterial> render_materials;
	for (const Material& material : materials) {
		render_materials.emplace_back(upload_material(material, textures));
	}
	return render_materials;
}

RenderMaterial upload_collada_material(
	const ColladaMaterial& material, const std::vector<Texture>& textures)
{
	RenderMaterial rm;
	s32 texture_index;
	if (material.surface.type == MaterialSurfaceType::COLOUR) {
		rm.colour = material.surface.colour;
		texture_index = 0;
	} else {
		texture_index = material.surface.texture;
	}
	if (texture_index < textures.size()) {
		Texture texture = textures.at(texture_index);
		texture.to_rgba();
		glGenTextures(1, &rm.texture.id);
		glBindTexture(GL_TEXTURE_2D, rm.texture.id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texture.width, texture.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture.data.data());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}
	return rm;
}

RenderMaterial upload_material(const Material& material, const std::vector<Texture>& textures)
{
	RenderMaterial render_material;
	s32 texture_index;
	if (material.surface.type == MaterialSurfaceType::COLOUR) {
		render_material.colour = material.surface.colour;
		texture_index = 0;
	} else {
		texture_index = material.surface.texture;
	}
	if (texture_index < textures.size()) {
		Texture texture = textures.at(texture_index);
		texture.to_rgba();
		glGenTextures(1, &render_material.texture.id);
		glBindTexture(GL_TEXTURE_2D, render_material.texture.id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texture.width, texture.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture.data.data());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}
	return render_material;
}
