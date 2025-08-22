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

#include "collada.h"

#include <map>
#include <ctype.h>
#include <algorithm>
#include <rapidxml/rapidxml.hpp>

#include <core/mesh.h>

using XmlDocument = rapidxml::xml_document<>;
using XmlNode = rapidxml::xml_node<>;
using XmlAttrib = rapidxml::xml_attribute<>;
#define xml_for_each_child_of_type(child, parent, name) \
	for (const XmlNode* child = parent->first_node(name); child != nullptr; child = child->next_sibling(name))

using IdMap = std::map<std::string, const XmlNode*>;
using NodeToIndexMap = std::map<const XmlNode*, s32>;
static ColladaMaterial read_material(const XmlNode* material_node, const IdMap& ids, const NodeToIndexMap& images);
using JointSidsMap = std::map<std::tuple<const XmlNode*, std::string>, s32>; // (skeleton, joint name) -> joint id
struct VertexData {
	Opt<std::vector<f32>> positions;
	Opt<std::vector<f32>> normals;
	Opt<std::vector<f32>> colours;
	Opt<std::vector<f32>> tex_coords;
};
static VertexData read_vertices(const XmlNode* geometry, const IdMap& ids);
static std::vector<f32> read_vertex_source(const XmlNode* source, const IdMap& ids);
static std::vector<SkinAttributes> read_skin(Mesh& mesh, const XmlNode* controller, const XmlNode* skeleton, const IdMap& ids, const JointSidsMap& joint_sids);
static void read_submeshes(Mesh& mesh, const XmlNode* instance, const XmlNode* geometry, const XmlNode* controller, const IdMap& ids, const NodeToIndexMap& materials, const VertexData& vertex_data, const std::vector<SkinAttributes>& skin_data);
struct CreateVertexInput {
	const std::vector<f32>* positions = nullptr;
	const std::vector<f32>* normals = nullptr;
	const std::vector<f32>* colours = nullptr;
	const std::vector<f32>* tex_coords = nullptr;
	const std::vector<SkinAttributes>* skin_data = nullptr;
	s32 position_offset = -1;
	s32 normal_offset = -1;
	s32 colour_offset = -1;
	s32 tex_coord_offset = -1;
};
static Vertex create_vertex(const std::vector<s32>& indices, s32 base, const CreateVertexInput& input);
static std::vector<f32> read_float_array(const XmlNode* float_array);
static s32 read_s32(const char*& input, const char* context);
static void enumerate_ids(std::map<std::string, const XmlNode*>& ids, const XmlNode* node);
static void enumerate_joint_sids(JointSidsMap& joint_sids, s32& next_joint, const XmlNode* skeleton, const XmlNode* node);
static const XmlNode* xml_child(const XmlNode* node, const char* name);
static const XmlAttrib* xml_attrib(const XmlNode* node, const char* name);
static const XmlNode* node_from_id(const IdMap& map, const char* id);

static void write_asset_metadata(OutBuffer dest);
static void write_images(OutBuffer dest, const std::vector<std::string>& texture_paths);
static void write_effects(OutBuffer dest, const std::vector<ColladaMaterial>& materials, size_t texture_count);
static void write_materials(OutBuffer dest, const std::vector<ColladaMaterial>& materials);
static void write_geometries(OutBuffer dest, const std::vector<Mesh>& meshes);
static void write_controllers(OutBuffer dest, const std::vector<Mesh>& meshes, const std::vector<Joint>& joints);
static void write_visual_scenes(OutBuffer dest, const ColladaScene& scene);
static void write_joint_node(OutBuffer dest, const std::vector<Joint>& joints, s32 index, s32 indent);
static void write_matrix4x4(OutBuffer dest, const glm::mat4& matrix);

Mesh* ColladaScene::find_mesh(const std::string& name)
{
	for (Mesh& mesh : meshes) {
		if (mesh.name == name) {
			return &mesh;
		}
	}
	return nullptr;
}

ColladaScene read_collada(char* src)
{
	XmlDocument doc;
	try {
		doc.parse<0>(src);
	} catch(rapidxml::parse_error& err) {
		verify_not_reached("%s", err.what());
	}
	
	const XmlNode* root = xml_child(&doc, "COLLADA");
	
	IdMap ids;
	enumerate_ids(ids, root);
	
	ColladaScene scene;
	
	NodeToIndexMap images;
	const XmlNode* library_images = root->first_node("library_images");
	if (library_images) {
		xml_for_each_child_of_type(image, library_images, "image") {
			scene.texture_paths.emplace_back(xml_child(image, "init_from")->value());
			images.emplace(image, scene.texture_paths.size() - 1);
		}
	}
	
	NodeToIndexMap materials;
	const XmlNode* library_materials = xml_child(root, "library_materials");
	xml_for_each_child_of_type(material, library_materials, "material") {
		scene.materials.emplace_back(read_material(material, ids, images));
		materials.emplace(material, scene.materials.size() - 1);
	}
	
	const XmlNode* library_visual_scenes = xml_child(root, "library_visual_scenes");
	const XmlNode* visual_scene = xml_child(library_visual_scenes, "visual_scene");
	
	JointSidsMap joint_sids;
	s32 next_joint = 0;
	xml_for_each_child_of_type(node, visual_scene, "node") {
		const XmlAttrib* type = node->first_attribute("type");
		if (type != nullptr && strcmp(type->value(), "JOINT") == 0) {
			enumerate_joint_sids(joint_sids, next_joint, node, node);
		}
	}
	
	xml_for_each_child_of_type(node, visual_scene, "node") {
		const XmlNode* instance = node->first_node("instance_controller");
		const XmlNode* controller;
		const XmlNode* geometry;
		const XmlNode* skeleton;
		if (instance) {
			controller = node_from_id(ids, xml_attrib(instance, "url")->value());
			const XmlNode* skin = xml_child(controller, "skin");
			geometry = node_from_id(ids, xml_attrib(skin, "source")->value());
			const char* skeleton_id = xml_child(instance, "skeleton")->value();
			auto skeleton_iter = ids.find(skeleton_id);
			verify(skeleton_iter != ids.end(), "Bad skeleton ID '%s'.", skeleton_id);
			skeleton = skeleton_iter->second;
		} else {
			instance = node->first_node("instance_geometry");
			if (!instance) {
				continue;
			}
			controller = nullptr;
			geometry = node_from_id(ids, xml_attrib(instance, "url")->value());
			skeleton = nullptr;
		}
		
		Mesh mesh;
		mesh.name = xml_attrib(node, "name")->value();
		auto vertex_data = read_vertices(geometry, ids);
		std::vector<SkinAttributes> skin_data;
		if (controller) {
			skin_data = read_skin(mesh, controller, skeleton, ids, joint_sids);
		}
		if (vertex_data.positions.has_value()) {
			read_submeshes(mesh, instance, geometry, controller, ids, materials, vertex_data, skin_data);
		}
		scene.meshes.emplace_back(deduplicate_vertices(std::move(mesh)));
	}
	return scene;
}

