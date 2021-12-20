/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019 chaoticgd

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

#include "mesh.h"

static u32* depalletise_texture(const Texture& texture);

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

RenderMaterial upload_material(const Material& material, const std::vector<Texture>& textures) {
	RenderMaterial rm;
	if(material.colour.has_value()) {
		rm.colour = *material.colour;
	}
	if(material.texture) {
		const Texture& texture = textures.at(*material.texture);
		u32* data = depalletise_texture(texture);
		glGenTextures(1, &rm.texture.id);
		glBindTexture(GL_TEXTURE_2D, rm.texture.id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texture.width, texture.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}
	return rm;
}

std::vector<RenderMaterial> upload_materials(const std::vector<Material>& materials, const std::vector<Texture>& textures) {
	std::vector<RenderMaterial> rms;
	for(const Material& material : materials) {
		rms.emplace_back(upload_material(material, textures));
	}
	return rms;
}

static u32* depalletise_texture(const Texture& texture) {
	static std::vector<u32> image;
	image.resize(texture.width * texture.height);
	for(s32 y = 0; y < texture.height; y++) {
		for(s32 x = 0; x < texture.width; x++) {
			u8 index = texture.pixels[y * texture.width + x];
			image[(y * texture.width + x)] = texture.palette.colours[index];
		}
	}
	return image.data();
}
