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
#include "shaders.h"

static void draw_mobies(level& lvl, const std::vector<MobyInstance>& instances);
static void draw_mesh(const RenderMesh& mesh, const glm::mat4& local_to_world);
static void draw_mesh_instanced(const RenderMesh& mesh, GLuint inst_buffer, size_t inst_begin, size_t inst_count);
static Mesh create_cube();

static shader_programs shaders;
static RenderMesh cube;
static std::vector<glm::mat4> moby_matrices;

void init_renderer() {
	shaders.init();
	cube = upload_mesh(create_cube());
}

void prepare_frame(level& lvl, const glm::mat4& world_to_clip) {
	moby_matrices.clear();
	moby_matrices.reserve(opt_size(lvl.gameplay().moby_instances));
	for(MobyInstance& inst : opt_iterator(lvl.gameplay().moby_instances)) {
		moby_matrices.push_back(world_to_clip * inst.matrix());
	}
}

void draw_level(level& lvl, const glm::mat4& world_to_clip, const RenderSettings& settings) {
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glUseProgram(shaders.solid_colour.id());
	
	static const glm::vec4 selected_colour = glm::vec4(1, 0, 0, 1);
	
	auto get_colour = [&](bool selected, glm::vec4 normal_colour) {
		return selected ? selected_colour : normal_colour;
	};
	
	//if(settings.draw_ties) {
	//	for(TieInstance& inst : opt_iterator(lvl.gameplay().tie_instances)) {
	//		glm::mat4 local_to_clip = world_to_clip * inst.matrix();
	//		glm::vec4 colour = get_colour(inst.selected, glm::vec4(0.5, 0, 1, 1));
	//		draw_cube(local_to_clip, colour);
	//		draw_mes
	//	}
	//}
	
	if(settings.draw_mobies && lvl.gameplay().moby_instances.has_value()) {
		draw_mobies(lvl, *lvl.gameplay().moby_instances);
	}
	
	//if(draw_cuboids) {
	//	for(Cuboid& inst : opt_iterator(lvl.gameplay().cuboids)) {
	//		glm::mat4 local_to_clip = world_to_clip * inst.matrix();
	//		glm::vec4 colour = get_colour(inst.selected, glm::vec4(0, 0, 1, 1));
	//		draw_cube(local_to_clip, colour);
	//	}
	//}
	//
	//if(draw_spheres) {
	//	for(Sphere& inst : opt_iterator(lvl.gameplay().spheres)) {
	//		glm::mat4 local_to_clip = world_to_clip * inst.matrix();
	//		glm::vec4 colour = get_colour(inst.selected, glm::vec4(0, 0, 1, 1));
	//		draw_cube(local_to_clip, colour);
	//	}
	//}
	//
	//if(draw_cylinders) {
	//	for(Cylinder& inst : opt_iterator(lvl.gameplay().cylinders)) {
	//		glm::mat4 local_to_clip = world_to_clip * inst.matrix();
	//		glm::vec4 colour = get_colour(inst.selected, glm::vec4(0, 0, 1, 1));
	//		draw_cube(local_to_clip, colour);
	//	}
	//}
	//
	//if(draw_paths) {
	//	for(Path& inst : opt_iterator(lvl.gameplay().paths)) {
	//		glm::vec4 colour = get_colour(inst.selected, glm::vec4(1, 0.5, 0, 1));
	//		draw_spline(inst.spline(), world_to_clip, colour);
	//	}
	//}
	//
	//if(draw_grind_paths) {
	//	for(GrindPath& inst : opt_iterator(lvl.gameplay().grind_paths)) {
	//		glm::vec4 colour = get_colour(inst.selected, glm::vec4(0, 0.5, 1, 1));
	//		draw_spline(inst.spline(), world_to_clip, colour);
	//		
	//		glm::mat4 local_to_world = glm::translate(glm::mat4(1.f), glm::vec3(inst.bounding_sphere()));
	//		draw_cube(world_to_clip * local_to_world, colour);
	//	}
	//}
	//
	//if(draw_tfrags) {
	//	for(auto& frag : lvl.tfrags) {
	//		glm::vec4 colour(0.5, 0.5, 0.5, 1);
	//		draw_model(frag, world_to_clip, colour);
	//	}
	//}
	
	if(settings.draw_collision) {
		for(const RenderMesh& mesh : lvl.collision) {
			draw_mesh(mesh, world_to_clip);
		}
	}

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void draw_pickframe(level& lvl, const glm::mat4& world_to_clip, const RenderSettings& settings) {
	//glEnable(GL_DEPTH_TEST);
	//glDepthFunc(GL_LESS);
	//
	//glUseProgram(shaders.solid_colour.id);
	//
	//auto encode_pick_colour = [&](InstanceId id) {
	//	glm::vec4 colour;
	//	colour.r = ((id.type  & 0x00ff) >> 0) / 255.f;
	//	colour.g = ((id.value & 0x00ff) >> 0) / 255.f;
	//	colour.b = ((id.value & 0xff00) >> 8) / 255.f;
	//	colour.a = 1.f;
	//	return colour;
	//};
	//
	//if(draw_ties) {
	//	for(TieInstance& inst : opt_iterator(lvl.gameplay().tie_instances)) {
	//		glm::vec4 colour = encode_pick_colour(inst.id());
	//		draw_cube(world_to_clip * inst.matrix(), colour);
	//	}
	//}
	//
	//if(draw_shrubs) {
	//	for(ShrubInstance& inst : opt_iterator(lvl.gameplay().shrub_instances)) {
	//		glm::vec4 colour = encode_pick_colour(inst.id());
	//		draw_cube(world_to_clip * inst.matrix(), colour);
	//	}
	//}
	//
	//if(draw_mobies) {
	//	size_t i = 0;
	//	for(MobyInstance& inst : opt_iterator(lvl.gameplay().moby_instances)) {
	//		glm::vec4 colour = encode_pick_colour(inst.id());
	//		draw_cube(world_to_clip * inst.matrix(), colour);
	//		i++;
	//	}
	//}
	//
	//if(draw_paths) {
	//	for(Path& inst : opt_iterator(lvl.gameplay().paths)) {
	//		glm::vec4 colour = encode_pick_colour(inst.id());
	//		draw_spline(inst.spline(), world_to_clip, colour);
	//	}
	//}
	//
	//if(draw_grind_paths) {
	//	for(GrindPath& inst : opt_iterator(lvl.gameplay().grind_paths)) {
	//		glm::vec4 colour = encode_pick_colour(inst.id());
	//		draw_spline(inst.spline(), world_to_clip, colour);
	//	}
	//}
}

static void draw_mobies(level& lvl, const std::vector<MobyInstance>& instances) {
	if(instances.size() < 1) {
		return;
	}
	
	GlBuffer inst_buffer;
	glGenBuffers(1, &inst_buffer.id);
	glBindBuffer(GL_ARRAY_BUFFER, inst_buffer.id);
	size_t inst_buffer_size_bytes = moby_matrices.size() * sizeof(glm::mat4);
	glBufferData(GL_ARRAY_BUFFER, inst_buffer_size_bytes, moby_matrices.data(), GL_STATIC_DRAW);
	
	size_t begin = 0;
	size_t end = 0;
	for(size_t i = 1; i <= instances.size(); i++) {
		s32 last_class = instances[i - 1].o_class;
		if(i == instances.size() || instances[i].o_class != last_class) {
			end = i;
			auto cls = lvl.mobies.find(last_class);
			if(cls != lvl.mobies.end()) {
				draw_mesh_instanced(cls->second, inst_buffer.id, begin, end - begin);
			} else {
				draw_mesh_instanced(cube, inst_buffer.id, begin, end - begin);
			}
			begin = i;
		}
	}
}

static void draw_mesh(const RenderMesh& mesh, const glm::mat4& local_to_world) {
	GlBuffer inst_buffer;
	glGenBuffers(1, &inst_buffer.id);
	glBindBuffer(GL_ARRAY_BUFFER, inst_buffer.id);
	glBufferData(GL_ARRAY_BUFFER, sizeof(local_to_world), &local_to_world, GL_STATIC_DRAW);
	
	draw_mesh_instanced(mesh, inst_buffer.id, 0, 1);
}

static void draw_mesh_instanced(const RenderMesh& mesh, GLuint inst_buffer, size_t inst_begin, size_t inst_count) {
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glUseProgram(shaders.textured.id());
	
	size_t inst_offset = inst_begin * sizeof(glm::mat4);
	
	glBindBuffer(GL_ARRAY_BUFFER, inst_buffer);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*) (inst_offset + sizeof(glm::vec4) * 0));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*) (inst_offset + sizeof(glm::vec4) * 1));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*) (inst_offset + sizeof(glm::vec4) * 2));
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*) (inst_offset + sizeof(glm::vec4) * 3));
	
	glVertexAttribDivisor(0, 1);
	glVertexAttribDivisor(1, 1);
	glVertexAttribDivisor(2, 1);
	glVertexAttribDivisor(3, 1);
	
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vertex_buffer.id);
	setup_vertex_attributes(4, 5, 6);
	
	for(const RenderSubMesh& submesh : mesh.submeshes) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, submesh.texture);
		
		glUniform1i(shaders.textured_sampler, 0);
		
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, submesh.index_buffer.id);
		glDrawElementsInstanced(GL_TRIANGLES, submesh.index_count, GL_UNSIGNED_INT, nullptr, (GLsizei) inst_count);
	}
	
	glDisableVertexAttribArray(6);
	glDisableVertexAttribArray(5);
	glDisableVertexAttribArray(4);
	
	glDisableVertexAttribArray(4);
	glDisableVertexAttribArray(3);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(0);

	glVertexAttribDivisor(0, 0);
	glVertexAttribDivisor(1, 0);
	glVertexAttribDivisor(2, 0);
	glVertexAttribDivisor(3, 0);
}