static ColladaMaterial read_material(const XmlNode* material_node, const IdMap& ids, const NodeToIndexMap& images)
{
	// Follow the white rabbit (it's white because its texture couldn't be loaded).
	const XmlNode* instance_effect = xml_child(material_node, "instance_effect");
	const XmlNode* effect = node_from_id(ids, xml_attrib(instance_effect, "url")->value());
	verify(strcmp(effect->name(), "effect") == 0, "Effect referenced by id is not an <effect> node.");
	const XmlNode* profile = effect->first_node();
	verify(profile, "<%s> node has no children.", effect->name());
	const XmlNode* technique = xml_child(profile, "technique");
	const XmlNode* shader = technique->first_node();
	verify(shader, "<%s> node has no children.", technique->name());
	const XmlNode* diffuse = xml_child(shader, "diffuse");
	if (const XmlNode* texture = diffuse->first_node("texture")) {
		const char* sampler_sid = xml_attrib(texture, "texture")->value();
		const XmlNode* sampler = nullptr;
		xml_for_each_child_of_type(newparam, profile, "newparam") {
			if (strcmp(xml_attrib(newparam, "sid")->value(), sampler_sid) == 0) {
				sampler = xml_child(newparam, "sampler2D");
				break;
			}
		}
		verify(sampler, "Unable to find sampler '%s'.", sampler_sid);
		const char* surface_sid = xml_child(sampler, "source")->value();
		const XmlNode* surface = nullptr;
		xml_for_each_child_of_type(newparam, profile, "newparam") {
			if (strcmp(xml_attrib(newparam, "sid")->value(), surface_sid) == 0) {
				surface = xml_child(newparam, "surface");
				break;
			}
		}
		verify(surface, "Unable to find surface '%s'.", surface_sid);
		auto image_id = std::string("#") + xml_child(surface, "init_from")->value();
		const XmlNode* image = node_from_id(ids, image_id.c_str());
		auto texture_index = images.find(image);
		verify(texture_index != images.end(), "An <image> node that was referenced with id '%s' cannot be found.", image_id.c_str());
		ColladaMaterial material;
		material.name = xml_attrib(material_node, "name")->value();
		material.surface = MaterialSurface(texture_index->second);
		return material;
	} else if (const XmlNode* colour = diffuse->first_node("color")) {
		glm::vec4 value;
		const char* r_ptr = colour->value();
		char* g_ptr;
		value.r = strtof(r_ptr, &g_ptr);
		verify(g_ptr != r_ptr, "<color> node has invalid body.");
		char* b_ptr;
		value.g = strtof(g_ptr, &b_ptr);
		verify(b_ptr != g_ptr, "<color> node has invalid body.");
		char* a_ptr;
		value.b = strtof(b_ptr, &a_ptr);
		verify(a_ptr != b_ptr, "<color> node has invalid body.");
		char* end_ptr;
		value.a = strtof(a_ptr, &end_ptr);
		verify(end_ptr != a_ptr, "<color> node has invalid body.");
		ColladaMaterial material;
		material.name = xml_attrib(material_node, "name")->value();
		material.surface = MaterialSurface(value);
		return material;
	}
	verify_not_reached("<diffuse> node needs either a <texture> or <color> node as a child.");
}

static VertexData read_vertices(const XmlNode* geometry, const IdMap& ids)
{
	const XmlNode* mesh_node = xml_child(geometry, "mesh");
	const XmlNode* triangles = mesh_node->first_node("triangles");
	const XmlNode* polylist = mesh_node->first_node("polylist");
	const XmlNode* indices = triangles != nullptr ? triangles : polylist;
	const char* mesh_name = xml_attrib(geometry, "id")->value();
	if (indices == nullptr) {
		return {};
	}
	
	const XmlNode* vertices = nullptr;
	const XmlNode* normals_source = nullptr;
	const XmlNode* colours_source = nullptr;
	const XmlNode* tex_coords_source = nullptr;
	xml_for_each_child_of_type(input, indices, "input") {
		const XmlAttrib* semantic = xml_attrib(input, "semantic");
		if (strcmp(semantic->value(), "VERTEX") == 0) {
			vertices = node_from_id(ids, xml_attrib(input, "source")->value());
		}
		if (strcmp(semantic->value(), "NORMAL") == 0) {
			normals_source = node_from_id(ids, xml_attrib(input, "source")->value());
		}
		if (strcmp(semantic->value(), "COLOR") == 0) {
			colours_source = node_from_id(ids, xml_attrib(input, "source")->value());
		}
		if (strcmp(semantic->value(), "TEXCOORD") == 0) {
			tex_coords_source = node_from_id(ids, xml_attrib(input, "source")->value());
		}
	}
	verify(vertices, "<triangles> node missing VERTEX input.");
	
	const XmlNode* positions_source = nullptr;
	xml_for_each_child_of_type(input, vertices, "input") {
		const XmlAttrib* semantic = xml_attrib(input, "semantic");
		if (strcmp(semantic->value(), "POSITION") == 0) {
			positions_source = node_from_id(ids, xml_attrib(input, "source")->value());
		}
	}
	verify(positions_source, "<vertices> node missing POSITIONS input.");
	
	auto positions = read_vertex_source(positions_source, ids);
	verify(positions.size() % 3 == 0, "Vertex positions array for mesh '%s' has a bad size (not divisible by 3).", mesh_name);
	
	Opt<std::vector<f32>> normals;
	if (normals_source) {
		normals = read_vertex_source(normals_source, ids);
		verify(normals->size() % 3 == 0, "Normals array for mesh '%s' has a bad size (not divisible by 3).", mesh_name);
	}
	
	Opt<std::vector<f32>> colours;
	if (colours_source) {
		colours = read_vertex_source(colours_source, ids);
		verify(colours->size() % 4 == 0, "Vertex colours array for mesh '%s' has a bad size (not divisible by 4).", mesh_name);
	}
	
	Opt<std::vector<f32>> tex_coords;
	if (tex_coords_source) {
		tex_coords = read_vertex_source(tex_coords_source, ids);
		verify(tex_coords->size() % 2 == 0, "Texture coordinates array for mesh '%s' has a bad size (not divisible by 2).", mesh_name);
	}
	
	return {positions, normals, colours, tex_coords};
}

static std::vector<f32> read_vertex_source(const XmlNode* source, const IdMap& ids)
{
	const XmlNode* technique_common = xml_child(source, "technique_common");
	const XmlNode* accessor = xml_child(technique_common, "accessor");
	const XmlNode* float_array = node_from_id(ids, xml_attrib(accessor, "source")->value());
	verify(strcmp(float_array->name(), "float_array") == 0, "Only <float_array> nodes are supported for storing vertex attributes.");
	return read_float_array(float_array);
}

