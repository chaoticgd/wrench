/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2023 chaoticgd

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

#include <toolwads/wads.h>
#include <engine/compression.h>
#include <gui/shaders.h>
#include <gui/render_mesh.h>
#include <editor/app.h>

template <typename ThisInstance>
static void upload_instance_buffer(GLuint& buffer, const InstanceList<ThisInstance>& insts);
static glm::vec4 inst_colour(const Instance& inst);
static glm::vec4 encode_inst_id(InstanceId id);
static void draw_instances(
	Level& lvl, GLenum mesh_mode, bool draw_wireframes, const RenderSettings& settings);
static void draw_moby_instances(
	Level& lvl, InstanceList<MobyInstance>& instances, GLenum mesh_mode, GLenum cube_mode);
static void draw_tie_instances(
	Level& lvl, const InstanceList<TieInstance>& instances, GLenum mesh_mode, GLenum cube_mode);
static void draw_shrub_instances(
	Level& lvl, const InstanceList<ShrubInstance>& instances, GLenum mesh_mode, GLenum cube_mode);
static void draw_selected_shrub_normals(Level& lvl);
static void draw_selected_moby_normals(Level& lvl);
template <typename ThisPath>
static void draw_paths(const InstanceList<ThisPath>& paths, const RenderMaterial& material);
static void draw_icons(Level& lvl, const RenderSettings& settings);
static void draw_moby_icons(Level& lvl, InstanceList<MobyInstance>& instances);
static void draw_cube_instanced(
	GLenum cube_mode, const RenderMaterial& material, GLuint inst_buffer, size_t inst_begin, size_t inst_count);
static void draw_icon_instanced(s32 type, GLuint inst_buffer, size_t inst_begin, size_t inst_count);
static void draw_mesh(
	const RenderMesh& mesh, const RenderMaterial* mats, size_t mat_count, const glm::mat4& local_to_world);
static void draw_mesh_instanced(const RenderMesh& mesh, const RenderMaterial* mats, size_t mat_count, GLuint inst_buffer, size_t inst_begin, size_t inst_count);
static Mesh create_fill_cube();
static Mesh create_line_cube();
static Mesh create_quad();
static Texture create_white_texture();
static void set_shader(const Shader& shader);

static Shaders shaders;
static RenderMesh fill_cube;
static RenderMesh line_cube;
static RenderMesh quad;
static RenderMaterial purple;
static RenderMaterial green;
static RenderMaterial white;
static RenderMaterial orange;
static RenderMaterial cyan;
static RenderMaterial blue;
static RenderMaterial instance_icons[ARRAY_SIZE(wadinfo.editor.instance_3d_view_icons)];
static GLuint moby_inst_buffer = 0;
static GLuint moby_group_inst_buffer = 0;
static GLuint tie_inst_buffer = 0;
static GLuint tie_group_inst_buffer = 0;
static GLuint shrub_inst_buffer = 0;
static GLuint shrub_group_inst_buffer = 0;
static GLuint point_light_inst_buffer = 0;
static GLuint env_sample_point_inst_buffer = 0;
static GLuint env_transition_inst_buffer = 0;
static GLuint cuboid_inst_buffer = 0;
static GLuint sphere_inst_buffer = 0;
static GLuint cylinder_inst_buffer = 0;
static GLuint pill_inst_buffer = 0;
static GLuint camera_inst_buffer = 0;
static GLuint sound_inst_buffer = 0;
static GLuint area_inst_buffer = 0;
static GlBuffer ghost_inst_buffer;
static GLuint program = 0;

void init_renderer()
{
	shaders.init();
	
	fill_cube = upload_mesh(create_fill_cube(), false);
	line_cube = upload_mesh(create_line_cube(), false);
	quad = upload_mesh(create_quad(), false);
	
	purple = upload_material(Material{"", glm::vec4(0.5f, 0.f, 1.f, 1.f)}, {create_white_texture()});
	green = upload_material(Material{"", glm::vec4(0.f, 0.5f, 0.f, 1.f)}, {create_white_texture()});
	white = upload_material(Material{"", glm::vec4(1.f, 1.f, 1.f, 1.f)}, {create_white_texture()});
	orange = upload_material(Material{"", glm::vec4(1.f, 0.5f, 0.f, 1.f)}, {create_white_texture()});
	cyan = upload_material(Material{"", glm::vec4(0.f, 0.5f, 1.f, 1.f)}, {create_white_texture()});
	blue = upload_material(Material{"", glm::vec4(0.f, 0.0f, 1.f, 1.f)}, {create_white_texture()});
	
	for (s32 i = 0; i < ARRAY_SIZE(wadinfo.editor.instance_3d_view_icons); i++) {
		if (!wadinfo.editor.instance_3d_view_icons[i].offset.empty()) {
			g_editorwad.seek(wadinfo.editor.instance_3d_view_icons[i].offset.bytes());
			std::vector<u8> compressed = g_editorwad.read_multiple<u8>(wadinfo.editor.instance_3d_view_icons[i].size.bytes());
			std::vector<u8> decompressed;
			decompress_wad(decompressed, compressed);
			s32 width = *(s32*) &decompressed[0];
			s32 height = *(s32*) &decompressed[4];
			decompressed.erase(decompressed.begin(), decompressed.begin() + 16);
			std::vector<Texture> textures = {Texture::create_rgba(width, height, std::move(decompressed))};
			instance_icons[i] = upload_material(Material{"", glm::vec4(1.f, 1.f, 1.f, 1.f)}, textures);
		}
	}
}

