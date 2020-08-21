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
#include "imgui_includes.h" // HSV stuff.

void gl_renderer::prepare_frame(level& lvl, glm::mat4 world_to_clip) {
	moby_local_to_clip_cache.resize(lvl.mobies.size());
	for(std::size_t i = 0; i < lvl.mobies.size(); i++) {
		moby_entity& moby = lvl.mobies[i];
		moby.local_to_world_cache = glm::translate(glm::mat4(1.f), moby.position);
		moby.local_to_world_cache = glm::rotate(moby.local_to_world_cache, moby.rotation.x, glm::vec3(1, 0, 0));
		moby.local_to_world_cache = glm::rotate(moby.local_to_world_cache, moby.rotation.y, glm::vec3(0, 1, 0));
		moby.local_to_world_cache = glm::rotate(moby.local_to_world_cache, moby.rotation.z, glm::vec3(0, 0, 1));
		
		moby.local_to_clip_cache = world_to_clip * moby.local_to_world_cache;
		
		glm::mat4& local_to_clip = moby_local_to_clip_cache[i];
		local_to_clip = moby.local_to_clip_cache;
		if(lvl.moby_class_to_model.find(moby.class_num) != lvl.moby_class_to_model.end()) {
			moby_model& model = lvl.moby_models[lvl.moby_class_to_model.at(moby.class_num)];
			local_to_clip = glm::scale(local_to_clip, glm::vec3(model.scale * moby.scale * 32.f));
		}
	}
}

void gl_renderer::draw_spline(spline_entity& spline, const glm::mat4& world_to_clip, const glm::vec4& colour) const{
	
	glUniformMatrix4fv(shaders.solid_colour_transform, 1, GL_FALSE, &world_to_clip[0][0]);
	glUniform4f(shaders.solid_colour_rgb, colour.r, colour.g, colour.b, colour.a);

	GLuint vertex_buffer;
	glGenBuffers(1, &vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER,
		spline.vertices.size() * sizeof(glm::vec4),
		spline.vertices.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), nullptr);

	glDrawArrays(GL_LINE_STRIP, 0, spline.vertices.size());

	glDisableVertexAttribArray(0);
	glDeleteBuffers(1, &vertex_buffer);

}

void gl_renderer::draw_tris(const std::vector<float>& vertex_data, const glm::mat4& mvp, const glm::vec4& colour) const {
	glUniformMatrix4fv(shaders.solid_colour_transform, 1, GL_FALSE, &mvp[0][0]);
	glUniform4f(shaders.solid_colour_rgb, colour.r, colour.g, colour.b, colour.a);

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

void gl_renderer::draw_model(const model& mdl, const glm::mat4& mvp, const glm::vec4& colour) const {
	glUniformMatrix4fv(shaders.solid_colour_transform, 1, GL_FALSE, &mvp[0][0]);
	glUniform4f(shaders.solid_colour_rgb, colour.r, colour.g, colour.b, colour.a);
	
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, mdl.vertex_buffer());
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	glDrawArrays(GL_TRIANGLES, 0, mdl.vertex_buffer_size() / 3);

	glDisableVertexAttribArray(0);
}

void gl_renderer::draw_cube(const glm::mat4& mvp, const glm::vec4& colour) const {
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
	glUniform4f(shaders.solid_colour_rgb, colour.r, colour.g, colour.b, colour.a);
	
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	glDrawArrays(GL_TRIANGLES, 0, 108);

	glDisableVertexAttribArray(0);
}

