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

RenderMesh upload_mesh(const Mesh& mesh, bool generate_normals) {
	RenderMesh render_mesh;
	
	for(const SubMesh& submesh : mesh.submeshes) {
		RenderSubMesh render_submesh;
		
		render_submesh.material = submesh.material;
		
		std::vector<Vertex> vertices;
		for(const Face& face : submesh.faces) {
			if(face.is_quad()) {
				Vertex v0 = mesh.vertices[face.v0];
				Vertex v1 = mesh.vertices[face.v1];
				Vertex v2 = mesh.vertices[face.v2];
				Vertex v3 = mesh.vertices[face.v3];
				
				if(generate_normals) {
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
				
				if(generate_normals) {
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

RenderMaterial upload_material(const ColladaMaterial& material, const std::vector<Texture>& textures) {
	RenderMaterial rm;
	if(material.surface.type == MaterialSurfaceType::COLOUR) {
		rm.colour = material.surface.colour;
	}
	if(material.surface.type == MaterialSurfaceType::TEXTURE && material.surface.texture < textures.size()) {
		Texture texture = textures[material.surface.texture];
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

std::vector<RenderMaterial> upload_materials(const std::vector<ColladaMaterial>& materials, const std::vector<Texture>& textures) {
	std::vector<RenderMaterial> rms;
	for(const ColladaMaterial& material : materials) {
		rms.emplace_back(upload_material(material, textures));
	}
	return rms;
}
