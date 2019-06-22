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

#include "renderer.h"

glm::vec3 level_to_world(glm::vec3 point) {
	return glm::vec3(point.x, point.z, point.y);
}

void draw_current_level(const app& a, shader_programs& shaders) {
	if(!a.has_level()) {
		return;
	}
	const level& lvl = a.read_level();

	glm::mat4 projection = glm::perspective(glm::radians(45.0f), 4.0f / 3.0f, 0.1f, 100.0f);

	auto rot = lvl.camera_rotation;
	glm::mat4 pitch = glm::rotate(glm::mat4(1.0f), rot.x, glm::vec3(1.0f, 0.0f, 0.0f));
	glm::mat4 yaw   = glm::rotate(glm::mat4(1.0f), rot.y, glm::vec3(0.0f, 1.0f, 0.0f));
 
	glm::mat4 translate =
		glm::translate(glm::mat4(1.0f), level_to_world(-lvl.camera_position));
	glm::mat4 view = pitch * yaw * translate;

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	for(auto& [uid, moby] : a.read_level().mobies()) {
		glm::vec3 pos = level_to_world(moby->position());
		glm::mat4 model = glm::translate(glm::mat4(1.f), pos);
		glm::vec3 colour =
			lvl.is_selected(uid) ? glm::vec3(1, 0, 0) : glm::vec3(0, 1, 0);
		draw_test_tri(shaders, projection * view * model, colour);
	}
}

void draw_test_tri(shader_programs& shaders, glm::mat4 mvp, glm::vec3 colour) {

	using v = glm::vec3;
	static const vertex_list vertex_data {
		{ v(1, 1, 0), v(1, 0, 1), v(0, 1, 0) }
	};
	
	glUseProgram(shaders.solid_colour.id());

	glUniformMatrix4fv(shaders.solid_colour_transform, 1, GL_FALSE, &mvp[0][0]);
	glUniform3f(shaders.solid_colour_rgb, colour.x, colour.y, colour.z);

	GLuint vertex_buffer;
	glGenBuffers(1, &vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER,
		vertex_data.size() * sizeof(vertex_list::value_type),
		&vertex_data[0][0].x, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	glDrawArrays(GL_TRIANGLES, 0, 3);

	glDisableVertexAttribArray(0);
	glDeleteBuffers(1, &vertex_buffer);
}