static std::vector<SkinAttributes> read_skin(Mesh& mesh, const XmlNode* controller, const XmlNode* skeleton, const IdMap& ids, const JointSidsMap& joint_sids)
{
	const XmlNode* skin = xml_child(controller, "skin");
	const XmlNode* vertex_weights = xml_child(skin, "vertex_weights");
	
	s32 vertex_weight_count = atoi(xml_attrib(vertex_weights, "count")->value());
	
	const XmlNode* joints_source = nullptr;
	const XmlNode* weights_source = nullptr;
	s32 joint_offset = 0;
	s32 weight_offset = 0;
	xml_for_each_child_of_type(input, vertex_weights, "input") {
		const XmlAttrib* semantic = xml_attrib(input, "semantic");
		if (strcmp(semantic->value(), "JOINT") == 0) {
			joints_source = node_from_id(ids, xml_attrib(input, "source")->value());
			joint_offset = atoi(xml_attrib(input, "offset")->value());
		}
		if (strcmp(semantic->value(), "WEIGHT") == 0) {
			weights_source = node_from_id(ids, xml_attrib(input, "source")->value());
			weight_offset = atoi(xml_attrib(input, "offset")->value());
		}
	}
	verify(joints_source, "<vertex_weights> node missing JOINT input.");
	verify(weights_source, "<vertex_weights> node missing WEIGHT input.");
	
	s32 stride = std::max(joint_offset, weight_offset) + 1;
	
	const XmlNode* joints_name_array = xml_child(joints_source, "Name_array");
	std::vector<s32> joints;
	joints.resize(atoi(xml_attrib(joints_name_array, "count")->value()));
	const char* joint_ptr = joints_name_array->value();
	for (s32& joint_index : joints) {
		std::string joint_name;
		while (*joint_ptr != ' ' && *joint_ptr != '\0') {
			joint_name += *joint_ptr;
			joint_ptr++;
		}
		auto joint_iter = joint_sids.find({skeleton, joint_name});
		if (joint_iter == joint_sids.end()) {
			verify(joint_iter != joint_sids.end(), "Bad joint name or skeleton.");
		}
		verify(joint_iter->second < 256, "Too many joints.");
		joint_index = joint_iter->second;
		while (*joint_ptr == ' ') {
			joint_ptr++;
		}
	}
	
	const XmlNode* weights_float_array = xml_child(weights_source, "float_array");
	auto weights = read_float_array(weights_float_array);
	
	std::vector<s32> vcount_data;
	const char* vcount = xml_child(vertex_weights, "vcount")->value();
	for (s32 i = 0; i < vertex_weight_count; i++) {
		s32 vc = read_s32(vcount, "<vcount> node");
		verify(vc >= 0 && vc <= 3, "Only between 0 and 3 joints weights are supported for each vertex.");
		vcount_data.push_back(vc);
	}
	
	std::vector<SkinAttributes> skin_data(vertex_weight_count);
	
	const char* v = xml_child(vertex_weights, "v")->value();
	std::vector<s32> v_data;
	for (s32 i = 0; i < vertex_weight_count; i++) {
		SkinAttributes& attribs = skin_data[i];
		attribs.count = (u8) vcount_data[i];
		for (u8 j = 0; j < attribs.count * stride; j++) {
			v_data.push_back(read_s32(v, "<v> data"));
		}
	}
	
	size_t v_index = 0;
	for (s32 i = 0; i < vertex_weight_count; i++) {
		SkinAttributes& attribs = skin_data[i];
		for (u8 j = 0; j < attribs.count; j++) {
			attribs.joints[j] = (u8) joints.at(v_data.at(v_index + joint_offset));
			attribs.weights[j] = (u8) (weights.at(v_data.at(v_index + weight_offset)) * 255.f);
			v_index += stride;
		}
	}
	
	return skin_data;
}

static void read_submeshes(
	Mesh& mesh,
	const XmlNode* instance,
	const XmlNode* geometry,
	const XmlNode* controller,
	const IdMap& ids,
	const NodeToIndexMap& materials,
	const VertexData& vertex_data,
	const std::vector<SkinAttributes>& skin_data)
{
	const XmlNode* bind_material = xml_child(instance, "bind_material");
	const XmlNode* technique_common = xml_child(bind_material, "technique_common");
	const XmlNode* mesh_node = xml_child(geometry, "mesh");
	for (const XmlNode* indices = mesh_node->first_node(); indices != nullptr; indices = indices->next_sibling()) {
		bool is_triangles = strcmp(indices->name(), "triangles") == 0;
		bool is_polylist = strcmp(indices->name(), "polylist") == 0;
		
		if (is_triangles || is_polylist) {
			s32 face_count = atoi(xml_attrib(indices, "count")->value());
			const char* material_symbol = xml_attrib(indices, "material")->value();
			
			// Find the material.
			s32 material = -1;
			xml_for_each_child_of_type(instance_material, technique_common, "instance_material") {
				if (strcmp(xml_attrib(instance_material, "symbol")->value(), material_symbol) == 0) {
					material = materials.at(node_from_id(ids, xml_attrib(instance_material, "target")->value()));
				}
			}
			verify(material != -1, "Missing <instance_material> node.");
			
			// Find the offsets of each <input> and the overall stride.
			s32 position_offset = -1;
			s32 normal_offset = -1;
			s32 colour_offset = -1;
			s32 tex_coord_offset = -1;
			xml_for_each_child_of_type(input, indices, "input") {
				if (strcmp(xml_attrib(input, "semantic")->value(), "VERTEX") == 0) {
					position_offset = atoi(xml_attrib(input, "offset")->value());
				}
				if (strcmp(xml_attrib(input, "semantic")->value(), "NORMAL") == 0) {
					normal_offset = atoi(xml_attrib(input, "offset")->value());
				}
				if (strcmp(xml_attrib(input, "semantic")->value(), "COLOR") == 0) {
					colour_offset = atoi(xml_attrib(input, "offset")->value());
				}
				if (strcmp(xml_attrib(input, "semantic")->value(), "TEXCOORD") == 0) {
					tex_coord_offset = atoi(xml_attrib(input, "offset")->value());
				}
			}
			s32 vertex_stride = std::max({position_offset, normal_offset, tex_coord_offset, colour_offset}) + 1;
			verify(position_offset > -1 && vertex_stride < 10, "Invalid or missing <input> node.");
			
			if (normal_offset > -1) {
				mesh.flags |= MESH_HAS_NORMALS;
			}
			if (tex_coord_offset > -1) {
				mesh.flags |= MESH_HAS_TEX_COORDS;
			}
			
			CreateVertexInput args {
				vertex_data.positions.has_value() ? &(*vertex_data.positions) : nullptr,
				vertex_data.normals.has_value() ? &(*vertex_data.normals) : nullptr,
				vertex_data.colours.has_value() ? &(*vertex_data.colours) : nullptr,
				vertex_data.tex_coords.has_value() ? &(*vertex_data.tex_coords) : nullptr,
				skin_data.size() > 0 ? &skin_data : nullptr,
				position_offset, normal_offset, colour_offset, tex_coord_offset
			};
			
			// Because of the permissive nature of the COLLADA format, here we
			// just add a new vertex for every index. We can deduplicate them
			// later if necessary.
			SubMesh submesh;
			submesh.material = material;
			if (is_triangles) {
				const char* p = xml_child(indices, "p")->value();
				std::vector<s32> index_data;
				for (s32 i = 0; i < face_count * 3 * vertex_stride; i++) {
					index_data.push_back(read_s32(p, "<p> node"));
				}
				
				s32 i = 0;
				for (s32 face = 0; face < face_count; face++) {
					s32 index = (s32) mesh.vertices.size();
					mesh.vertices.emplace_back(create_vertex(index_data, i + vertex_stride * 0, args));
					mesh.vertices.emplace_back(create_vertex(index_data, i + vertex_stride * 1, args));
					mesh.vertices.emplace_back(create_vertex(index_data, i + vertex_stride * 2, args));
					submesh.faces.emplace_back(index, index + 1, index + 2);
					i += vertex_stride * 3;
				}
			} else {
				const char* vcount = xml_child(indices, "vcount")->value();
				std::vector<s32> vcount_data;
				for (s32 i = 0; i < face_count; i++) {
					s32 vc = read_s32(vcount, "<vcount> node");
					verify(vc == 3 || vc == 4, "Only tris and quads are supported.");
					vcount_data.push_back(vc);
				}
				
				const char* p = xml_child(indices, "p")->value();
				std::vector<s32> index_data;
				for (s32 i = 0; i < face_count; i++) {
					for (s32 j = 0; j < vcount_data[i] * vertex_stride; j++) {
						index_data.push_back(read_s32(p, "<p> node"));
					}
				}
				
				mesh.flags |= MESH_HAS_QUADS;
				s32 i = 0;
				for (s32 face = 0; face < face_count; face++) {
					s32 index = (s32) mesh.vertices.size();
					if (vcount_data[face] == 3) {
						mesh.vertices.emplace_back(create_vertex(index_data, i + vertex_stride * 0, args));
						mesh.vertices.emplace_back(create_vertex(index_data, i + vertex_stride * 1, args));
						mesh.vertices.emplace_back(create_vertex(index_data, i + vertex_stride * 2, args));
						submesh.faces.emplace_back(index, index + 1, index + 2);
					} else {
						mesh.vertices.emplace_back(create_vertex(index_data, i + vertex_stride * 0, args));
						mesh.vertices.emplace_back(create_vertex(index_data, i + vertex_stride * 1, args));
						mesh.vertices.emplace_back(create_vertex(index_data, i + vertex_stride * 2, args));
						mesh.vertices.emplace_back(create_vertex(index_data, i + vertex_stride * 3, args));
						submesh.faces.emplace_back(index, index + 1, index + 2, index + 3);
					}
					i += vertex_stride * vcount_data[face];
				}
			}
			mesh.submeshes.emplace_back(std::move(submesh));
		}
	}
}

