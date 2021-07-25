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

void gl_renderer::prepare_frame(level& lvl, glm::mat4 world_to_clip) {
	moby_matrices.resize(opt_size(lvl.gameplay().moby_instances));
	size_t i = 0;
	for(MobyInstance& inst : opt_iterator(lvl.gameplay().moby_instances)) {
		moby_matrices[i++] = world_to_clip * inst.matrix();
	}
	
	if(lvl.gameplay().shrub_instances.has_value()) {
		shrub_matrices.resize(lvl.gameplay().shrub_instances->size());
		//for (std::size_t i = 0; i < lvl.world.shrubs.size(); i++) {
		//	shrub_entity& shrub = lvl.world.shrubs[i];
		//
		//	glm::mat4& local_to_clip = shrub_local_to_clip_cache[i];
		//	local_to_clip = world_to_clip * shrub.local_to_world;
		//	if (lvl.shrub_class_to_model.find(shrub.o_class) != lvl.shrub_class_to_model.end()) {
		//		shrub_model& model = lvl.shrub_models[lvl.shrub_class_to_model.at(shrub.o_class)];
		//		local_to_clip = glm::scale(local_to_clip, glm::vec3(model.scale * 32.f));
		//	}
		//}
	}
}

void gl_renderer::draw_level(level& lvl, glm::mat4 world_to_clip) const {
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glUseProgram(shaders.solid_colour.id());
	
	static const glm::vec4 selected_colour = glm::vec4(1, 0, 0, 1);
	
	auto get_colour = [&](bool selected, glm::vec4 normal_colour) {
		return selected ? selected_colour : normal_colour;
	};
	
	if(draw_ties) {
		for(TieInstance& inst : opt_iterator(lvl.gameplay().tie_instances)) {
			glm::mat4 local_to_clip = world_to_clip * inst.matrix();
			glm::vec4 colour = get_colour(inst.selected, glm::vec4(0.5, 0, 1, 1));
			draw_cube(local_to_clip, colour);
		}
	}
	
	if(draw_shrubs && lvl.gameplay().shrub_instances.has_value()) {
		gl_buffer shrub_matrix_buffer;
		glGenBuffers(1, &shrub_matrix_buffer());
		glBindBuffer(GL_ARRAY_BUFFER, shrub_matrix_buffer());
		glBufferData(GL_ARRAY_BUFFER,
			shrub_matrices.size() * sizeof(glm::mat4),
			shrub_matrices.data(), GL_STATIC_DRAW);

		std::size_t shrub_batch_class = INT64_MAX;
		std::size_t shrub_batch_begin = 0;

		auto draw_shrub_batch = [&](std::size_t batch_end) {
			if (lvl.shrub_class_to_model.find(shrub_batch_class) != lvl.shrub_class_to_model.end()) {
				std::size_t model_index = lvl.shrub_class_to_model.at(shrub_batch_class);
				shrub_model& model = lvl.shrub_models[model_index];
				draw_shrub_models(
					model,
					lvl.shrub_textures,
					mode,
					true,
					shrub_matrix_buffer(),
					shrub_batch_begin * sizeof(glm::mat4),
					batch_end - shrub_batch_begin);
			}
			else {
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				glUseProgram(shaders.solid_colour.id());
				
				for (std::size_t i = shrub_batch_begin; i < batch_end; i++) {
					const glm::mat4& local_to_clip = shrub_matrices[i];
					glm::vec4 colour = get_colour((*lvl.gameplay().shrub_instances)[i].selected, glm::vec4(0, 1, 0, 1));
					draw_cube(local_to_clip, colour);
				}
			}
		};
		
		for(std::size_t i = 0; i < lvl.gameplay().shrub_instances->size(); i++) {
			ShrubInstance& inst = (*lvl.gameplay().shrub_instances)[i];
			if (inst.o_class != shrub_batch_class) {
				draw_shrub_batch(i);
				shrub_batch_class = inst.o_class;
				shrub_batch_begin = i;
			}
		}
		draw_shrub_batch(lvl.gameplay().shrub_instances->size());
		
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glUseProgram(shaders.solid_colour.id());
		
		for (std::size_t i = 0; i < lvl.gameplay().shrub_instances->size(); i++) {
			if ((*lvl.gameplay().shrub_instances)[i].selected) {
				draw_cube(shrub_matrices[i], selected_colour);
			}
		}
	}
	
	if(draw_mobies && lvl.gameplay().moby_instances.has_value()) {
		gl_buffer moby_matrix_buffer;
		glGenBuffers(1, &moby_matrix_buffer());
		glBindBuffer(GL_ARRAY_BUFFER, moby_matrix_buffer());
		glBufferData(GL_ARRAY_BUFFER,
			moby_matrices.size() * sizeof(glm::mat4),
			moby_matrices.data(), GL_STATIC_DRAW);
		
		std::size_t moby_batch_class = INT64_MAX;
		std::size_t moby_batch_begin = 0;
		
		auto draw_moby_batch = [&](std::size_t batch_end) {
			if(lvl.moby_class_to_model.find(moby_batch_class) != lvl.moby_class_to_model.end()) {
				std::size_t model_index = lvl.moby_class_to_model.at(moby_batch_class);
				moby_model& model = lvl.moby_models[model_index];
				draw_moby_models(
					model,
					lvl.moby_textures,
					mode,
					true,
					false,
					moby_matrix_buffer(),
					moby_batch_begin * sizeof(glm::mat4),
					batch_end - moby_batch_begin);
			} else {
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
				glUseProgram(shaders.solid_colour.id());
				
				for(std::size_t i = moby_batch_begin; i < batch_end; i++) {
					const glm::mat4& local_to_clip = moby_matrices[i];
					glm::vec4 colour = get_colour((*lvl.gameplay().moby_instances)[i].selected, glm::vec4(0, 1, 0, 1));
					draw_cube(local_to_clip, colour);
				}
			}
		};
		
		size_t i = 0;
		for(MobyInstance& inst : opt_iterator(lvl.gameplay().moby_instances)) {
			if(inst.o_class != moby_batch_class) {
				draw_moby_batch(i);
				moby_batch_class = inst.o_class;
				moby_batch_begin = i;
			}
			i++;
		}
		draw_moby_batch(lvl.gameplay().moby_instances->size());
		
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glUseProgram(shaders.solid_colour.id());
		
		i = 0;
		for(MobyInstance& inst : opt_iterator(lvl.gameplay().moby_instances)) {
			if(inst.selected) {
				draw_cube(moby_matrices[i], selected_colour);
			}
			i++;
		}
	}
	
	//if(draw_triggers) {
	//	for(trigger_entity& trigger : lvl.world.triggers) {
	//		glm::mat4 local_to_clip = world_to_clip * trigger.local_to_world;
	//		glm::vec4 colour = get_colour(trigger.selected, glm::vec4(0, 0, 1, 1));
	//		draw_cube(local_to_clip, colour);
	//	}
	//}
	//
	//if(draw_splines) {
	//	for(regular_spline_entity& spline : lvl.world.splines) {
	//		glm::vec4 colour = get_colour(spline.selected, glm::vec4(1, 0.5, 0, 1));
	//		draw_spline(spline, world_to_clip, colour);
	//	}
	//}
	//
	//if(draw_grind_rails) {
	//	for(grindrail_spline_entity& spline : lvl.world.grindrails) {
	//		glm::vec4 colour = get_colour(spline.selected, glm::vec4(0, 0.5, 1, 1));
	//		draw_spline(spline, world_to_clip, colour);
	//		
	//		glm::mat4 local_to_world = glm::translate(glm::mat4(1.f), glm::vec3(spline.special_point));
	//		draw_cube(world_to_clip * local_to_world, colour);
	//	}
	//}
	
	if(draw_tfrags) {
		for(auto& frag : lvl.tfrags) {
			glm::vec4 colour(0.5, 0.5, 0.5, 1);
			draw_model(frag, world_to_clip, colour);
		}
	}
	
	if (draw_tcols) {
		for (auto& col : lvl.baked_collisions) {
			draw_model_vcolor(col, world_to_clip);
		}
	}

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void gl_renderer::draw_pickframe(level& lvl, glm::mat4 world_to_clip) const {
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	
	glUseProgram(shaders.solid_colour.id());
	
	auto encode_pick_colour = [&](InstanceId id) {
		glm::vec4 colour;
		// IDs are unique across all object types.
		colour.r = ((id.value & 0x00ff) >> 0) / 255.f;
		colour.g = ((id.value & 0xff00) >> 8) / 255.f;
		colour.b = ((id.type  & 0x00ff) >> 0) / 255.f;
		colour.a = ((id.type  & 0xff00) >> 8) / 255.f;
		return colour;
	};
	
	//if(draw_ties) {
	//	for(tie_entity& tie : lvl.world.ties) {
	//		glm::mat4 local_to_clip = world_to_clip * tie.local_to_world;
	//		glm::vec4 colour = encode_pick_colour(tie.id);
	//		draw_cube(local_to_clip, colour);
	//	}
	//}
	//
	//if(draw_shrubs) {
	//	for(shrub_entity& shrub : lvl.world.shrubs) {
	//		glm::mat4 local_to_clip = world_to_clip * shrub.local_to_world;
	//		glm::vec4 colour = encode_pick_colour(shrub.id);
	//		draw_cube(local_to_clip, colour);
	//	}
	//}
	//
	//if(draw_mobies) {
	//	for(moby_entity& moby : lvl.world.mobies) {
	//		glm::vec4 colour = encode_pick_colour(moby.id);
	//		draw_cube(moby.local_to_clip_cache, colour);
	//	}
	//}
	//
	//if(draw_splines) {
	//	for(regular_spline_entity& spline : lvl.world.splines) {
	//		glm::vec4 colour = encode_pick_colour(spline.id);
	//		draw_spline(spline, world_to_clip, colour);
	//	}
	//}
	//
	//if(draw_grind_rails) {
	//	for(grindrail_spline_entity& spline : lvl.world.grindrails) {
	//		glm::vec4 colour = encode_pick_colour(spline.id);
	//		draw_spline(spline, world_to_clip, colour);
	//	}
	//}
}

void gl_renderer::draw_spline(const std::vector<glm::vec4>& spline, const glm::mat4& world_to_clip, const glm::vec4& colour) const{
	
	glUniformMatrix4fv(shaders.solid_colour_transform, 1, GL_FALSE, &world_to_clip[0][0]);
	glUniform4f(shaders.solid_colour_rgb, colour.r, colour.g, colour.b, colour.a);

	GLuint vertex_buffer;
	glGenBuffers(1, &vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER,
		spline.size() * sizeof(glm::vec4),
		spline.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), nullptr);

	glDrawArrays(GL_LINE_STRIP, 0, spline.size());

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

	glDrawArrays(GL_TRIANGLES, 0, vertex_data.size());

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

void gl_renderer::draw_model_vcolor(const model& mdl, const glm::mat4& mvp) const {
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glUseProgram(shaders.vertex_color.id());
	glUniformMatrix4fv(shaders.vertex_color_transform, 1, GL_FALSE, &mvp[0][0]);

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, mdl.vertex_buffer());
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, mdl.vertex_color_buffer());
	glVertexAttribPointer(
		1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
		3,                                // size
		GL_FLOAT,                         // type
		GL_TRUE,                          // normalized?
		3 * sizeof(float),                // stride
		(void*)0                          // array buffer offset
	);

	glDrawArrays(GL_TRIANGLES, 0, mdl.vertex_buffer_size() / 3);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
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