void shutdown_renderer()
{
	shaders = Shaders();
	
	fill_cube = RenderMesh();
	line_cube = RenderMesh();
	quad = RenderMesh();
	
	purple.texture.destroy();
	green.texture.destroy();
	white.texture.destroy();
	orange.texture.destroy();
	cyan.texture.destroy();
	blue.texture.destroy();
	
	for (s32 i = 0; i < ARRAY_SIZE(wadinfo.editor.instance_3d_view_icons); i++) {
		instance_icons[i].texture.destroy();
	}
	
	ghost_inst_buffer.destroy();
}

void prepare_frame(Level& lvl)
{
	upload_instance_buffer(moby_inst_buffer, lvl.instances().moby_instances);
	upload_instance_buffer(moby_group_inst_buffer, lvl.instances().moby_groups);
	upload_instance_buffer(tie_inst_buffer, lvl.instances().tie_instances);
	upload_instance_buffer(tie_group_inst_buffer, lvl.instances().tie_groups);
	upload_instance_buffer(shrub_inst_buffer, lvl.instances().shrub_instances);
	upload_instance_buffer(shrub_group_inst_buffer, lvl.instances().shrub_groups);
	upload_instance_buffer(point_light_inst_buffer, lvl.instances().point_lights);
	upload_instance_buffer(env_sample_point_inst_buffer, lvl.instances().env_sample_points);
	upload_instance_buffer(env_transition_inst_buffer, lvl.instances().env_transitions);
	upload_instance_buffer(cuboid_inst_buffer, lvl.instances().cuboids);
	upload_instance_buffer(sphere_inst_buffer, lvl.instances().spheres);
	upload_instance_buffer(cylinder_inst_buffer, lvl.instances().cylinders);
	upload_instance_buffer(pill_inst_buffer, lvl.instances().pills);
	upload_instance_buffer(camera_inst_buffer, lvl.instances().cameras);
	upload_instance_buffer(sound_inst_buffer, lvl.instances().sound_instances);
	upload_instance_buffer(area_inst_buffer, lvl.instances().areas);
}

struct InstanceData
{
	InstanceData(const glm::mat4& m, const glm::vec4& c, const glm::vec4& i) : matrix(m), colour(c), id(i) {}
	glm::mat4 matrix;
	glm::vec4 colour;
	glm::vec4 id;
};

template <typename ThisInstance>
static void upload_instance_buffer(GLuint& buffer, const InstanceList<ThisInstance>& insts)
{
	static std::vector<InstanceData> inst_data;
	inst_data.clear();
	inst_data.reserve(insts.size());
	for (const ThisInstance& inst : insts) {
		glm::mat4 matrix;
		if (inst.is_dragging) {
			matrix = inst.drag_preview_matrix;
		} else {
			matrix = inst.transform().matrix();
		}
		inst_data.emplace_back(matrix, inst_colour(inst), encode_inst_id(inst.id()));
	}
	
	glDeleteBuffers(1, &buffer);
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	size_t inst_buffer_size = inst_data.size() * sizeof(InstanceData);
	glBufferData(GL_ARRAY_BUFFER, inst_buffer_size, inst_data.data(), GL_STATIC_DRAW);
}

static glm::vec4 inst_colour(const Instance& inst)
{
	if (inst.selected) {
		return glm::vec4(1.f, 0.f, 0.f, 1.f);
	} else if (inst.referenced_by_selected) {
		return glm::vec4(0.f, 0.f, 1.f, 1.f);
	}
	return glm::vec4(0.f, 0.f, 0.f, 0.f);
}

static glm::vec4 encode_inst_id(InstanceId id)
{
	glm::vec4 colour;
	colour.r = ((id.type  & 0x00ff) >> 0) / 255.f;
	colour.g = ((id.value & 0x00ff) >> 0) / 255.f;
	colour.b = ((id.value & 0xff00) >> 8) / 255.f;
	colour.a = 1.f;
	return colour;
}

void draw_level(
	Level& lvl, const glm::mat4& view, const glm::mat4& projection, const RenderSettings& settings)
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	set_shader(shaders.textured);
	glUniformMatrix4fv(shaders.textured_view_matrix, 1, GL_FALSE, &view[0][0]);
	glUniformMatrix4fv(shaders.textured_projection_matrix, 1, GL_FALSE, &projection[0][0]);
	
	for (const EditorChunk& chunk : lvl.chunks) {
		if (settings.draw_tfrags) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			draw_mesh(chunk.tfrags, lvl.tfrag_materials.data(), lvl.tfrag_materials.size(), glm::mat4(1.f));
		}
		
		if (settings.draw_collision) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			draw_mesh(chunk.collision, chunk.collision_materials.data(), chunk.collision_materials.size(), glm::mat4(1.f));
		}
		
		if (settings.draw_hero_collision) {
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			for (const RenderMesh& mesh : chunk.hero_collision) {
				draw_mesh(mesh, &blue, 1, glm::mat4(1.f));
			}
		}
	}
	
	draw_instances(lvl, GL_FILL, true, settings);
	
	set_shader(shaders.selection);
	glUniformMatrix4fv(shaders.selection_view_matrix, 1, GL_FALSE, &view[0][0]);
	glUniformMatrix4fv(shaders.selection_projection_matrix, 1, GL_FALSE, &projection[0][0]);
	
	draw_instances(lvl, GL_LINE, true, settings);
	
	if (settings.draw_selected_instance_normals) {
		draw_selected_shrub_normals(lvl);
		draw_selected_moby_normals(lvl);
	}
	
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	set_shader(shaders.icons);
	glUniformMatrix4fv(shaders.icons_view_matrix, 1, GL_FALSE, &view[0][0]);
	glUniformMatrix4fv(shaders.icons_projection_matrix, 1, GL_FALSE, &projection[0][0]);
	
	draw_icons(lvl, settings);
}