static Vertex create_vertex(const std::vector<s32>& indices, s32 base, const CreateVertexInput& input)
{
	Vertex vertex(glm::vec3(0.f, 0.f, 0.f));
	if (input.position_offset > -1) {
		s32 position_index = indices.at(base + input.position_offset);
		vertex.pos.x = input.positions->at(position_index * 3 + 0);
		vertex.pos.y = input.positions->at(position_index * 3 + 1);
		vertex.pos.z = input.positions->at(position_index * 3 + 2);
		if (input.skin_data != nullptr) {
			vertex.skin = input.skin_data->at(position_index);
		}
	}
	if (input.normal_offset > -1) {
		s32 normal_index = indices.at(base + input.normal_offset);
		vertex.normal.x = input.normals->at(normal_index * 3 + 0);
		vertex.normal.y = input.normals->at(normal_index * 3 + 1);
		vertex.normal.z = input.normals->at(normal_index * 3 + 2);
	}
	if (input.colour_offset > -1) {
		s32 colour_index = indices.at(base + input.colour_offset);
		vertex.colour.r = (u8) (input.colours->at(colour_index * 4 + 0) * 255.f);
		vertex.colour.g = (u8) (input.colours->at(colour_index * 4 + 1) * 255.f);
		vertex.colour.b = (u8) (input.colours->at(colour_index * 4 + 2) * 255.f);
		vertex.colour.a = (u8) (input.colours->at(colour_index * 4 + 3) * 255.f);
	}
	if (input.tex_coord_offset > -1) {
		s32 tex_coord_index = indices.at(base + input.tex_coord_offset);
		vertex.tex_coord.x = input.tex_coords->at(tex_coord_index * 2 + 0);
		vertex.tex_coord.y = 1.f - input.tex_coords->at(tex_coord_index * 2 + 1);
	}
	return vertex;
}

static std::vector<f32> read_float_array(const XmlNode* float_array)
{
	std::vector<f32> data;
	data.resize(atoi(xml_attrib(float_array, "count")->value()));
	const char* ptr = float_array->value();
	for (f32& value : data) {
		char* next;
		value = strtof(ptr, &next);
		verify(next != ptr, "Failed to read <float_array>.");
		ptr = next;
	}
	return data;
}

static s32 read_s32(const char*& input, const char* context)
{
	char* next;
	s32 value = strtol(input, &next, 10);
	verify(next != input, "Failed to read integers from %s.", context);
	input = next;
	return value;
}

static void enumerate_ids(std::map<std::string, const XmlNode*>& ids, const XmlNode* node)
{
	for (XmlNode* child = node->first_node(); child != nullptr; child = child->next_sibling()) {
		XmlAttrib* id = child->first_attribute("id");
		if (id != nullptr) {
			ids[std::string("#") + id->value()] = child;
		}
		enumerate_ids(ids, child);
	}
}

static void enumerate_joint_sids(
	JointSidsMap& joint_sids, s32& next_joint, const XmlNode* skeleton, const XmlNode* node)
{
	const char* sid = xml_attrib(node, "sid")->value();
	joint_sids[{skeleton, std::string(sid)}] = next_joint++;
	
	xml_for_each_child_of_type(child, node, "node") {
		enumerate_joint_sids(joint_sids, next_joint, skeleton, child);
	}
}

static const XmlNode* xml_child(const XmlNode* node, const char* name)
{
	XmlNode* child = node->first_node(name);
	verify(child, "<%s> node missing <%s> child.", node->name(), name);
	return child;
}

static const XmlAttrib* xml_attrib(const XmlNode* node, const char* name)
{
	XmlAttrib* attrib = node->first_attribute(name);
	verify(attrib, "<%s> node missing %s attribute.", node->name(), name);
	return attrib;
}

static const XmlNode* node_from_id(const IdMap& map, const char* id)
{
	verify(*id == '#', "Only ids starting with # are supported ('%s' passed).", id);
	auto iter = map.find(id);
	verify(iter != map.end(), "No element with id equal to '%s'.", id);
	return iter->second;
}