void gl_renderer::draw_static_mesh(
		const float* vertex_data,
		size_t vertex_data_size, const
		glm::mat4 local_to_clip,
		glm::vec4 colour) {
	static std::map<const float*, GLint> vertex_buffers;
	
	GLuint vertex_buffer = 0;
	if(vertex_buffers.find(vertex_data) == vertex_buffers.end()) {
		glGenBuffers(1, &vertex_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
		glBufferData(GL_ARRAY_BUFFER, vertex_data_size, vertex_data, GL_STATIC_DRAW);
		vertex_buffers[vertex_data] = vertex_buffer;
	} else {
		vertex_buffer = vertex_buffers.at(vertex_data);
	}
	
	glUniformMatrix4fv(shaders.solid_colour_transform, 1, GL_FALSE, &local_to_clip[0][0]);
	glUniform4f(shaders.solid_colour_rgb, colour.r, colour.g, colour.b, colour.a);
	
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	glDrawArrays(GL_TRIANGLES, 0, vertex_data_size / sizeof(float));

	glDisableVertexAttribArray(0);
}

void gl_renderer::draw_moby_models(
		moby_model& model,
		std::vector<texture>& textures,
		view_mode mode,
		bool show_all_submodels,
		bool show_bounding_box,
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
						if(tex.opengl_texture.id == 0) {
							tex.upload_to_opengl();
						}
						
						glActiveTexture(GL_TEXTURE0);
						glBindTexture(GL_TEXTURE_2D, tex.opengl_texture.id);
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

	if (show_bounding_box) {
		glm::vec3 max = model.bounding_box.max / (float) INT16_MAX, min = model.bounding_box.min / (float) INT16_MAX;

		std::vector<float> bounding_box_verts {
			min.x, min.y, min.z, max.x, max.y, min.z, max.x, min.y,  min.z,
			min.x, max.y, min.z, max.x, max.y, min.z, min.x, min.y, min.z,
			min.x, min.y, max.z, max.x, max.y, max.z, max.x, min.y,  max.z,
			min.x, max.y, max.z, max.x, max.y, max.z, min.x, min.y, max.z,
			min.x, max.y, min.z, min.x, max.y, max.z, max.x, max.y,  min.z,
			min.x, max.y, max.z, max.x, max.y, max.z, max.x, max.y, min.z,
			min.x, min.y, min.z, min.x, min.y, max.z, max.x, min.y,  min.z,
			min.x, min.y, max.z, max.x, min.y, max.z, max.x, min.y, min.z,
			min.x, min.y, min.z, min.x, max.y, min.z, min.x, min.y, max.z,
			min.x, max.y, min.z, min.x, max.y, max.z, min.x, min.y, max.z,
			max.x, min.y, min.z, max.x, max.y, min.z, max.x, min.y,  max.z,
			max.x, max.y, min.z, max.x, max.y, max.z, max.x, min.y, max.z
		};

		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		glUseProgram(shaders.solid_colour_batch.id());

		if(model.bounding_box_buffer() == 0) {
			glGenBuffers(1, &model.bounding_box_buffer());
			glBindBuffer(GL_ARRAY_BUFFER, model.bounding_box_buffer());
			glBufferData(GL_ARRAY_BUFFER,
				bounding_box_verts.size() * sizeof(float),
				bounding_box_verts.data(), GL_STATIC_DRAW);
		}

		glm::vec4 colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
		glUniform4f(shaders.solid_colour_rgb, colour.r, colour.g, colour.b, colour.a);

		glEnableVertexAttribArray(4);
		glBindBuffer(GL_ARRAY_BUFFER, model.bounding_box_buffer());
		glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
		
		glDrawArrays(GL_TRIANGLES, 0, 108);
	}
	
	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);
	glDisableVertexAttribArray(4);

	glVertexAttribDivisor(0, 0);
	glVertexAttribDivisor(1, 0);
	glVertexAttribDivisor(2, 0);
	glVertexAttribDivisor(3, 0);
}