void gl_renderer::draw_moby_models(
		moby_model& model,
		std::vector<texture>& textures,
		view_mode mode,
		bool show_all_submodels,
		GLuint local_to_world_buffer,
		std::size_t instance_offset,
		std::size_t count) const {
	switch(mode) {
		case view_mode::WIREFRAME:
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			glUseProgram(shaders.solid_colour_batch.id());
			break;
		case view_mode::TEXTURED_POLYGONS:
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			glUseProgram(shaders.textured.id());
			break;
	}
	
	glBindBuffer(GL_ARRAY_BUFFER, local_to_world_buffer);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*) (instance_offset + sizeof(glm::vec4) * 0));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*) (instance_offset + sizeof(glm::vec4) * 1));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*) (instance_offset + sizeof(glm::vec4) * 2));
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*) (instance_offset + sizeof(glm::vec4) * 3));
	
	glVertexAttribDivisor(0, 1);
	glVertexAttribDivisor(1, 1);
	glVertexAttribDivisor(2, 1);
	glVertexAttribDivisor(3, 1);
	
	moby_model_texture_data texture_data = {};
	for(std::size_t i = 0; i < model.submodels.size(); i++) {
		moby_submodel& submodel = model.submodels[i];
		if(!show_all_submodels && !submodel.visible_in_model_viewer) {
			continue;
		}
		
		if(submodel.vertices.size() == 0) {
			continue;
		}
		
		if(submodel.vertex_buffer() == 0) {
			glGenBuffers(1, &submodel.vertex_buffer());
			glBindBuffer(GL_ARRAY_BUFFER, submodel.vertex_buffer());
			glBufferData(GL_ARRAY_BUFFER,
				submodel.vertices.size() * sizeof(moby_model_vertex),
				submodel.vertices.data(), GL_STATIC_DRAW);
		}
		
		if(submodel.st_buffer() == 0) {
			glGenBuffers(1, &submodel.st_buffer());
			glBindBuffer(GL_ARRAY_BUFFER, submodel.st_buffer());
			glBufferData(GL_ARRAY_BUFFER,
				submodel.st_coords.size() * sizeof(moby_model_st),
				submodel.st_coords.data(), GL_STATIC_DRAW);
		}
		
		for(moby_subsubmodel& subsubmodel : submodel.subsubmodels) {
			if(subsubmodel.index_buffer() == 0) {
				glGenBuffers(1, &subsubmodel.index_buffer());
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, subsubmodel.index_buffer());
				glBufferData(GL_ELEMENT_ARRAY_BUFFER,
					subsubmodel.indices.size(),
					subsubmodel.indices.data(), GL_STATIC_DRAW);
			}
			
			if(subsubmodel.texture) {
				texture_data = *subsubmodel.texture;
			}
			
			switch(mode) {
				case view_mode::WIREFRAME: {
					glm::vec4 colour = colour_coded_submodel_index(i, model.submodels.size());
					glUniform4f(shaders.solid_colour_batch_rgb, colour.r, colour.g, colour.b, colour.a);
					break;
				}
				case view_mode::TEXTURED_POLYGONS: {
					if(model.texture_indices.size() > (std::size_t) texture_data.texture_index) {
						texture& tex = textures.at(model.texture_indices.at(texture_data.texture_index));
						if(tex.opengl_id() == 0) {
							tex.upload_to_opengl();
						}
						
						glActiveTexture(GL_TEXTURE0);
						glBindTexture(GL_TEXTURE_2D, tex.opengl_id());
					} else {
						// TODO: Actually fix this so model textures get read in
						// correctly. This warning was commented out because it
						// was spamming stderr.
						//fprintf(stderr, "warning: Model %s has bad texture index!\n", model.name().c_str());
					}
					glUniform1i(shaders.textured_sampler, 0);
					break;
				}
			}
			
			glEnableVertexAttribArray(4);
			glBindBuffer(GL_ARRAY_BUFFER, submodel.vertex_buffer());
			glVertexAttribPointer(4, 3, GL_SHORT, GL_TRUE, sizeof(moby_model_vertex), (void*) offsetof(moby_model_vertex, x));
			
			glEnableVertexAttribArray(5);
			glBindBuffer(GL_ARRAY_BUFFER, submodel.st_buffer());
			glVertexAttribPointer(5, 2, GL_SHORT, GL_TRUE, sizeof(moby_model_st), (void*) offsetof(moby_model_st, s));
			
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, subsubmodel.index_buffer());
			glDrawElementsInstanced(
				GL_TRIANGLES,
				subsubmodel.indices.size(),
				GL_UNSIGNED_BYTE,
				nullptr,
				count);
			
			glDisableVertexAttribArray(4);
			glDisableVertexAttribArray(5);
		}
	}
	
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);
	
	glVertexAttribDivisor(0, 0);
	glVertexAttribDivisor(1, 0);
	glVertexAttribDivisor(2, 0);
	glVertexAttribDivisor(3, 0);
}

glm::vec4 gl_renderer::colour_coded_submodel_index(std::size_t index, std::size_t submodel_count) {
	glm::vec4 colour;
	ImGui::ColorConvertHSVtoRGB(fmod(index / (float) submodel_count, 1.f), 1.f, 1.f, colour.r, colour.g, colour.b);
	colour.a = 1;
	return colour;
}

void gl_renderer::reset_camera(app* a) {
	camera_rotation = glm::vec3(0, 0, 0);
	if(level* lvl = a->get_level()) {
		if(lvl->mobies.size() > 0) {
			camera_position = lvl->mobies[0].position;
		} else {
			camera_position = lvl->properties.ship_position();
		}
	} else {
		camera_position = glm::vec3(0, 0, 0);
	}
}