glm::mat4 compose_world_to_clip(const ImVec2& view_size, const glm::vec3& cam_pos, const glm::vec2& cam_rot) {
	glm::mat4 projection = glm::perspective(glm::radians(45.0f), view_size.x / view_size.y, 0.1f, 10000.0f);
	glm::mat4 pitch = glm::rotate(glm::mat4(1.0f), cam_rot.x, glm::vec3(1.0f, 0.0f, 0.0f));
	glm::mat4 yaw = glm::rotate(glm::mat4(1.0f), cam_rot.y, glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 translate = glm::translate(glm::mat4(1.0f), -cam_pos);
	glm::mat4 view = pitch * yaw * RATCHET_TO_OPENGL_MATRIX * translate;
	return projection * view;
}

glm::vec3 apply_local_to_screen(const glm::mat4& world_to_clip, const glm::mat4& local_to_world, const ImVec2& view_size) {
	glm::mat4 local_to_clip = world_to_clip * local_to_clip;
	glm::vec4 homogeneous_pos = local_to_clip * glm::vec4(glm::vec3(local_to_world[3]), 1);
	glm::vec3 gl_pos {
			homogeneous_pos.x / homogeneous_pos.w,
			homogeneous_pos.y / homogeneous_pos.w,
			homogeneous_pos.z
	};
	ImVec2 window_pos = ImGui::GetWindowPos();
	glm::vec3 screen_pos(
			window_pos.x + (1 + gl_pos.x) * view_size.x / 2.0,
			window_pos.y + (1 + gl_pos.y) * view_size.y / 2.0,
			gl_pos.z
	);
	return screen_pos;
}

glm::vec3 create_ray(const glm::mat4& world_to_clip, const ImVec2& screen_pos, const ImVec2& view_pos, const ImVec2& view_size) {
	auto imgui_to_glm = [](ImVec2 v) { return glm::vec2(v.x, v.y); };
	glm::vec2 relative_pos = imgui_to_glm(screen_pos) - imgui_to_glm(view_pos);
	glm::vec2 device_space_pos = 2.f * relative_pos / imgui_to_glm(view_size) - 1.f;
	glm::vec4 clip_pos(device_space_pos.x, device_space_pos.y, 1.f, 1.f);
	glm::mat4 clip_to_world = glm::inverse(world_to_clip);
	glm::vec4 world_pos = clip_to_world * clip_pos;
	glm::vec3 direction = glm::normalize(glm::vec3(world_pos));
	return direction;
}

void reset_camera(app* a) {
	a->render_settings.camera_rotation = glm::vec3(0, 0, 0);
	if(level* lvl = a->get_level()) {
		if(opt_size(lvl->gameplay().moby_instances) > 0) {
			a->render_settings.camera_position = (*lvl->gameplay().moby_instances)[0].position();
		} else if(lvl->gameplay().properties.has_value()) {
			a->render_settings.camera_position = lvl->gameplay().properties->first_part.ship_position;
		} else {
			a->render_settings.camera_position = glm::vec3(0, 0, 0);
		}
	} else {
		a->render_settings.camera_position = glm::vec3(0, 0, 0);
	}
}

static Mesh create_cube() {
	Mesh mesh;
	mesh.vertices.emplace_back(glm::vec3(-1.f, -1.f, -1.f));
	mesh.vertices.emplace_back(glm::vec3(-1.f, -1.f, +1.f));
	mesh.vertices.emplace_back(glm::vec3(-1.f, +1.f, -1.f));
	mesh.vertices.emplace_back(glm::vec3(-1.f, +1.f, +1.f));
	mesh.vertices.emplace_back(glm::vec3(+1.f, -1.f, -1.f));
	mesh.vertices.emplace_back(glm::vec3(+1.f, -1.f, +1.f));
	mesh.vertices.emplace_back(glm::vec3(+1.f, +1.f, -1.f));
	mesh.vertices.emplace_back(glm::vec3(+1.f, +1.f, +1.f));
	SubMesh& submesh = mesh.submeshes.emplace_back();
	submesh.faces.emplace_back(0, 1, 2, 3);
	submesh.faces.emplace_back(4, 5, 6, 7);
	submesh.faces.emplace_back(0, 1, 4, 5);
	submesh.faces.emplace_back(2, 3, 6, 7);
	submesh.faces.emplace_back(0, 2, 4, 6);
	submesh.faces.emplace_back(1, 3, 5, 7);
	return mesh;
}