glm::mat4 gl_renderer::draw_single_moby(
		moby_model& model,
		std::vector<texture>& textures,
		view_params params,
		int width,
		int height) const {
	float fov_y = 45.0f;
	float camera_distance;
	float zoom_ratio;

	// Get the largest dimension of the model.
	float model_size = 0;
	model_size = glm::max(model_size, glm::abs(model.bounding_box.max.x - model.bounding_box.min.x));
	model_size = glm::max(model_size, glm::abs(model.bounding_box.max.y - model.bounding_box.min.y));
	model_size = glm::max(model_size, glm::abs(model.bounding_box.max.z - model.bounding_box.min.z));

	float focal_length = (height/2) / tan(glm::radians(fov_y/2));

	// Get a ratio of how wide the largest dimension of the model is compared to the render window width.
	if (width < height) {
		zoom_ratio = model_size / width;
	} else { // Same as above, but use height if the render window is shorter than wide.
		zoom_ratio = model_size / height;
	}
	 
	// Fit the camera to the model bounding box.
	camera_distance = focal_length * zoom_ratio / (float) INT16_MAX;

	glm::vec3 eye = glm::vec3(((camera_distance) * (2.0f - params.zoom)), 0, 0);

	glm::mat4 view_fixed = glm::lookAt(eye, glm::vec3(0), glm::vec3(0, 1, 0));
	glm::mat4 view_pitched = glm::rotate(view_fixed, params.pitch_yaw.x, glm::vec3(0, 0, 1));
	glm::mat4 view = glm::rotate(view_pitched, params.pitch_yaw.y, glm::vec3(0, 1, 0));
	glm::vec3 center_point = (model.bounding_box.max + model.bounding_box.min) * 0.5f / (float) INT16_MAX;
	glm::mat4 offset_view = glm::translate(view * RATCHET_TO_OPENGL_MATRIX, -center_point);

	glm::mat4 projection = glm::perspective(glm::radians(fov_y), width / (float) height, 0.01f, 100.0f);
	
	glm::mat4 local_to_clip = projection * offset_view;

	std::vector<GLuint> gl_textures;
	for(texture& tex : textures) {
		gl_textures.push_back(tex.opengl_texture.id);
	}
	
	gl_buffer local_to_clip_buffer;
	glGenBuffers(1, &local_to_clip_buffer());
	glBindBuffer(GL_ARRAY_BUFFER, local_to_clip_buffer());
	glBufferData(GL_ARRAY_BUFFER,
		sizeof(glm::mat4),
		&local_to_clip, GL_STATIC_DRAW);
	
	draw_moby_models(model, textures, params.mode, false, params.show_bounding_box, local_to_clip_buffer(), 0, 1);
	return local_to_clip;
}