void draw_pickframe(
	Level& lvl, const glm::mat4& view, const glm::mat4& projection, const RenderSettings& settings)
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	
	set_shader(shaders.pickframe);
	glUniformMatrix4fv(shaders.pickframe_view_matrix, 1, GL_FALSE, &view[0][0]);
	glUniformMatrix4fv(shaders.pickframe_projection_matrix, 1, GL_FALSE, &projection[0][0]);

	draw_instances(lvl, GL_FILL, false, settings);
	
	set_shader(shaders.pickframe_icons);
	glUniformMatrix4fv(shaders.pickframe_icons_view_matrix, 1, GL_FALSE, &view[0][0]);
	glUniformMatrix4fv(shaders.pickframe_icons_projection_matrix, 1, GL_FALSE, &projection[0][0]);
	
	draw_icons(lvl, settings);
}

void draw_model_preview(
	const RenderMesh& mesh,
	const std::vector<RenderMaterial>& materials,
	const glm::mat4* bb,
	const glm::mat4& view,
	const glm::mat4& projection,
	bool wireframe) 
{
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	
	glm::mat4 local_to_world(1.f);
	
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	set_shader(shaders.textured);
	glUniformMatrix4fv(shaders.textured_view_matrix, 1, GL_FALSE, &view[0][0]);
	glUniformMatrix4fv(shaders.textured_projection_matrix, 1, GL_FALSE, &projection[0][0]);
	draw_mesh(mesh, materials.data(), materials.size(), local_to_world);
	
	if (wireframe) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		set_shader(shaders.selection);
		glUniformMatrix4fv(shaders.selection_view_matrix, 1, GL_FALSE, &view[0][0]);
		glUniformMatrix4fv(shaders.selection_projection_matrix, 1, GL_FALSE, &projection[0][0]);
		draw_mesh(mesh, materials.data(), materials.size(), local_to_world);
	}
	
	if (bb) {
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		draw_mesh(line_cube, &white, 1, *bb);
	}
}

void draw_drag_ghosts(Level& lvl, const std::vector<InstanceId>& ids, const RenderSettings& settings)
{
	// Prepare matrices to upload.
	static std::vector<InstanceData> inst_data;
	inst_data.clear();
	for (const InstanceId& id : ids) {
		Instance* inst = lvl.instances().from_id(id);
		if (inst && inst->has_component(COM_TRANSFORM)) {
			inst_data.emplace_back(inst->transform().matrix(), glm::vec4(1.f, 1.f, 1.f, 1.f), glm::vec4(0.f, 0.f, 0.f, 0.f));
		}
	}
	
	// Upload matrices.
	glDeleteBuffers(1, &ghost_inst_buffer.id);
	glGenBuffers(1, &ghost_inst_buffer.id);
	glBindBuffer(GL_ARRAY_BUFFER, ghost_inst_buffer.id);
	size_t inst_buffer_size = inst_data.size() * sizeof(InstanceData);
	glBufferData(GL_ARRAY_BUFFER, inst_buffer_size, inst_data.data(), GL_STATIC_DRAW);
	
	// Prepare for drawing.
	set_shader(shaders.selection);
	glUniformMatrix4fv(shaders.selection_view_matrix, 1, GL_FALSE, &settings.view_gl[0][0]);
	glUniformMatrix4fv(shaders.selection_projection_matrix, 1, GL_FALSE, &settings.projection[0][0]);
	
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	
	// Draw wireframes.
	static std::vector<const Instance*> inst_ptrs;
	for (size_t i = 0; i < ids.size();) {
		InstanceId first_id = ids[i];
		const Instance* first_inst = lvl.instances().from_id(first_id);
		InstanceType first_type = first_inst->type();
		verify_fatal(first_inst);
		inst_ptrs.emplace_back(first_inst);
		size_t j;
		for (j = i + 1; j < ids.size(); j++) {
			InstanceId id = ids[j];
			const Instance* inst = lvl.instances().from_id(id);
			InstanceType type = inst->type();
			if (type != first_type || (((type == INST_MOBY || type == INST_TIE || type == INST_SHRUB) && inst->o_class() == first_inst->o_class()))) {
				break;
			}
			inst_ptrs.emplace_back(inst);
		}
		size_t count = j - i;
		bool drawn = false;
		if (first_type == INST_MOBY) {
			auto iter = lvl.moby_classes.find(first_inst->o_class());
			if (iter != lvl.moby_classes.end() && iter->second.render_mesh.has_value()) {
				EditorClass& ec = iter->second;
				draw_mesh_instanced(*ec.render_mesh, ec.materials.data(), ec.materials.size(), ghost_inst_buffer.id, i, count);
				drawn = true;
			}
		} else if (first_type == INST_TIE) {
			auto iter = lvl.tie_classes.find(first_inst->o_class());
			if (iter != lvl.tie_classes.end() && iter->second.render_mesh.has_value()) {
				EditorClass& ec = iter->second;
				draw_mesh_instanced(*ec.render_mesh, ec.materials.data(), ec.materials.size(), ghost_inst_buffer.id, i, count);
				drawn = true;
			}
		} else if (first_type == INST_SHRUB) {
			auto iter = lvl.shrub_classes.find(first_inst->o_class());
			if (iter != lvl.shrub_classes.end() && iter->second.render_mesh.has_value()) {
				EditorClass& ec = iter->second;
				draw_mesh_instanced(*ec.render_mesh, ec.materials.data(), ec.materials.size(), ghost_inst_buffer.id, i, count);
				drawn = true;
			}
		}
		if (!drawn) {
			draw_cube_instanced(GL_LINE, white, ghost_inst_buffer.id, i, count);
		}
		inst_ptrs.clear();
		i = j;
	}
}