void map_lhs_material_indices_to_rhs_list(ColladaScene& scene, const std::vector<Material>& materials)
{
	// Generate mapping.
	std::vector<s32> mapping(scene.materials.size(), -1);
	for (size_t i = 0; i < scene.materials.size(); i++) {
		for (size_t j = 0; j < materials.size(); j++) {
			if (materials[j].name == scene.materials[i].name) {
				mapping[i] = j;
			}
		}
	}
	
	// Apply mapping.
	for (Mesh& mesh : scene.meshes) {
		for (SubMesh& submesh : mesh.submeshes) {
			verify(mapping[submesh.material] > -1,
				"Material '%s' has no corresponding asset defined for it.",
				scene.materials.at(submesh.material).name.c_str());
			submesh.material = mapping[submesh.material];
		}
	}
}

std::vector<u8> write_collada(const ColladaScene& scene)
{
	std::vector<u8> vec;
	OutBuffer dest(vec);
	dest.writelf("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>");
	dest.writelf("<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">");
	write_asset_metadata(dest);
	if (scene.texture_paths.size() > 0) {
		write_images(dest, scene.texture_paths);
	}
	write_effects(dest, scene.materials, scene.texture_paths.size());
	write_materials(dest, scene.materials);
	write_geometries(dest, scene.meshes);
	if (scene.joints.size() > 0) {
		write_controllers(dest, scene.meshes, scene.joints);
	}
	write_visual_scenes(dest, scene);
	dest.writelf("\t<scene>");
	dest.writelf("\t\t<instance_visual_scene url=\"#scene\"/>");
	dest.writelf("\t</scene>");
	dest.writelf("</COLLADA>");
	vec.push_back(0);
	return vec;
}

static void write_asset_metadata(OutBuffer dest)
{
	dest.writelf("\t<asset>");
	dest.writelf("\t\t<contributor>");
	dest.writelf("\t\t\t<authoring_tool>Wrench Build Tool</authoring_tool>");
	dest.writelf("\t\t</contributor>");
	dest.writelf("\t\t<created>%04d-%02d-%02dT%02d:%02d:%02d</created>", 1, 1, 1, 0, 0, 0);
	dest.writelf("\t\t<modified>%04d-%02d-%02dT%02d:%02d:%02d</modified>", 1, 1, 1, 0, 0, 0);
	dest.writelf("\t\t<unit name=\"meter\" meter=\"1\"/>");
	dest.writelf("\t\t<up_axis>Z_UP</up_axis>");
	dest.writelf("\t</asset>");
}

static void write_images(OutBuffer dest, const std::vector<std::string>& texture_paths)
{
	dest.writelf("\t<library_images>");
	for (s32 i = 0; i < (s32) texture_paths.size(); i++) {
		dest.writelf("\t\t<image id=\"texture_%d\" name=\"texture_%d\">", i, i);
		dest.writesf("\t\t\t<init_from>");
		const std::string& path = texture_paths[i];
		dest.vec.insert(dest.vec.end(), path.begin(), path.end());
		dest.writelf("</init_from>");
		dest.writelf("\t\t</image>");
	}
	dest.writelf("\t</library_images>");
}

static void write_effects(
	OutBuffer dest, const std::vector<ColladaMaterial>& materials, size_t texture_count)
{
	dest.writelf("\t<library_effects>");
	for (const ColladaMaterial& material : materials) {
		dest.writelf("\t\t<effect id=\"%s_effect\" name=\"%s_effect\">", material.name.c_str(), material.name.c_str());
		dest.writelf("\t\t\t<profile_COMMON>");
		if (material.surface.type == MaterialSurfaceType::TEXTURE) {
			dest.writelf(4, "<newparam sid=\"%s_surface\">", material.name.c_str());
			dest.writelf(4, "\t<surface type=\"2D\">");
			verify_fatal(material.surface.texture < texture_count);
			dest.writelf(4, "\t\t<init_from>texture_%d</init_from>", material.surface.texture);
			dest.writelf(4, "\t\t<format>A8R8G8B8</format>");
			dest.writelf(4, "\t</surface>");
			dest.writelf(4, "</newparam>");
			dest.writelf(4, "<newparam sid=\"%s_sampler\">", material.name.c_str());
			dest.writelf(4, "\t<sampler2D>");
			dest.writelf(4, "\t\t<source>%s_surface</source>", material.name.c_str());
			dest.writelf(4, "\t\t<minfilter>LINEAR_MIPMAP_LINEAR</minfilter>");
			dest.writelf(4, "\t\t<magfilter>LINEAR</magfilter>");
			dest.writelf(4, "\t</sampler2D>");
			dest.writelf(4, "</newparam>");
			dest.writelf(4, "<technique sid=\"common\">");
			dest.writelf(4, "\t<lambert>");
			dest.writelf(4, "\t\t<diffuse>");
			dest.writelf(4, "\t\t\t<texture texture=\"%s_sampler\" texcoord=\"%s_texcoord\"/>", material.name.c_str(), material.name.c_str());
			dest.writelf(4, "\t\t</diffuse>");
			dest.writelf(4, "\t</lambert>");
			dest.writelf(4, "</technique>");
		} else if (material.surface.type == MaterialSurfaceType::COLOUR) {
			dest.writelf(4, "<technique sid=\"common\">");
			dest.writelf(4, "\t<lambert>");
			dest.writelf(4, "\t\t<diffuse>");
			const auto& col = material.surface.colour;
			dest.writelf(4, "\t\t\t<color sid=\"diffuse\">%.9g %.9g %.9g %.9g</color>", col.r, col.g, col.b, col.a);
			dest.writelf(4, "\t\t</diffuse>");
			dest.writelf(4, "\t</lambert>");
			dest.writelf(4, "</technique>");
		}
		dest.writelf("\t\t\t</profile_COMMON>");
		dest.writelf("\t\t</effect>");
	}
	dest.writelf("\t</library_effects>");
}

static void write_materials(OutBuffer dest, const std::vector<ColladaMaterial>& materials)
{
	dest.writelf("\t<library_materials>");
	for (const ColladaMaterial& material : materials) {
		dest.writelf("\t\t<material id=\"%s\" name=\"%s\">", material.name.c_str(), material.name.c_str());
		dest.writelf("\t\t\t<instance_effect url=\"#%s_effect\"/>", material.name.c_str());
		dest.writelf("\t\t</material>");
	}
	dest.writelf("\t</library_materials>");
}

static void write_geometries(OutBuffer dest, const std::vector<Mesh>& meshes)
{
	dest.writelf("\t<library_geometries>");
	for (size_t i = 0; i < meshes.size(); i++) {
		const Mesh& mesh = meshes[i];
		dest.writelf("\t\t<geometry id=\"%s_mesh\" name=\"%s_mesh\">", mesh.name.c_str(), mesh.name.c_str());
		dest.writelf("\t\t\t<mesh>");
		
		dest.writelf(4, "<source id=\"mesh_%d_positions\">", i);
		dest.writesf(4, "\t<float_array id=\"mesh_%d_positions_array\" count=\"%d\">", i, 3 * mesh.vertices.size());
		for (const Vertex& v : mesh.vertices) {
			dest.writesf("%.9g %.9g %.9g ", v.pos.x, v.pos.y, v.pos.z);
		}
		if (mesh.vertices.size() > 0) {
			dest.vec.resize(dest.vec.size() - 1);
		}
		dest.writelf("</float_array>");
		dest.writelf(4, "\t<technique_common>");
		dest.writelf(4, "\t\t<accessor count=\"%d\" offset=\"0\" source=\"#mesh_%d_positions_array\" stride=\"3\">", mesh.vertices.size(), i);
		dest.writelf(4, "\t\t\t<param name=\"X\" type=\"float\"/>");
		dest.writelf(4, "\t\t\t<param name=\"Y\" type=\"float\"/>");
		dest.writelf(4, "\t\t\t<param name=\"Z\" type=\"float\"/>");
		dest.writelf(4, "\t\t</accessor>");
		dest.writelf(4, "\t</technique_common>");
		dest.writelf(4, "</source>");
		if (mesh.flags & MESH_HAS_NORMALS) {
			dest.writelf(4, "<source id=\"mesh_%d_normals\">", i);
			dest.writesf(4, "\t<float_array id=\"mesh_%d_normals_array\" count=\"%d\">", i, 3 * mesh.vertices.size());
			for (const Vertex& v : mesh.vertices) {
				dest.writesf("%.9g %.9g %.9g ", v.normal.x, v.normal.y, v.normal.z);
			}
			if (mesh.vertices.size() > 0) {
				dest.vec.resize(dest.vec.size() - 1);
			}
			dest.writelf("</float_array>");
			dest.writelf(4, "\t<technique_common>");
			dest.writelf(4, "\t\t<accessor count=\"%d\" offset=\"0\" source=\"#mesh_%d_normals_array\" stride=\"3\">", mesh.vertices.size(), i);
			dest.writelf(4, "\t\t\t<param name=\"X\" type=\"float\"/>");
			dest.writelf(4, "\t\t\t<param name=\"Y\" type=\"float\"/>");
			dest.writelf(4, "\t\t\t<param name=\"Z\" type=\"float\"/>");
			dest.writelf(4, "\t\t</accessor>");
			dest.writelf(4, "\t</technique_common>");
			dest.writelf(4, "</source>");
		}
		if (mesh.flags & MESH_HAS_VERTEX_COLOURS) {
			dest.writelf(4, "<source id=\"mesh_%d_colours\">", i);
			dest.writesf(4, "\t<float_array id=\"mesh_%d_colours_array\" count=\"%d\">", i, 4 * mesh.vertices.size());
			for (const Vertex& v : mesh.vertices) {
				f32 r = v.colour.r / 255.f;
				f32 g = v.colour.g / 255.f;
				f32 b = v.colour.b / 255.f;
				f32 a = v.colour.a / 255.f;
				dest.writesf("%.9g %.9g %.9g %.9g ", r, g, b, a);
			}
			if (mesh.vertices.size() > 0) {
				dest.vec.resize(dest.vec.size() - 1);
			}
			dest.writelf("</float_array>");
			dest.writelf(4, "\t<technique_common>");
			dest.writelf(4, "\t\t<accessor count=\"%d\" offset=\"0\" source=\"#mesh_%d_colours_array\" stride=\"4\">", mesh.vertices.size(), i);
			dest.writelf(4, "\t\t\t<param name=\"R\" type=\"float\"/>");
			dest.writelf(4, "\t\t\t<param name=\"G\" type=\"float\"/>");
			dest.writelf(4, "\t\t\t<param name=\"B\" type=\"float\"/>");
			dest.writelf(4, "\t\t\t<param name=\"A\" type=\"float\"/>");
			dest.writelf(4, "\t\t</accessor>");
			dest.writelf(4, "\t</technique_common>");
			dest.writelf(4, "</source>");
		}
		if (mesh.flags & MESH_HAS_TEX_COORDS) {
			dest.writelf(4, "<source id=\"mesh_%d_texcoords\">", i);
			dest.writesf(4, "\t<float_array id=\"mesh_%d_texcoords_array\" count=\"%d\">", i, 2 * mesh.vertices.size());
			for (const Vertex& v : mesh.vertices) {
				dest.writesf("%.9g %.9g ", v.tex_coord.x, 1.f - v.tex_coord.y);
			}
			if (mesh.vertices.size() > 0) {
				dest.vec.resize(dest.vec.size() - 1);
			}
			dest.writelf("</float_array>");
			dest.writelf(4, "\t<technique_common>");
			dest.writelf(4, "\t\t<accessor count=\"%d\" offset=\"0\" source=\"#mesh_%d_texcoords_array\" stride=\"2\">", mesh.vertices.size(), i);
			dest.writelf(4, "\t\t\t<param name=\"S\" type=\"float\"/>");
			dest.writelf(4, "\t\t\t<param name=\"T\" type=\"float\"/>");
			dest.writelf(4, "\t\t</accessor>");
			dest.writelf(4, "\t</technique_common>");
			dest.writelf(4, "</source>");
		}
		dest.writelf(4, "<vertices id=\"mesh_%d_vertices\">", i);
		dest.writelf(4, "\t<input semantic=\"POSITION\" source=\"#mesh_%d_positions\"/>", i);
		dest.writelf(4, "</vertices>");
		if (mesh.flags & MESH_HAS_QUADS) {
			for (s32 j = 0; j < (s32) mesh.submeshes.size(); j++) {
				const SubMesh& submesh = mesh.submeshes[j];
				dest.writelf(4, "<polylist count=\"%d\" material=\"material_symbol_%d\">", submesh.faces.size(), j);
				dest.writelf(4, "\t<input semantic=\"VERTEX\" source=\"#mesh_%d_vertices\" offset=\"0\"/>", i);
				if (mesh.flags & MESH_HAS_NORMALS) {
					dest.writelf(4, "\t<input semantic=\"NORMAL\" source=\"#mesh_%d_normals\" offset=\"0\"/>", i);
				}
				if (mesh.flags & MESH_HAS_TEX_COORDS) {
					dest.writelf(4, "\t<input semantic=\"TEXCOORD\" source=\"#mesh_%d_texcoords\" offset=\"0\" set=\"0\"/>", i);
				}
				dest.writesf(4, "\t<vcount>");
				for (const Face& face : submesh.faces) {
					if (face.v3 > -1) {
						dest.writesf("4 ");
					} else {
						dest.writesf("3 ");
					}
				}
				if (submesh.faces.size() > 0) {
					dest.vec.resize(dest.vec.size() - 1);
				}
				dest.writelf("</vcount>");
				dest.writesf(4, "\t<p>");
				for (const Face& face : submesh.faces) {
					if (face.v3 > -1) {
						dest.writesf("%d %d %d %d ", face.v0, face.v1, face.v2, face.v3);
					} else {
						dest.writesf("%d %d %d ", face.v0, face.v1, face.v2);
					}
				}
				if (submesh.faces.size() > 0) {
					dest.vec.resize(dest.vec.size() - 1);
				}
				dest.writelf("</p>");
				dest.writelf(4, "</polylist>");
			}
		} else {
			for (s32 j = 0; j < (s32) mesh.submeshes.size(); j++) {
				const SubMesh& submesh = mesh.submeshes[j];
				dest.writelf(4, "<triangles count=\"%d\" material=\"material_symbol_%d\">", submesh.faces.size(), j);
				dest.writelf(4, "\t<input semantic=\"VERTEX\" source=\"#mesh_%d_vertices\" offset=\"0\"/>", i);
				if (mesh.flags & MESH_HAS_NORMALS) {
					dest.writelf(4, "\t<input semantic=\"NORMAL\" source=\"#mesh_%d_normals\" offset=\"0\"/>", i);
				}
				if (mesh.flags & MESH_HAS_VERTEX_COLOURS) {
					dest.writelf(4, "\t<input semantic=\"COLOR\" source=\"#mesh_%d_colours\" offset=\"0\"/>", i);
				}
				if (mesh.flags & MESH_HAS_TEX_COORDS) {
					dest.writelf(4, "\t<input semantic=\"TEXCOORD\" source=\"#mesh_%d_texcoords\" offset=\"0\" set=\"0\"/>", i);
				}
				dest.writesf(4, "\t<p>");
				for (const Face& face : submesh.faces) {
					dest.writesf("%d %d %d ", face.v0, face.v1, face.v2);
				}
				if (submesh.faces.size() > 0) {
					dest.vec.resize(dest.vec.size() - 1);
				}
				dest.writelf("</p>");
				dest.writelf(4, "</triangles>");
			}
		}
		dest.writelf("\t\t\t</mesh>");
		dest.writelf("\t\t</geometry>");
	}
	dest.writelf("\t</library_geometries>");
}

static void write_controllers(
	OutBuffer dest, const std::vector<Mesh>& meshes, const std::vector<Joint>& joints)
{
	dest.writelf("\t<library_controllers>");
	for (const Mesh& mesh : meshes) {
		dest.writelf("\t\t<controller id=\"%s_skin\" name=\"%s_skin\">", mesh.name.c_str(), mesh.name.c_str());
		dest.writelf("\t\t\t<skin source=\"#%s_mesh\">", mesh.name.c_str());
		dest.writelf(4, "<source id=\"%s_joints\">", mesh.name.c_str());
		dest.writesf(4, "\t<Name_array count=\"%d\">", (s32) joints.size());
		for (s32 i = 0; i < (s32) joints.size(); i++) {
			dest.writesf("joint_%d ", i);
		}
		if (joints.size() > 0) {
			dest.vec.resize(dest.vec.size() - 1);
		}
		dest.writelf("</Name_array>");
		dest.writelf(4, "</source>");
		dest.writelf(4, "<source id=\"%s_weights\">", mesh.name.c_str());
		s32 weight_count = 0;
		for (const Vertex& vertex : mesh.vertices) {
			weight_count += vertex.skin.count;
		}
		dest.writesf(4, "\t<float_array count=\"%d\">", weight_count);
		for (const Vertex& vertex : mesh.vertices) {
			verify_fatal(vertex.skin.count > 0);
			for (s32 i = 0; i < vertex.skin.count; i++) {
				dest.writesf("%.9g ", vertex.skin.weights[i] / 255.f);
			}
		}
		if (mesh.vertices.size() > 0) {
			dest.vec.resize(dest.vec.size() - 1);
		}
		dest.writelf("</float_array>");
		dest.writelf(4, "</source>");
		dest.writelf(4, "<source id=\"%s_inv_bind_mats\">", mesh.name.c_str());
		dest.writesf(4, "\t<float_array id=\"%s_inv_bind_mats_array\" count=\"%d\">", mesh.name.c_str(), (s32) joints.size());
		for (const Joint& joint : joints) {
			write_matrix4x4(dest, joint.inverse_bind_matrix);
		}
		if (joints.size() > 0) {
			dest.vec.resize(dest.vec.size() - 1);
		}
		dest.writelf("</float_array>");
		dest.writelf(4, "\t<technique_common>");
		dest.writelf(4, "\t\t<accessor source=\"#%s_inv_bind_mats_array\" count=\"%d\" stride=\"16\">", mesh.name.c_str(), joints.size());
		dest.writelf(4, "\t\t\t<param name=\"TRANSFORM\" type=\"float4x4\"/>");
		dest.writelf(4, "\t\t</accessor>");
		dest.writelf(4, "\t</technique_common>");
		dest.writelf(4, "</source>");
		dest.writelf(4, "<joints>");
		dest.writelf(4, "\t<input semantic=\"JOINT\" source=\"#%s_joints\"/>", mesh.name.c_str());
		dest.writelf(4, "\t<input semantic=\"INV_BIND_MATRIX\" source=\"#%s_inv_bind_mats\"/>", mesh.name.c_str());
		dest.writelf(4, "</joints>");
		dest.writelf(4, "<vertex_weights count=\"%d\">", mesh.vertices.size());
		dest.writelf(4, "\t<input semantic=\"JOINT\" source=\"#%s_joints\" offset=\"0\"/>", mesh.name.c_str());
		dest.writelf(4, "\t<input semantic=\"WEIGHT\" source=\"#%s_weights\" offset=\"1\"/>", mesh.name.c_str());
		dest.writesf(4, "\t<vcount>");
		for (const Vertex& vertex : mesh.vertices) {
			dest.writesf("%hhu ", vertex.skin.count);
		}
		if (mesh.vertices.size() > 0) {
			dest.vec.resize(dest.vec.size() - 1);
		}
		dest.writelf("</vcount>");
		dest.writesf(4, "\t<v>");
		s32 weight_index = 0;
		for (const Vertex& vertex : mesh.vertices) {
			verify_fatal(vertex.skin.count > 0);
			for (s32 i = 0; i < vertex.skin.count; i++) {
				dest.writesf("%hhd %d ", vertex.skin.joints[i], weight_index++);
			}
		}
		if (mesh.vertices.size() > 0) {
			dest.vec.resize(dest.vec.size() - 1);
		}
		dest.writelf("</v>");
		dest.writelf(4, "</vertex_weights>");
		dest.writelf("\t\t\t</skin>");
		dest.writelf("\t\t</controller>");
	}
	dest.writelf("\t</library_controllers>");
}

static void write_visual_scenes(OutBuffer dest, const ColladaScene& scene)
{
	dest.writelf("\t<library_visual_scenes>");
	dest.writelf("\t\t<visual_scene id=\"scene\">");
	if (scene.joints.size() > 0) {
		write_joint_node(dest, scene.joints, 0, 3);
	}
	for (const Mesh& mesh : scene.meshes) {
		verify_fatal(mesh.name.size() > 0);
		dest.writelf("\t\t\t<node id=\"%s\" name=\"%s\">", mesh.name.c_str(), mesh.name.c_str());
		if (scene.joints.size() > 0) {
			dest.writelf(4, "<instance_controller url=\"#%s_skin\">", mesh.name.c_str());
			dest.writelf(4, "\t<skeleton>#joint_0</skeleton>");
		} else {
			dest.writelf(4, "<instance_geometry url=\"#%s_mesh\">", mesh.name.c_str());
		}
		dest.writelf(4, "\t<bind_material>");
		dest.writelf(4, "\t\t<technique_common>");
		for (s32 i = 0; i < (s32) mesh.submeshes.size(); i++) {
			s32 material_index = mesh.submeshes[i].material;
			verify_fatal(material_index >= 0 && material_index < scene.materials.size());
			const std::string& material_name = scene.materials[material_index].name;
			dest.writelf(7, "<instance_material symbol=\"material_symbol_%d\" target=\"#%s\">", i, material_name.c_str());
			dest.writelf(7, "\t<bind_vertex_input semantic=\"%s_texcoord\" input_semantic=\"TEXCOORD\" input_set=\"0\"/>", material_name.c_str());
			dest.writelf(7, "</instance_material>");
		}
		dest.writelf(4, "\t\t</technique_common>");
		dest.writelf(4, "\t</bind_material>");
		if (scene.joints.size() > 0) {
			dest.writelf(4, "</instance_controller>");
		} else {
			dest.writelf(4, "</instance_geometry>");
		}
		dest.writelf("\t\t\t</node>");
	}
	dest.writelf("\t\t</visual_scene>");
	dest.writelf("\t</library_visual_scenes>");
}

static void write_joint_node(OutBuffer dest, const std::vector<Joint>& joints, s32 index, s32 indent)
{
	const Joint& joint = joints[index];
	dest.writelf(indent, "<node id=\"joint_%d\" sid=\"joint_%d\" type=\"JOINT\">", index, index);
	dest.writesf(indent, "\t<matrix sid=\"transform\">");
	write_matrix4x4(dest, glm::mat4(1.f));
	dest.vec.resize(dest.vec.size() - 1);
	dest.writelf("</matrix>");
	dest.writelf(indent, "\t<extra>");
	dest.writelf(indent, "\t\t<technique profile=\"blender\">");
	dest.writelf(indent, "\t\t\t<connect>1</connect>");
	dest.writelf(indent, "\t\t\t<layer>0</layer>");
	dest.writelf(indent, "\t\t\t<roll>0</roll>");
	dest.writelf(indent, "\t\t\t<tip_x>%.9g</tip_x>", joint.tip.x);
	dest.writelf(indent, "\t\t\t<tip_y>%.9g</tip_y>", joint.tip.y);
	dest.writelf(indent, "\t\t\t<tip_z>%.9g</tip_z>", joint.tip.z);
	dest.writelf(indent, "\t\t</technique>");
	dest.writelf(indent, "\t</extra>");
	for (s32 child = joint.first_child; child != -1; child = joints[child].right_sibling) {
		write_joint_node(dest, joints, child, indent + 1);
	}
	dest.writelf(indent, "</node>");
}

static void write_matrix4x4(OutBuffer dest, const glm::mat4& matrix)
{
	dest.writesf("%.9g %.9g %.9g %.9g ", matrix[0][0], matrix[1][0], matrix[2][0], matrix[3][0]);
	dest.writesf("%.9g %.9g %.9g %.9g ", matrix[0][1], matrix[1][1], matrix[2][1], matrix[3][1]);
	dest.writesf("%.9g %.9g %.9g %.9g ", matrix[0][2], matrix[1][2], matrix[2][2], matrix[3][2]);
	dest.writesf("%.9g %.9g %.9g %.9g ", matrix[0][3], matrix[1][3], matrix[2][3], matrix[3][3]);
}

s32 add_joint(std::vector<Joint>& joints, Joint joint, s32 parent)
{
	s32 index = (s32) joints.size();
	joint.parent = parent;
	if (parent != -1) {
		s32* next = &joints[parent].first_child;
		while (*next != -1) {
			joint.left_sibling = *next;
			next = &joints[*next].right_sibling;
		}
		*next = index;
	}
	joints.push_back(joint);
	return index;
}

void verify_fatal_collada_scenes_equal(const ColladaScene& lhs, const ColladaScene& rhs)
{
	verify_fatal(lhs.texture_paths.size() == rhs.texture_paths.size());
	verify_fatal(lhs.texture_paths == rhs.texture_paths);
	verify_fatal(lhs.materials.size() == rhs.materials.size());
	for (size_t i = 0; i < lhs.materials.size(); i++) {
		const ColladaMaterial& lmat = lhs.materials[i];
		const ColladaMaterial& rmat = rhs.materials[i];
		verify_fatal(lmat.name == rmat.name);
		verify_fatal(lmat.surface == rmat.surface);
	}
	verify_fatal(lhs.meshes.size() == rhs.meshes.size());
	for (size_t i = 0; i < lhs.meshes.size(); i++) {
		const Mesh& lmesh = lhs.meshes[i];
		const Mesh& rmesh = rhs.meshes[i];
		verify_fatal(lmesh.name == rmesh.name);
		verify_fatal(lmesh.submeshes.size() == rmesh.submeshes.size());
		// If there are no submeshes, we can't recover the flags.
		verify_fatal(lmesh.flags == rmesh.flags || lmesh.submeshes.size() == 0);
		// The COLLADA importer/exporter doesn't preserve the layout of the
		// vertex buffer, so don't check that.
		for (size_t j = 0; j < lmesh.submeshes.size(); j++) {
			const SubMesh& lsub = lmesh.submeshes[j];
			const SubMesh& rsub = rmesh.submeshes[j];
			for (size_t k = 0; k < lsub.faces.size(); k++) {
				const Face& lface = lsub.faces[k];
				const Face& rface = rsub.faces[k];
				Vertex lverts[4] = {
					lmesh.vertices.at(lface.v0),
					lmesh.vertices.at(lface.v1),
					lmesh.vertices.at(lface.v2),
					Vertex(glm::vec3(0, 0, 0))
				};
				Vertex rverts[4] = {
					rmesh.vertices.at(rface.v0),
					rmesh.vertices.at(rface.v1),
					rmesh.vertices.at(rface.v2),
					Vertex(glm::vec3(0, 0, 0))
				};
				verify_fatal((lface.v3 > -1) == (rface.v3 > -1));
				if (lface.v3 > -1) {
					lverts[3] = lmesh.vertices.at(lface.v3);
					rverts[3] = rmesh.vertices.at(rface.v3);
				}
				for (s32 k = 0; k < 4; k++) {
					verify_fatal(lverts[k].pos == rverts[k].pos);
					verify_fatal(lverts[k].normal == rverts[k].normal);
					// We don't currently preserve joint indices, so we don't
					// check them here.
					for (s32 l = 0; l < 3; l++) {
						lverts[k].skin.joints[l] = 0;
						rverts[k].skin.joints[l] = 0;
					}
					verify_fatal(lverts[k].skin == rverts[k].skin);
					verify_fatal(lverts[k].tex_coord == rverts[k].tex_coord);
				}
			}
			verify_fatal(lsub.material == rsub.material);
		}
	}
}