void gl_renderer::draw_shrub_models(
	shrub_model& model,
	std::vector<texture>& textures,
	view_mode mode,
	bool show_all_submodels,
	GLuint local_to_world_buffer,
	std::size_t instance_offset,
	std::size_t count) const {
	switch (mode) {
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
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(instance_offset + sizeof(glm::vec4) * 0));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(instance_offset + sizeof(glm::vec4) * 1));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(instance_offset + sizeof(glm::vec4) * 2));
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(instance_offset + sizeof(glm::vec4) * 3));

	glVertexAttribDivisor(0, 1);
	glVertexAttribDivisor(1, 1);
	glVertexAttribDivisor(2, 1);
	glVertexAttribDivisor(3, 1);

	shrub_texture_entry texture_data = {};
	for (std::size_t i = 0; i < model.submodels.size(); i++) {
		shrub_submodel& submodel = model.submodels[i];
		if (!show_all_submodels && !submodel.visible_in_model_viewer) {
			continue;
		}

		if (submodel.vertices.size() == 0) {
			continue;
		}

		if (submodel.vertex_buffer() == 0) {
			glGenBuffers(1, &submodel.vertex_buffer());
			glBindBuffer(GL_ARRAY_BUFFER, submodel.vertex_buffer());
			glBufferData(GL_ARRAY_BUFFER,
				submodel.vertices.size() * sizeof(moby_model_vertex),
				submodel.vertices.data(), GL_STATIC_DRAW);
		}

		if (submodel.st_buffer() == 0) {
			glGenBuffers(1, &submodel.st_buffer());
			glBindBuffer(GL_ARRAY_BUFFER, submodel.st_buffer());
			glBufferData(GL_ARRAY_BUFFER,
				submodel.st_coords.size() * sizeof(moby_model_st),
				submodel.st_coords.data(), GL_STATIC_DRAW);
		}

		for (shrub_subsubmodel& subsubmodel : submodel.subsubmodels) {
			if (subsubmodel.index_buffer() == 0) {
				glGenBuffers(1, &subsubmodel.index_buffer());
				glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, subsubmodel.index_buffer());
				glBufferData(GL_ELEMENT_ARRAY_BUFFER,
					subsubmodel.indices.size(),
					subsubmodel.indices.data(), GL_STATIC_DRAW);
			}

			if (subsubmodel.texture) {
				texture_data = *subsubmodel.texture;
			}

			switch (mode) {
			case view_mode::WIREFRAME: {
				glm::vec4 colour = colour_coded_submodel_index(i, model.submodels.size());
				glUniform4f(shaders.solid_colour_batch_rgb, colour.r, colour.g, colour.b, colour.a);
				break;
			}
			case view_mode::TEXTURED_POLYGONS: {
				if (model.texture_indices.size() > (std::size_t)texture_data.texture_index_1) {
					texture& tex = textures.at(model.texture_indices.at(texture_data.texture_index_1));
					if (tex.opengl_texture.id == 0) {
						tex.upload_to_opengl();
					}

					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, tex.opengl_texture.id);
				}
				else {
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
			glVertexAttribPointer(4, 3, GL_SHORT, GL_TRUE, sizeof(shrub_model_vertex), (void*)offsetof(shrub_model_vertex, x));

			glEnableVertexAttribArray(5);
			glBindBuffer(GL_ARRAY_BUFFER, submodel.st_buffer());
			glVertexAttribPointer(5, 2, GL_SHORT, GL_TRUE, sizeof(shrub_model_st), (void*)offsetof(shrub_model_st, s));

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

glm::mat4 gl_renderer::get_world_to_clip() const {
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), viewport_size.x / viewport_size.y, 0.1f, 10000.0f);

	auto rot = camera_rotation;
	glm::mat4 pitch = glm::rotate(glm::mat4(1.0f), rot.x, glm::vec3(1.0f, 0.0f, 0.0f));
	glm::mat4 yaw   = glm::rotate(glm::mat4(1.0f), rot.y, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 translate = glm::translate(glm::mat4(1.0f), -camera_position);
	glm::mat4 view = pitch * yaw * RATCHET_TO_OPENGL_MATRIX * translate;
	return projection * view;
}

glm::mat4 gl_renderer::get_local_to_clip(glm::mat4 world_to_clip, glm::vec3 position, glm::vec3 rotation) const {
	glm::mat4 model = glm::translate(glm::mat4(1.f), position);
	model = glm::rotate(model, rotation.x, glm::vec3(1, 0, 0));
	model = glm::rotate(model, rotation.y, glm::vec3(0, 1, 0));
	model = glm::rotate(model, rotation.z, glm::vec3(0, 0, 1));
	return world_to_clip * model;
}

glm::vec3 gl_renderer::apply_local_to_screen(glm::mat4 world_to_clip, glm::mat4 local_to_world) const {
	glm::mat4 local_to_clip = get_local_to_clip(world_to_clip, glm::vec3(1.f), glm::vec3(0.f));
	glm::vec4 homogeneous_pos = local_to_clip * glm::vec4(glm::vec3(local_to_world[3]), 1);
	glm::vec3 gl_pos {
			homogeneous_pos.x / homogeneous_pos.w,
			homogeneous_pos.y / homogeneous_pos.w,
			homogeneous_pos.z
	};
	ImVec2 window_pos = ImGui::GetWindowPos();
	glm::vec3 screen_pos(
			window_pos.x + (1 + gl_pos.x) * viewport_size.x / 2.0,
			window_pos.y + (1 + gl_pos.y) * viewport_size.y / 2.0,
			gl_pos.z
	);
	return screen_pos;
}

glm::vec3 gl_renderer::create_ray(glm::mat4 world_to_clip, ImVec2 screen_pos) {
	auto imgui_to_glm = [](ImVec2 v) { return glm::vec2(v.x, v.y); };
	glm::vec2 relative_pos = imgui_to_glm(screen_pos) - imgui_to_glm(viewport_pos);
	glm::vec2 device_space_pos = 2.f * relative_pos / imgui_to_glm(viewport_size) - 1.f;
	glm::vec4 clip_pos(device_space_pos.x, device_space_pos.y, 1.f, 1.f);
	glm::mat4 clip_to_world = glm::inverse(world_to_clip);
	glm::vec4 world_pos = clip_to_world * clip_pos;
	glm::vec3 direction = glm::normalize(glm::vec3(world_pos));
	return direction;
}

void gl_renderer::reset_camera(app* a) {
	camera_rotation = glm::vec3(0, 0, 0);
	if(level* lvl = a->get_level()) {
		if(opt_size(lvl->gameplay().moby_instances) > 0) {
			camera_position = (*lvl->gameplay().moby_instances)[0].position();
		} else if(lvl->gameplay().properties.has_value()) {
			camera_position = lvl->gameplay().properties->first_part.ship_position.unpack();
		} else {
			camera_position = glm::vec3(0, 0, 0);
		}
	} else {
		camera_position = glm::vec3(0, 0, 0);
	}
}
