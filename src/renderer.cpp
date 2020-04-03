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

#include "app.h"

void gl_renderer::draw_spline(const std::vector<glm::vec3>& points, glm::mat4 vp, glm::vec3& colour) const {
	
	if(points.size() < 1) {
		return;
	}
	
	std::vector<float> vertex_data;
	for(auto iter = points.begin(); iter != points.end()-1; iter++) {
		vertex_data.push_back((*iter).x);
		vertex_data.push_back((*iter).y);
		vertex_data.push_back((*iter).z);
		
		vertex_data.push_back((*iter).x + 0.5);
		vertex_data.push_back((*iter).y);
		vertex_data.push_back((*iter).z);
		
		vertex_data.push_back((*(iter+1)).x);
		vertex_data.push_back((*(iter+1)).y);
		vertex_data.push_back((*(iter+1)).z);
	}
	draw_tris(vertex_data, vp, colour);
}

void gl_renderer::draw_tris(const std::vector<float>& vertex_data, glm::mat4 mvp, glm::vec3& colour) const {
	glUniformMatrix4fv(shaders.solid_colour_transform, 1, GL_FALSE, &mvp[0][0]);
	glUniform4f(shaders.solid_colour_rgb, colour.x, colour.y, colour.z, 1);

	GLuint vertex_buffer;
	glGenBuffers(1, &vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER,
		vertex_data.size() * sizeof(float),
		vertex_data.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	glDrawArrays(GL_TRIANGLES, 0, vertex_data.size() * 3);

	glDisableVertexAttribArray(0);
	glDeleteBuffers(1, &vertex_buffer);
}

void gl_renderer::draw_model(const game_model& mdl, glm::mat4 mvp, glm::vec3 colour) const {
	glUniformMatrix4fv(shaders.solid_colour_transform, 1, GL_FALSE, &mvp[0][0]);
	glUniform4f(shaders.solid_colour_rgb, colour.x, colour.y, colour.z, 1);
	
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, mdl.vertex_buffer());
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	glDrawArrays(GL_TRIANGLES, 0, mdl.vertex_buffer_size() * 3);

	glDisableVertexAttribArray(0);
}

void gl_renderer::draw_cube(glm::mat4 mvp, glm::vec3 colour) const {
	static GLuint vertex_buffer = 0;
	
	if(vertex_buffer == 0) {
		const static std::vector<float> vertex_data {
			-1, -1, -1, -1, -1,  1, -1,  1,  1,
			 1,  1, -1, -1, -1, -1, -1,  1, -1,
			 1, -1,  1, -1, -1, -1,  1, -1, -1,
			 1,  1, -1,  1, -1, -1, -1, -1, -1,
			-1, -1, -1, -1,  1,  1, -1,  1, -1,
			 1, -1,  1, -1, -1,  1, -1, -1, -1,
			-1,  1,  1, -1, -1,  1,  1, -1,  1,
			 1,  1,  1,  1, -1, -1,  1,  1, -1,
			 1, -1, -1,  1,  1,  1,  1, -1,  1,
			 1,  1,  1,  1,  1, -1, -1,  1, -1,
			 1,  1,  1, -1,  1, -1, -1,  1,  1,
			 1,  1,  1, -1,  1,  1,  1, -1,  1
		};
		
		glGenBuffers(1, &vertex_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
		glBufferData(GL_ARRAY_BUFFER,
			108 * sizeof(float),
			vertex_data.data(), GL_STATIC_DRAW);
	}
	
	glUniformMatrix4fv(shaders.solid_colour_transform, 1, GL_FALSE, &mvp[0][0]);
	glUniform4f(shaders.solid_colour_rgb, colour.x, colour.y, colour.z, 1);
	
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	glDrawArrays(GL_TRIANGLES, 0, 108);

	glDisableVertexAttribArray(0);
}

void gl_renderer::reset_camera(app* a) {
	camera_rotation = glm::vec3(0, 0, 0);
	auto lvl = a->get_level();
	if(lvl != nullptr && lvl->world.count<moby>() >= 1) {
		camera_position = lvl->world.object_at<moby>(0).position();
	} else {
		camera_position = glm::vec3(0, 0, 0);
	}
}