static void draw_instances(
	Level& lvl, GLenum mesh_mode, bool draw_wireframes, const RenderSettings& settings)
{
	if (settings.draw_moby_instances) {
		draw_moby_instances(lvl, lvl.instances().moby_instances, mesh_mode, GL_LINE);
	}
	
	if (settings.draw_moby_groups && draw_wireframes) {
		draw_cube_instanced(GL_LINE, white, moby_group_inst_buffer, 0, lvl.instances().moby_groups.size());
	}
	
	if (settings.draw_tie_instances) {
		draw_tie_instances(lvl, lvl.instances().tie_instances, mesh_mode, GL_LINE);
	}
	
	if (settings.draw_tie_groups && draw_wireframes) {
		draw_cube_instanced(GL_LINE, white, tie_group_inst_buffer, 0, lvl.instances().tie_groups.size());
	}
	
	if (settings.draw_shrub_instances) {
		draw_shrub_instances(lvl, lvl.instances().shrub_instances, mesh_mode, GL_LINE);
	}
	
	if (settings.draw_shrub_groups && draw_wireframes) {
		draw_cube_instanced(GL_LINE, white, shrub_group_inst_buffer, 0, lvl.instances().shrub_groups.size());
	}
	
	if (settings.draw_point_lights && draw_wireframes) {
		draw_cube_instanced(GL_LINE, white, point_light_inst_buffer, 0, lvl.instances().point_lights.size());
	}
	
	if (settings.draw_env_sample_points && draw_wireframes) {
		draw_cube_instanced(GL_LINE, white, env_sample_point_inst_buffer, 0, lvl.instances().env_sample_points.size());
	}
	
	if (settings.draw_env_transitions && draw_wireframes) {
		draw_cube_instanced(GL_LINE, white, env_transition_inst_buffer, 0, lvl.instances().env_transitions.size());
	}
	
	if (settings.draw_cuboids && draw_wireframes) {
		draw_cube_instanced(GL_LINE, white, cuboid_inst_buffer, 0, lvl.instances().cuboids.size());
	}
	
	if (settings.draw_spheres && draw_wireframes) {
		draw_cube_instanced(GL_LINE, white, sphere_inst_buffer, 0, lvl.instances().spheres.size());
	}
	
	if (settings.draw_cylinders && draw_wireframes) {
		draw_cube_instanced(GL_LINE, white, cylinder_inst_buffer, 0, lvl.instances().cylinders.size());
	}
	
	if (settings.draw_pills && draw_wireframes) {
		draw_cube_instanced(GL_LINE, white, pill_inst_buffer, 0, lvl.instances().pills.size());
	}
	
	if (settings.draw_cameras && draw_wireframes) {
		draw_cube_instanced(GL_LINE, white, camera_inst_buffer, 0, lvl.instances().cameras.size());
	}
	
	if (settings.draw_sound_instances && draw_wireframes) {
		draw_cube_instanced(GL_LINE, white, sound_inst_buffer, 0, lvl.instances().sound_instances.size());
	}
	
	if (settings.draw_paths) {
		draw_paths(lvl.instances().paths, orange);
	}
	
	if (settings.draw_grind_paths) {
		draw_paths(lvl.instances().grind_paths, cyan);
	}
	
	if (settings.draw_areas && draw_wireframes) {
		draw_cube_instanced(GL_LINE, white, area_inst_buffer, 0, lvl.instances().areas.size());
	}
}

