#include "renderer.h"

using vertex_list = std::vector<std::array<glm::vec3, 3>>;

void draw_current_level(const app& a, shader_programs& shaders) {
	if(!a.has_level()) {
		return;
	}
	const level& lvl = a.read_level();

	glm::mat4 projection = glm::perspective(glm::radians(45.0f), 4.0f / 3.0f, 0.1f, 100.0f);

	auto rot = lvl.camera_rotation;
	glm::mat4 view = glm::yawPitchRoll(rot.y, rot.x, rot.z);
	view = glm::translate(view, lvl.camera_position);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	for(auto& moby : a.read_level().mobies()) {
		glm::vec3 pos = moby.second->position() / 10.0f;
		std::swap(pos.y, pos.z);
		glm::mat4 model = glm::translate(glm::mat4(1.f), pos);
		draw_test_tri(shaders, projection * view * model, glm::vec3(0, 1, 0));
	}

	for(int i = -10 ; i < 10; i++) {
		for(int j = -10; j < 10; j++) {
			for(int k = -10; k < 10; k++) {
				glm::mat4 model = glm::translate(glm::mat4(1.f), glm::vec3(i, j, k) * 10.0f);
				glm::vec3 colour((i + 10.0) / 20.0, (j + 10.0) / 20.0, (k + 10.0) / 20.0);
				draw_test_tri(shaders, projection * view * model, colour);
			}
		}
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
