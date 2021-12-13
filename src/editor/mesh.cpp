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

RenderMesh upload_mesh(const Mesh& mesh) {
	RenderMesh render_mesh;
	
	glGenBuffers(1, &render_mesh.vertex_buffer.id);
	glBindBuffer(GL_ARRAY_BUFFER, render_mesh.vertex_buffer.id);
	glBufferData(GL_ARRAY_BUFFER,
		mesh.vertices.size() * sizeof(Vertex),
		mesh.vertices.data(), GL_STATIC_DRAW);
	
	for(const SubMesh& submesh : mesh.submeshes) {
		RenderSubMesh render_submesh;
		
		std::vector<s32> indices;
		for(const Face& face : submesh.faces) {
			indices.push_back(face.v0);
			indices.push_back(face.v1);
			indices.push_back(face.v2);
			render_submesh.index_count += 3;
			if(face.is_quad()) {
				indices.push_back(face.v1);
				indices.push_back(face.v2);
				indices.push_back(face.v3);
				render_submesh.index_count += 3;
			}
		}
		
		glGenBuffers(1, &render_submesh.index_buffer.id);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, render_submesh.index_buffer.id);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(s32), indices.data(), GL_STATIC_DRAW);
		
		render_mesh.submeshes.emplace_back(std::move(render_submesh));
	}
	
	return render_mesh;
}

void setup_vertex_attributes(GLuint pos, GLuint normal, GLuint tex_coord) {
	glEnableVertexAttribArray(pos);
	glVertexAttribPointer(pos, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, pos));
	glEnableVertexAttribArray(normal);
	glVertexAttribPointer(normal, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, normal));
	glEnableVertexAttribArray(tex_coord);
	glVertexAttribPointer(tex_coord, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, tex_coord));
}