static void draw_moby_instances(
	Level& lvl, InstanceList<MobyInstance>& instances, GLenum mesh_mode, GLenum cube_mode)
{
	if (instances.size() < 1) {
		return;
	}
	
	size_t begin = 0;
	size_t end = 0;
	for (size_t i = 1; i <= instances.size(); i++) {
		s32 last_class = instances[i - 1].o_class();
		if (i == instances.size() || instances[i].o_class() != last_class) {
			end = i;
			auto iter = lvl.moby_classes.find(last_class);
			if (iter != lvl.moby_classes.end() && iter->second.render_mesh.has_value()) {
				EditorClass& cls = iter->second;
				glPolygonMode(GL_FRONT_AND_BACK, mesh_mode);
				draw_mesh_instanced(*cls.render_mesh, cls.materials.data(), cls.materials.size(), moby_inst_buffer, begin, end - begin);
			} else {
				draw_cube_instanced(cube_mode, white, moby_inst_buffer, begin, end - begin);
			}
			begin = i;
		}
	}
}

static void draw_tie_instances(
	Level& lvl, const InstanceList<TieInstance>& instances, GLenum mesh_mode, GLenum cube_mode)
{
	if (instances.size() < 1) {
		return;
	}
	
	size_t begin = 0;
	size_t end = 0;
	for (size_t i = 1; i <= instances.size(); i++) {
		s32 last_class = instances[i - 1].o_class();
		if (i == instances.size() || instances[i].o_class() != last_class) {
			end = i;
			auto iter = lvl.tie_classes.find(last_class);
			if (iter != lvl.tie_classes.end() && iter->second.render_mesh.has_value()) {
				EditorClass& cls = iter->second;
				glPolygonMode(GL_FRONT_AND_BACK, mesh_mode);
				draw_mesh_instanced(*cls.render_mesh, cls.materials.data(), cls.materials.size(), tie_inst_buffer, begin, end - begin);
			} else {
				draw_cube_instanced(cube_mode, purple, tie_inst_buffer, begin, end - begin);
			}
			begin = i;
		}
	}
}

static void draw_shrub_instances(
	Level& lvl, const InstanceList<ShrubInstance>& instances, GLenum mesh_mode, GLenum cube_mode)
{
	if (instances.size() < 1) {
		return;
	}
	
	size_t begin = 0;
	size_t end = 0;
	for (size_t i = 1; i <= instances.size(); i++) {
		s32 last_class = instances[i - 1].o_class();
		if (i == instances.size() || instances[i].o_class() != last_class) {
			end = i;
			auto iter = lvl.shrub_classes.find(last_class);
			if (iter != lvl.shrub_classes.end() && iter->second.render_mesh.has_value()) {
				EditorClass& cls = iter->second;
				glPolygonMode(GL_FRONT_AND_BACK, mesh_mode);
				draw_mesh_instanced(*cls.render_mesh, cls.materials.data(), cls.materials.size(), shrub_inst_buffer, begin, end - begin);
			} else {
				draw_cube_instanced(cube_mode, green, shrub_inst_buffer, begin, end - begin);
			}
			begin = i;
		}
	}
}

static void draw_selected_shrub_normals(Level& lvl)
{
	for (ShrubInstance& inst : lvl.instances().shrub_instances) {
		if (inst.selected && lvl.shrub_classes.find(inst.o_class()) != lvl.shrub_classes.end()) {
			const EditorClass& cls = lvl.shrub_classes.at(inst.o_class());
			if (cls.mesh.has_value()) {
				std::vector<Vertex> vertices;
				for (const Vertex& v : cls.mesh->vertices) {
					Vertex v2 = v;
					v2.pos += v2.normal * 0.5f;
					vertices.emplace_back(v);
					vertices.emplace_back(v2);
					vertices.emplace_back(v2);
				}
				
				RenderMesh mesh;
				RenderSubMesh& submesh = mesh.submeshes.emplace_back();
				submesh.material = 0;
				
				glGenBuffers(1, &submesh.vertex_buffer.id);
				glBindBuffer(GL_ARRAY_BUFFER, submesh.vertex_buffer.id);
				glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
				submesh.vertex_count = vertices.size();
				
				glm::mat4 matrix = inst.transform().matrix();
				
				auto inst_data = InstanceData(matrix, glm::vec4(0.f, 0.f, 1.f, 1.f), glm::vec4(1.f));
				GlBuffer inst_buffer;
				glGenBuffers(1, &inst_buffer.id);
				glBindBuffer(GL_ARRAY_BUFFER, inst_buffer.id);
				glBufferData(GL_ARRAY_BUFFER, sizeof(inst_data), &inst_data, GL_STATIC_DRAW);
				
				draw_mesh_instanced(mesh, &white, 1, inst_buffer.id, 0, 1);
			}
		}
	}
}

static void draw_selected_moby_normals(Level& lvl)
{
	for (MobyInstance& inst : lvl.instances().moby_instances) {
		if (inst.selected && lvl.moby_classes.find(inst.o_class()) != lvl.moby_classes.end()) {
			const EditorClass& cls = lvl.moby_classes.at(inst.o_class());
			if (cls.mesh.has_value()) {
				std::vector<Vertex> vertices;
				for (const Vertex& v : cls.mesh->vertices) {
					Vertex v2 = v;
					v2.pos += v2.normal * 0.5f;
					vertices.emplace_back(v);
					vertices.emplace_back(v2);
					vertices.emplace_back(v2);
				}
				
				RenderMesh mesh;
				RenderSubMesh& submesh = mesh.submeshes.emplace_back();
				submesh.material = 0;
				
				glGenBuffers(1, &submesh.vertex_buffer.id);
				glBindBuffer(GL_ARRAY_BUFFER, submesh.vertex_buffer.id);
				glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
				submesh.vertex_count = vertices.size();
				
				auto inst_data = InstanceData(inst.transform().matrix(), glm::vec4(0.f, 0.f, 1.f, 1.f), glm::vec4(1.f));
				GlBuffer inst_buffer;
				glGenBuffers(1, &inst_buffer.id);
				glBindBuffer(GL_ARRAY_BUFFER, inst_buffer.id);
				glBufferData(GL_ARRAY_BUFFER, sizeof(inst_data), &inst_data, GL_STATIC_DRAW);
				
				draw_mesh_instanced(mesh, &white, 1, inst_buffer.id, 0, 1);
			}
		}
	}
}

template <typename ThisPath>
static void draw_paths(const InstanceList<ThisPath>& paths, const RenderMaterial& material)
{
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	
	for (const ThisPath& path : paths) {
		if (path.spline().size() < 1) {
			continue;
		}
		
		RenderMesh mesh;
		RenderSubMesh& submesh = mesh.submeshes.emplace_back();
		submesh.material = 0;
		
		std::vector<Vertex> vertices;
		for (size_t i = 0; i < path.spline().size() - 1; i++) {
			vertices.emplace_back(path.spline()[i]);
			vertices.emplace_back(path.spline()[i + 1]);
			vertices.emplace_back(path.spline()[i + 1]);
		}
		
		glGenBuffers(1, &submesh.vertex_buffer.id);
		glBindBuffer(GL_ARRAY_BUFFER, submesh.vertex_buffer.id);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
		submesh.vertex_count = vertices.size();
		
		auto inst = InstanceData(glm::mat4(1.f), inst_colour(path), encode_inst_id(path.id()));
		GlBuffer inst_buffer;
		glGenBuffers(1, &inst_buffer.id);
		glBindBuffer(GL_ARRAY_BUFFER, inst_buffer.id);
		glBufferData(GL_ARRAY_BUFFER, sizeof(inst), &inst, GL_STATIC_DRAW);
		
		draw_mesh_instanced(mesh, &material, 1, inst_buffer.id, 0, 1);
	}
}

static void draw_icons(Level& lvl, const RenderSettings& settings)
{
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	
	if (settings.draw_moby_instances) {
		draw_moby_icons(lvl, lvl.instances().moby_instances);
	}
	
	if (settings.draw_moby_groups) {
		draw_icon_instanced(INST_MOBYGROUP, moby_group_inst_buffer, 0, lvl.instances().moby_groups.size());
	}
	
	if (settings.draw_tie_groups) {
		draw_icon_instanced(INST_TIEGROUP, tie_group_inst_buffer, 0, lvl.instances().tie_groups.size());
	}
	
	if (settings.draw_shrub_groups) {
		draw_icon_instanced(INST_SHRUBGROUP, shrub_group_inst_buffer, 0, lvl.instances().shrub_groups.size());
	}
	
	if (settings.draw_point_lights) {
		draw_icon_instanced(INST_POINTLIGHT, point_light_inst_buffer, 0, lvl.instances().point_lights.size());
	}
	
	if (settings.draw_env_sample_points) {
		draw_icon_instanced(INST_ENVSAMPLEPOINT, env_sample_point_inst_buffer, 0, lvl.instances().env_sample_points.size());
	}
	
	if (settings.draw_env_transitions) {
		draw_icon_instanced(INST_ENVTRANSITION, env_transition_inst_buffer, 0, lvl.instances().env_transitions.size());
	}
	
	if (settings.draw_cuboids) {
		draw_icon_instanced(INST_CUBOID, cuboid_inst_buffer, 0, lvl.instances().cuboids.size());
	}
	
	if (settings.draw_spheres) {
		draw_icon_instanced(INST_SPHERE, sphere_inst_buffer, 0, lvl.instances().spheres.size());
	}
	
	if (settings.draw_cylinders) {
		draw_icon_instanced(INST_CYLINDER, cylinder_inst_buffer, 0, lvl.instances().cylinders.size());
	}
	
	if (settings.draw_pills) {
		draw_icon_instanced(INST_PILL, pill_inst_buffer, 0, lvl.instances().pills.size());
	}
	
	if (settings.draw_cameras) {
		draw_icon_instanced(INST_CAMERA, camera_inst_buffer, 0, lvl.instances().cameras.size());
	}
	
	if (settings.draw_sound_instances) {
		draw_icon_instanced(INST_SOUND, sound_inst_buffer, 0, lvl.instances().sound_instances.size());
	}
	
	if (settings.draw_areas) {
		draw_icon_instanced(INST_AREA, area_inst_buffer, 0, lvl.instances().areas.size());
	}
}

static void draw_moby_icons(Level& lvl, InstanceList<MobyInstance>& instances)
{
	if (instances.size() < 1) {
		return;
	}
	
	size_t begin = 0;
	size_t end = 0;
	for (size_t i = 1; i <= instances.size(); i++) {
		s32 last_class = instances[i - 1].o_class();
		if (i == instances.size() || instances[i].o_class() != last_class) {
			end = i;
			auto iter = lvl.moby_classes.find(last_class);
			if (iter == lvl.moby_classes.end() || !iter->second.render_mesh.has_value()) {
				if (iter != lvl.moby_classes.end() && iter->second.icon.has_value()) {
					draw_mesh_instanced(quad, &(*iter->second.icon), 1, moby_inst_buffer, begin, end - begin);
				} else {
					draw_icon_instanced(INST_MOBY, moby_inst_buffer, begin, end - begin);
				}
			}
			begin = i;
		}
	}
}

static void draw_cube_instanced(
	GLenum cube_mode,
	const RenderMaterial& material,
	GLuint inst_buffer,
	size_t inst_begin,
	size_t inst_count)
{
	glPolygonMode(GL_FRONT_AND_BACK, cube_mode);
	if (cube_mode == GL_FILL) {
		draw_mesh_instanced(fill_cube, &material, 1, inst_buffer, inst_begin, inst_count);
	} else {
		draw_mesh_instanced(line_cube, &material, 1, inst_buffer, inst_begin, inst_count);
	}
}

static void draw_icon_instanced(s32 type, GLuint inst_buffer, size_t inst_begin, size_t inst_count) {
	draw_mesh_instanced(quad, &instance_icons[type], 1, inst_buffer, inst_begin, inst_count);
}

static void draw_mesh(
	const RenderMesh& mesh,
	const RenderMaterial* mats,
	size_t mat_count,
	const glm::mat4& local_to_world)
{
	auto inst = InstanceData(local_to_world, {}, {});
	inst.colour = glm::vec4(1.f, 1.f, 1.f, 1.f);
	
	GlBuffer inst_buffer;
	glGenBuffers(1, &inst_buffer.id);
	glBindBuffer(GL_ARRAY_BUFFER, inst_buffer.id);
	glBufferData(GL_ARRAY_BUFFER, sizeof(inst), &inst, GL_STATIC_DRAW);
	
	draw_mesh_instanced(mesh, mats, mat_count, inst_buffer.id, 0, 1);
}

static void draw_mesh_instanced(
	const RenderMesh& mesh,
	const RenderMaterial* mats,
	size_t mat_count,
	GLuint inst_buffer,
	size_t inst_begin,
	size_t inst_count)
{
	size_t inst_offset = inst_begin * sizeof(InstanceData);
	size_t matrix_offset = inst_offset + offsetof(InstanceData, matrix);
	
	// This probably isn't ideal.
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	
	glBindBuffer(GL_ARRAY_BUFFER, inst_buffer);
	
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*) (matrix_offset + sizeof(glm::vec4) * 0));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*) (matrix_offset + sizeof(glm::vec4) * 1));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*) (matrix_offset + sizeof(glm::vec4) * 2));
	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*) (matrix_offset + sizeof(glm::vec4) * 3));
	glEnableVertexAttribArray(4);
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*) (inst_offset + offsetof(InstanceData, colour)));
	glEnableVertexAttribArray(5);
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void*) (inst_offset + offsetof(InstanceData, id)));
	
	glVertexAttribDivisor(0, 1);
	glVertexAttribDivisor(1, 1);
	glVertexAttribDivisor(2, 1);
	glVertexAttribDivisor(3, 1);
	glVertexAttribDivisor(4, 1);
	glVertexAttribDivisor(5, 1);
	
	for (const RenderSubMesh& submesh : mesh.submeshes) {
		if (submesh.material >= mat_count) {
			continue;
		}
		
		glBindBuffer(GL_ARRAY_BUFFER, submesh.vertex_buffer.id);
		
		glEnableVertexAttribArray(6);
		glVertexAttribPointer(6, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, pos));
		glEnableVertexAttribArray(7);
		glVertexAttribPointer(7, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, normal));
		glEnableVertexAttribArray(8);
		glVertexAttribPointer(8, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*) offsetof(Vertex, tex_coord));
		
		const RenderMaterial& material = mats[submesh.material];
		
		if (program == shaders.textured.id()) {
			const glm::vec4& colour = material.colour;
			glUniform4f(shaders.textured_colour, colour.r, colour.g, colour.b, colour.a);
		
			glActiveTexture(GL_TEXTURE0);
			if (material.texture.id > 0) {
				glBindTexture(GL_TEXTURE_2D, material.texture.id);
			} else {
				glBindTexture(GL_TEXTURE_2D, white.texture.id);
			}
			glUniform1i(shaders.textured_sampler, 0);
		} else if (program == shaders.icons.id()) {
			glActiveTexture(GL_TEXTURE0);
			if (material.texture.id > 0) {
				glBindTexture(GL_TEXTURE_2D, material.texture.id);
			} else {
				glBindTexture(GL_TEXTURE_2D, white.texture.id);
			}
			glUniform1i(shaders.icons_sampler, 0);
		}
		
		glDrawArraysInstanced(GL_TRIANGLES, 0, submesh.vertex_count, (GLsizei) inst_count);
		
		glDisableVertexAttribArray(8);
		glDisableVertexAttribArray(7);
		glDisableVertexAttribArray(6);
	}
	
	glDisableVertexAttribArray(5);
	glDisableVertexAttribArray(4);
	glDisableVertexAttribArray(3);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(0);

	glVertexAttribDivisor(5, 0);
	glVertexAttribDivisor(4, 0);
	glVertexAttribDivisor(3, 0);
	glVertexAttribDivisor(2, 0);
	glVertexAttribDivisor(1, 0);
	glVertexAttribDivisor(0, 0);
	
	glDeleteVertexArrays(1, &vao);
}

glm::mat4 compose_view_matrix(const glm::vec3& cam_pos, const glm::vec2& cam_rot)
{
	glm::mat4 pitch = glm::rotate(glm::mat4(1.0f), cam_rot.x, glm::vec3(0.0f, -1.0f, 0.0f));
	glm::mat4 yaw = glm::rotate(glm::mat4(1.0f), cam_rot.y, glm::vec3(0.0f, 0.0f, 1.0f));
	glm::mat4 translate = glm::translate(glm::mat4(1.0f), -cam_pos);
	return pitch * yaw * translate;
}

glm::mat4 compose_projection_matrix(const ImVec2& view_size)
{
	return glm::perspective(glm::radians(45.0f), view_size.x / view_size.y, 0.1f, 10000.0f);
}

glm::vec3 apply_local_to_screen(const glm::mat4& world_to_clip, const glm::mat4& local_to_world, const ImVec2& view_size)
{
	glm::mat4 local_to_clip = world_to_clip * glm::translate(glm::mat4(1.f), glm::vec3(1.f));
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

glm::vec3 create_ray(
	const glm::mat4& world_to_clip,
	const ImVec2& screen_pos,
	const ImVec2& view_pos,
	const ImVec2& view_size)
{
	auto imgui_to_glm = [](ImVec2 v) { return glm::vec2(v.x, v.y); };
	glm::vec2 relative_pos = imgui_to_glm(screen_pos) - imgui_to_glm(view_pos);
	glm::vec2 device_space_pos = 2.f * relative_pos / imgui_to_glm(view_size) - 1.f;
	glm::vec4 clip_pos(device_space_pos.x, device_space_pos.y, 1.f, 1.f);
	glm::mat4 clip_to_world = glm::inverse(world_to_clip);
	glm::vec4 world_pos = clip_to_world * clip_pos;
	glm::vec3 direction = glm::normalize(glm::vec3(world_pos));
	return direction;
}

void reset_camera(app* a)
{
	a->render_settings.camera_rotation = glm::vec3(0, 0, 0);
	if (Level* lvl = a->get_level()) {
		if (!lvl->instances().moby_instances.empty()) {
			a->render_settings.camera_position = lvl->instances().moby_instances[0].transform().pos();
		} else {
			a->render_settings.camera_position = lvl->instances().level_settings.ship_pos;
		}
	} else {
		a->render_settings.camera_position = glm::vec3(0, 0, 0);
	}
}

static Mesh create_fill_cube()
{
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
	submesh.material = 0;
	
	submesh.faces.emplace_back(0, 1, 3, 2);
	submesh.faces.emplace_back(4, 5, 7, 6);
	submesh.faces.emplace_back(0, 1, 5, 4);
	submesh.faces.emplace_back(2, 3, 7, 6);
	submesh.faces.emplace_back(0, 2, 6, 4);
	submesh.faces.emplace_back(1, 3, 7, 5);
	
	return mesh;
}

static Mesh create_line_cube()
{
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
	submesh.material = 0;
	
	submesh.faces.emplace_back(0, 4, 4);
	submesh.faces.emplace_back(0, 2, 2);
	submesh.faces.emplace_back(0, 1, 1);
	submesh.faces.emplace_back(7, 3, 3);
	submesh.faces.emplace_back(7, 5, 5);
	submesh.faces.emplace_back(7, 6, 6);
	
	submesh.faces.emplace_back(4, 5, 5);
	submesh.faces.emplace_back(4, 6, 6);
	submesh.faces.emplace_back(2, 3, 3);
	submesh.faces.emplace_back(2, 6, 6);
	submesh.faces.emplace_back(1, 3, 3);
	submesh.faces.emplace_back(1, 5, 5);
	
	return mesh;
}

static Mesh create_quad()
{
	Mesh mesh;
	
	ColourAttribute dummy{};
	mesh.vertices.emplace_back(glm::vec3(-1.f, -1.f, 0.f), dummy, glm::vec2(0.f, 0.f));
	mesh.vertices.emplace_back(glm::vec3(1.f, -1.f, 0.f), dummy, glm::vec2(1.f, 0.f));
	mesh.vertices.emplace_back(glm::vec3(-1.f, 1.f, 0.f), dummy, glm::vec2(0.f, 1.f));
	mesh.vertices.emplace_back(glm::vec3(1.f, 1.f, 0.f), dummy, glm::vec2(1.f, 1.f));
	
	SubMesh& submesh = mesh.submeshes.emplace_back();
	submesh.material = 0;
	
	submesh.faces.emplace_back(0, 1, 2);
	submesh.faces.emplace_back(3, 2, 1);
	
	return mesh;
}

static Texture create_white_texture()
{
	return Texture::create_rgba(1, 1, {0xff, 0xff, 0xff, 0xff});
}

static void set_shader(const Shader& shader)
{
	glUseProgram(shader.id());
	program = shader.id();
}
