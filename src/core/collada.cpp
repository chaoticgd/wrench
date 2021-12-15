/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2021 chaoticgd

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

#include "rapidxml/rapidxml.hpp"
using XmlDocument = rapidxml::xml_document<>;
using XmlNode = rapidxml::xml_node<>;
using XmlAttrib = rapidxml::xml_attribute<>;
#define xml_for_each_child_of_type(child, parent, name) \
	for(const XmlNode* child = parent->first_node(name); child != nullptr; child = child->next_sibling(name))

using IdMap = std::map<std::string, const XmlNode*>;
using NodeToIndexMap = std::map<const XmlNode*, s32>;
static Material read_material(const XmlNode* material_node, const IdMap& ids, const NodeToIndexMap& images);
struct VertexData {
	Opt<std::vector<f32>> positions;
	Opt<std::vector<f32>> normals;
	Opt<std::vector<f32>> tex_coords;
};
static VertexData read_vertices(const XmlNode* geometry, const IdMap& ids);
static std::vector<f32> read_vertex_source(const XmlNode* source, const IdMap& ids);
static void read_submeshes(Mesh& mesh, const XmlNode* instance_geometry, const XmlNode* geometry, const IdMap& ids, const NodeToIndexMap& materials, const VertexData& vertex_data);
struct CreateVertexInput {
	const std::vector<f32>* positions = nullptr;
	const std::vector<f32>* normals = nullptr;
	const std::vector<f32>* tex_coords = nullptr;
	s32 position_offset = -1;
	s32 normal_offset = -1;
	s32 tex_coord_offset = -1;
};
static Vertex create_vertex(const std::vector<s32>& indices, s32 base, const CreateVertexInput& input);
static s32 read_s32(const char*& input, const char* context);
static void enumerate_ids(std::map<std::string, const XmlNode*>& ids, const XmlNode* node);
static const XmlNode* xml_child(const XmlNode* node, const char* name);
static const XmlAttrib* xml_attrib(const XmlNode* node, const char* name);
static const XmlNode* node_from_id(const IdMap& map, const char* id);

static void write_asset_metadata(OutBuffer dest);
static void write_images(OutBuffer dest, const std::vector<std::string>& texture_paths);
static void write_effects(OutBuffer dest, const std::vector<Material>& materials, size_t texture_count);
static void write_materials(OutBuffer dest, const std::vector<Material>& materials);
static void write_geometries(OutBuffer dest, const std::vector<Mesh>& meshes);
static void write_visual_scenes(OutBuffer dest, const ColladaScene& scene);
static void write_joint_node(OutBuffer dest, const std::vector<Joint>& joints, s32 index, s32 indent);

ColladaScene read_collada(std::vector<u8> src) {
	src.push_back('\0');
	
	XmlDocument doc;
	try {
		doc.parse<0>((char*) src.data());
	} catch(rapidxml::parse_error& err) {
		throw ParseError("%s", err.what());
	}
	
	const XmlNode* root = xml_child(&doc, "COLLADA");
	
	std::map<std::string, const XmlNode*> ids;
	enumerate_ids(ids, root);
	
	ColladaScene scene;
	
	NodeToIndexMap images;
	const XmlNode* library_images = root->first_node("library_images");
	if(library_images) {
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
	xml_for_each_child_of_type(node, visual_scene, "node") {
		const XmlNode* instance_geometry = node->first_node("instance_geometry");
		if(!instance_geometry) {
			continue;
		}
		const XmlNode* geometry = node_from_id(ids, xml_attrib(instance_geometry, "url")->value());
		
		Mesh mesh;
		mesh.name = xml_attrib(node, "id")->value();
		auto vertex_data = read_vertices(geometry, ids);
		if(vertex_data.positions.has_value()) {
			read_submeshes(mesh, instance_geometry, geometry, ids, materials, vertex_data);
		}
		scene.meshes.emplace_back(deduplicate_vertices(std::move(mesh)));
	}
	return scene;
}

static Material read_material(const XmlNode* material_node, const IdMap& ids, const NodeToIndexMap& images) {
	// Follow the white rabbit (it's white because its texture couldn't be loaded).
	const XmlNode* instance_effect = xml_child(material_node, "instance_effect");
	const XmlNode* effect = node_from_id(ids, xml_attrib(instance_effect, "url")->value());
	if(!strcmp(effect->name(), "effect") == 0) {
		throw ParseError("Effect referenced by id is not an <effect> node.");
	}
	const XmlNode* profile = effect->first_node();
	if(!profile) {
		throw ParseError("<%s> node has no children.", effect->name());
	}
	const XmlNode* technique = xml_child(profile, "technique");
	const XmlNode* shader = technique->first_node();
	if(!shader) {
		throw ParseError("<%s> node has no children.", technique->name());
	}
	const XmlNode* diffuse = xml_child(shader, "diffuse");
	if(const XmlNode* texture = diffuse->first_node("texture")) {
		const char* sampler_sid = xml_attrib(texture, "texture")->value();
		const XmlNode* sampler = nullptr;
		xml_for_each_child_of_type(newparam, profile, "newparam") {
			if(strcmp(xml_attrib(newparam, "sid")->value(), sampler_sid) == 0) {
				sampler = xml_child(newparam, "sampler2D");
				break;
			}
		}
		if(!sampler) {
			throw ParseError("Unable to find sampler '%s'.", sampler_sid);
		}
		const char* surface_sid = xml_child(sampler, "source")->value();
		const XmlNode* surface = nullptr;
		xml_for_each_child_of_type(newparam, profile, "newparam") {
			if(strcmp(xml_attrib(newparam, "sid")->value(), surface_sid) == 0) {
				surface = xml_child(newparam, "surface");
				break;
			}
		}
		if(!surface) {
			throw ParseError("Unable to find surface '%s'.", surface_sid);
		}
		auto image_id = std::string("#") + xml_child(surface, "init_from")->value();
		const XmlNode* image = node_from_id(ids, image_id.c_str());
		auto texture_index = images.find(image);
		if(texture_index == images.end()) {
			throw ParseError("An <image> node that was referenced cannot be found.");
		}
		Material material;
		material.name = xml_attrib(material_node, "id")->value();
		material.texture = texture_index->second;
		return material;
	} else if(const XmlNode* colour = diffuse->first_node("color")) {
		glm::vec4 value;
		const char* r_ptr = colour->value();
		char* g_ptr;
		value.r = strtof(r_ptr, &g_ptr);
		if(g_ptr == r_ptr) {
			throw ParseError("<color> node has invalid body.");
		}
		char* b_ptr;
		value.g = strtof(g_ptr, &b_ptr);
		if(b_ptr == g_ptr) {
			throw ParseError("<color> node has invalid body.");
		}
		char* a_ptr;
		value.b = strtof(b_ptr, &a_ptr);
		if(a_ptr == b_ptr) {
			throw ParseError("<color> node has invalid body.");
		}
		char* end_ptr;
		value.a = strtof(a_ptr, &end_ptr);
		if(end_ptr == a_ptr) {
			throw ParseError("<color> node has invalid body.");
		}
		Material material;
		material.name = xml_attrib(material_node, "id")->value();
		material.colour = value;
		return material;
	}
	throw ParseError("<diffuse> node needs either a <texture> or <color> node as a child.");
}

static VertexData read_vertices(const XmlNode* geometry, const IdMap& ids) {
	const XmlNode* mesh_node = xml_child(geometry, "mesh");
	const XmlNode* triangles = mesh_node->first_node("triangles");
	const XmlNode* polylist = mesh_node->first_node("polylist");
	const XmlNode* indices = triangles != nullptr ? triangles : polylist;
	const char* mesh_name = xml_attrib(geometry, "id")->value();
	if(indices == nullptr) {
		return {};
	}
	
	const XmlNode* vertices = nullptr;
	const XmlNode* normals_source = nullptr;
	const XmlNode* tex_coords_source = nullptr;
	xml_for_each_child_of_type(input, indices, "input") {
		const XmlAttrib* semantic = xml_attrib(input, "semantic");
		if(strcmp(semantic->value(), "VERTEX") == 0) {
			vertices = node_from_id(ids, xml_attrib(input, "source")->value());
		}
		if(strcmp(semantic->value(), "NORMAL") == 0) {
			normals_source = node_from_id(ids, xml_attrib(input, "source")->value());
		}
		if(strcmp(semantic->value(), "TEXCOORD") == 0) {
			tex_coords_source = node_from_id(ids, xml_attrib(input, "source")->value());
		}
	}
	if(!vertices) {
		throw ParseError("<triangles> node missing VERTEX input.");
	}
	
	const XmlNode* positions_source = nullptr;
	xml_for_each_child_of_type(input, vertices, "input") {
		const XmlAttrib* semantic = xml_attrib(input, "semantic");
		if(strcmp(semantic->value(), "POSITION") == 0) {
			positions_source = node_from_id(ids, xml_attrib(input, "source")->value());
		}
	}
	if(!positions_source) {
		throw ParseError("<vertices> node missing POSITIONS input.");
	}
	auto positions = read_vertex_source(positions_source, ids);
	if(positions.size() % 3 != 0) {
		throw ParseError("Vertex positions array for mesh '%s' has a bad size (not divisible by 3).", mesh_name);
	}
	
	Opt<std::vector<f32>> normals;
	if(normals_source) {
		normals = read_vertex_source(normals_source, ids);
		if(normals->size() % 3 != 0) {
			throw ParseError("Normals array for mesh '%s' has a bad size (not divisible by 3).", mesh_name);
		}
	}
	
	Opt<std::vector<f32>> tex_coords;
	if(tex_coords_source) {
		tex_coords = read_vertex_source(tex_coords_source, ids);
		if(tex_coords->size() % 2 != 0) {
			throw ParseError("Texture coordinates array for mesh '%s' has a bad size (not divisible by 2).", mesh_name);
		}
	}
	
	return {positions, normals, tex_coords};
}

static std::vector<f32> read_vertex_source(const XmlNode* source, const IdMap& ids) {
	const XmlNode* technique_common = xml_child(source, "technique_common");
	const XmlNode* accessor = xml_child(technique_common, "accessor");
	const XmlNode* array = node_from_id(ids, xml_attrib(accessor, "source")->value());
	if(strcmp(array->name(), "float_array") != 0) {
		throw ParseError("Only <float_array> nodes are supported for storing vertex attributes.");
	}
	std::vector<f32> data;
	data.resize(atoi(xml_attrib(array, "count")->value()));
	const char* ptr = array->value();
	for(f32& value : data) {
		char* next;
		value = strtof(ptr, &next);
		if(next == ptr) {
			throw ParseError("Failed to read <float_array>.");
		}
		ptr = next;
	}
	return data;
}

static void read_submeshes(Mesh& mesh, const XmlNode* instance_geometry, const XmlNode* geometry, const IdMap& ids, const NodeToIndexMap& materials, const VertexData& vertex_data) {
	const XmlNode* bind_material = xml_child(instance_geometry, "bind_material");
	const XmlNode* technique_common = xml_child(bind_material, "technique_common");
	const XmlNode* mesh_node = xml_child(geometry, "mesh");
	for(const XmlNode* indices = mesh_node->first_node(); indices != nullptr; indices = indices->next_sibling()) {
		bool is_triangles = strcmp(indices->name(), "triangles") == 0;
		bool is_polylist = strcmp(indices->name(), "polylist") == 0;
		
		if(is_triangles || is_polylist) {
			s32 face_count = atoi(xml_attrib(indices, "count")->value());
			const char* material_symbol = xml_attrib(indices, "material")->value();
			
			// Find the material.
			s32 material = -1;
			xml_for_each_child_of_type(instance_material, technique_common, "instance_material") {
				if(strcmp(xml_attrib(instance_material, "symbol")->value(), material_symbol) == 0) {
					material = materials.at(node_from_id(ids, xml_attrib(instance_material, "target")->value()));
				}
			}
			if(material == -1) {
				throw ParseError("Missing <instance_material> node.");
			}
			
			// Find the offsets of each <input> and the overall stride.
			s32 position_offset = -1;
			s32 normal_offset = -1;
			s32 tex_coord_offset = -1;
			xml_for_each_child_of_type(input, indices, "input") {
				if(strcmp(xml_attrib(input, "semantic")->value(), "VERTEX") == 0) {
					position_offset = atoi(xml_attrib(input, "offset")->value());
				}
				if(strcmp(xml_attrib(input, "semantic")->value(), "NORMAL") == 0) {
					normal_offset = atoi(xml_attrib(input, "offset")->value());
				}
				if(strcmp(xml_attrib(input, "semantic")->value(), "TEXCOORD") == 0) {
					tex_coord_offset = atoi(xml_attrib(input, "offset")->value());
				}
			}
			s32 vertex_stride = std::max({position_offset, normal_offset, tex_coord_offset}) + 1;
			verify(position_offset > -1 && vertex_stride < 10, "Invalid or missing <input> node.");
			
			if(normal_offset > -1) {
				mesh.flags |= MESH_HAS_NORMALS;
			}
			if(tex_coord_offset > -1) {
				mesh.flags |= MESH_HAS_TEX_COORDS;
			}
			
			CreateVertexInput args {
				vertex_data.positions.has_value() ? &(*vertex_data.positions) : nullptr,
				vertex_data.normals.has_value() ? &(*vertex_data.normals) : nullptr,
				vertex_data.tex_coords.has_value() ? &(*vertex_data.tex_coords) : nullptr,
				position_offset, normal_offset, tex_coord_offset
			};
			
			// Because of the permissive nature of the COLLADA format, here we
			// just add a new vertex for every index. We can deduplicate them
			// later if necessary.
			SubMesh submesh;
			submesh.material = material;
			if(is_triangles) {
				const char* p = xml_child(indices, "p")->value();
				std::vector<s32> index_data;
				for(s32 i = 0; i < face_count * 3 * vertex_stride; i++) {
					index_data.push_back(read_s32(p, "<p> node"));
				}
				
				s32 i = 0;
				for(s32 face = 0; face < face_count; face++) {
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
				for(s32 i = 0; i < face_count; i++) {
					s32 vc = read_s32(vcount, "<vcount> node");
					verify(vc == 3 || vc == 4, "Only tris and quads are supported.");
					vcount_data.push_back(vc);
				}
				
				const char* p = xml_child(indices, "p")->value();
				std::vector<s32> index_data;
				for(s32 i = 0; i < face_count; i++) {
					for(s32 j = 0; j < vcount_data[i] * vertex_stride; j++) {
						index_data.push_back(read_s32(p, "<p> node"));
					}
				}
				
				mesh.flags |= MESH_HAS_QUADS;
				s32 i = 0;
				for(s32 face = 0; face < face_count; face++) {
					s32 index = (s32) mesh.vertices.size();
					if(vcount_data[face] == 3) {
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

static Vertex create_vertex(const std::vector<s32>& indices, s32 base, const CreateVertexInput& input) {
	Vertex vertex(glm::vec3(0.f, 0.f, 0.f));
	if(input.position_offset > -1) {
		s32 position_index = indices.at(base + input.position_offset);
		vertex.pos.x = input.positions->at(position_index * 3 + 0);
		vertex.pos.y = input.positions->at(position_index * 3 + 1);
		vertex.pos.z = input.positions->at(position_index * 3 + 2);
	}
	if(input.normal_offset > -1) {
		s32 normal_index = indices.at(base + input.normal_offset);
		vertex.normal.x = input.normals->at(normal_index * 3 + 0);
		vertex.normal.y = input.normals->at(normal_index * 3 + 1);
		vertex.normal.z = input.normals->at(normal_index * 3 + 2);
	}
	if(input.tex_coord_offset > -1) {
		s32 tex_coord_index = indices.at(base + input.tex_coord_offset);
		vertex.tex_coord.x = input.tex_coords->at(tex_coord_index * 2 + 0);
		vertex.tex_coord.y = input.tex_coords->at(tex_coord_index * 2 + 1);
	}
	return vertex;
}

static s32 read_s32(const char*& input, const char* context) {
	char* next;
	s32 value = strtol(input, &next, 10);
	if(next == input) {
		throw ParseError("Failed to read integers from %s.", context);
	}
	input = next;
	return value;
}

static void enumerate_ids(std::map<std::string, const XmlNode*>& ids, const XmlNode* node) {
	for(XmlNode* child = node->first_node(); child != nullptr; child = child->next_sibling()) {
		XmlAttrib* id = child->first_attribute("id");
		if(id != nullptr) {
			ids[std::string("#") + id->value()] = child;
		}
		enumerate_ids(ids, child);
	}
}

static const XmlNode* xml_child(const XmlNode* node, const char* name) {
	XmlNode* child = node->first_node(name);
	if(!child) {
		throw ParseError("<%s> node missing <%s> child.", node->name(), name);
	}
	return child;
}

static const XmlAttrib* xml_attrib(const XmlNode* node, const char* name) {
	XmlAttrib* attrib = node->first_attribute(name);
	if(!attrib) {
		throw ParseError("<%s> node missing %s attribute.", node->name(), name);
	}
	return attrib;
}

static const XmlNode* node_from_id(const IdMap& map, const char* id) {
	if(*id != '#') {
		throw ParseError("Only ids starting with # are supported.");
	}
	auto iter = map.find(id);
	if(iter == map.end()) {
		throw ParseError("No element with id equal to '%s'.", id);
	}
	return iter->second;
}

std::vector<u8> write_collada(const ColladaScene& scene) {
	std::vector<u8> vec;
	OutBuffer dest(vec);
	dest.writelf("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>");
	dest.writelf("<COLLADA xmlns=\"http://www.collada.org/2005/11/COLLADASchema\" version=\"1.4.1\">");
	write_asset_metadata(dest);
	if(scene.texture_paths.size() > 0) {
		write_images(dest, scene.texture_paths);
	}
	write_effects(dest, scene.materials, scene.texture_paths.size());
	write_materials(dest, scene.materials);
	write_geometries(dest, scene.meshes);
	write_visual_scenes(dest, scene);
	dest.writelf("\t<scene>");
	dest.writelf("\t\t<instance_visual_scene url=\"#scene\"/>");
	dest.writelf("\t</scene>");
	dest.writelf("</COLLADA>");
	return vec;
}

static void write_asset_metadata(OutBuffer dest) {
	dest.writelf("\t<asset>");
	dest.writelf("\t\t<contributor>");
	dest.writelf("\t\t\t<authoring_tool>Wrench WAD Utility</authoring_tool>");
	dest.writelf("\t\t</contributor>");
	dest.writelf("\t\t<created>%04d-%02d-%02dT%02d:%02d:%02d</created>", 1, 1, 1, 0, 0, 0);
	dest.writelf("\t\t<modified>%04d-%02d-%02dT%02d:%02d:%02d</modified>", 1, 1, 1, 0, 0, 0);
	dest.writelf("\t\t<unit name=\"meter\" meter=\"1\"/>");
	dest.writelf("\t\t<up_axis>Z_UP</up_axis>");
	dest.writelf("\t</asset>");
}

static void write_images(OutBuffer dest, const std::vector<std::string>& texture_paths) {
	dest.writelf("\t<library_images>");
	for(s32 i = 0; i < (s32) texture_paths.size(); i++) {
		dest.writelf("\t\t<image id=\"texture_%d\">", i);
		dest.writesf("\t\t\t<init_from>");
		const std::string& path = texture_paths[i];
		dest.vec.insert(dest.vec.end(), path.begin(), path.end());
		dest.writelf("</init_from>");
		dest.writelf("\t\t</image>");
	}
	dest.writelf("\t</library_images>");
}

static void write_effects(OutBuffer dest, const std::vector<Material>& materials, size_t texture_count) {
	dest.writelf("\t<library_effects>");
	for(const Material& material : materials) {
		dest.writelf("\t\t<effect id=\"%s_effect\">", material.name.c_str());
		dest.writelf("\t\t\t<profile_COMMON>");
		if(material.texture.has_value()) {
			dest.writelf(4, "<newparam sid=\"%s_surface\">", material.name.c_str());
			dest.writelf(4, "\t<surface type=\"2D\">");
			assert(material.texture < texture_count);
			dest.writelf(4, "\t\t<init_from>texture_%d</init_from>", material.texture);
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
		} else {
			assert(material.colour.has_value());
			dest.writelf(4, "<technique sid=\"common\">");
			dest.writelf(4, "\t<lambert>");
			dest.writelf(4, "\t\t<diffuse>");
			auto col = *material.colour;
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

static void write_materials(OutBuffer dest, const std::vector<Material>& materials) {
	dest.writelf("\t<library_materials>");
	for(const Material& material : materials) {
		dest.writelf("\t\t<material id=\"%s\">", material.name.c_str());
		dest.writelf("\t\t\t<instance_effect url=\"#%s_effect\"/>", material.name.c_str());
		dest.writelf("\t\t</material>");
	}
	dest.writelf("\t</library_materials>");
}

static void write_geometries(OutBuffer dest, const std::vector<Mesh>& meshes) {
	dest.writelf("\t<library_geometries>");
	for(size_t i = 0; i < meshes.size(); i++) {
		const Mesh& mesh = meshes[i];
		dest.writelf("\t\t<geometry id=\"%s_mesh\">", mesh.name.c_str());
		dest.writelf("\t\t\t<mesh>");
		
		dest.writelf(4, "<source id=\"mesh_%d_positions\">", i);
		dest.writesf(4, "\t<float_array id=\"mesh_%d_positions_array\" count=\"%d\">", i, 3 * mesh.vertices.size());
		for(const Vertex& v : mesh.vertices) {
			dest.writesf("%.9g %.9g %.9g ", v.pos.x, v.pos.y, v.pos.z);
		}
		if(mesh.vertices.size() > 0) {
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
		if(mesh.flags & MESH_HAS_NORMALS) {
			dest.writelf(4, "<source id=\"mesh_%d_normals\">", i);
			dest.writesf(4, "\t<float_array id=\"mesh_%d_normals_array\" count=\"%d\">", i, 3 * mesh.vertices.size());
			for(const Vertex& v : mesh.vertices) {
				dest.writesf("%.9g %.9g %.9g ", v.normal.x, v.normal.y, v.normal.z);
			}
			if(mesh.vertices.size() > 0) {
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
		if(mesh.flags & MESH_HAS_TEX_COORDS) {
			dest.writelf(4, "<source id=\"mesh_%d_texcoords\">", i);
			dest.writesf(4, "\t<float_array id=\"mesh_%d_texcoords_array\" count=\"%d\">", i, 2 * mesh.vertices.size());
			for(const Vertex& v : mesh.vertices) {
				dest.writesf("%.9g %.9g ", v.tex_coord.x, v.tex_coord.y);
			}
			if(mesh.vertices.size() > 0) {
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
		if(mesh.flags & MESH_HAS_QUADS) {
			for(s32 j = 0; j < (s32) mesh.submeshes.size(); j++) {
				const SubMesh& submesh = mesh.submeshes[j];
				dest.writelf(4, "<polylist count=\"%d\" material=\"material_symbol_%d\">", submesh.faces.size(), j);
				dest.writelf(4, "\t<input semantic=\"VERTEX\" source=\"#mesh_%d_vertices\" offset=\"0\"/>", i);
				if(mesh.flags & MESH_HAS_NORMALS) {
					dest.writelf(4, "\t<input semantic=\"NORMAL\" source=\"#mesh_%d_normals\" offset=\"0\"/>", i);
				}
				if(mesh.flags & MESH_HAS_TEX_COORDS) {
					dest.writelf(4, "\t<input semantic=\"TEXCOORD\" source=\"#mesh_%d_texcoords\" offset=\"0\" set=\"0\"/>", i);
				}
				dest.writesf(4, "\t<vcount>");
				for(const Face& face : submesh.faces) {
					if(face.v3 > -1) {
						dest.writesf("4 ");
					} else {
						dest.writesf("3 ");
					}
				}
				if(submesh.faces.size() > 0) {
					dest.vec.resize(dest.vec.size() - 1);
				}
				dest.writelf("</vcount>");
				dest.writesf(4, "\t<p>");
				for(const Face& face : submesh.faces) {
					if(face.v3 > -1) {
						dest.writesf("%d %d %d %d ", face.v0, face.v1, face.v2, face.v3);
					} else {
						dest.writesf("%d %d %d ", face.v0, face.v1, face.v2);
					}
				}
				if(submesh.faces.size() > 0) {
					dest.vec.resize(dest.vec.size() - 1);
				}
				dest.writelf("</p>");
				dest.writelf(4, "</polylist>");
			}
		} else {
			for(s32 j = 0; j < (s32) mesh.submeshes.size(); j++) {
				const SubMesh& submesh = mesh.submeshes[j];
				dest.writelf(4, "<triangles count=\"%d\" material=\"material_symbol_%d\">", submesh.faces.size(), j);
				dest.writelf(4, "\t<input semantic=\"VERTEX\" source=\"#mesh_%d_vertices\" offset=\"0\"/>", i);
				if(mesh.flags & MESH_HAS_NORMALS) {
					dest.writelf(4, "\t<input semantic=\"NORMAL\" source=\"#mesh_%d_normals\" offset=\"0\"/>", i);
				}
				if(mesh.flags & MESH_HAS_TEX_COORDS) {
					dest.writelf(4, "\t<input semantic=\"TEXCOORD\" source=\"#mesh_%d_texcoords\" offset=\"0\" set=\"0\"/>", i);
				}
				dest.writesf(4, "\t<p>");
				for(const Face& face : submesh.faces) {
					dest.writesf("%d %d %d ", face.v0, face.v1, face.v2);
				}
				if(submesh.faces.size() > 0) {
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

static void write_visual_scenes(OutBuffer dest, const ColladaScene& scene) {
	dest.writelf("\t<library_visual_scenes>");
	dest.writelf("\t\t<visual_scene id=\"scene\">");
	if(scene.joints.size() > 0) {
		write_joint_node(dest, scene.joints, 0, 3);
	}
	for(const Mesh& mesh : scene.meshes) {
		assert(mesh.name.size() > 0);
		dest.writelf("\t\t\t<node id=\"%s\">", mesh.name.c_str());
		dest.writelf(4, "<instance_geometry url=\"#%s_mesh\">", mesh.name.c_str());
		dest.writelf(4, "\t<bind_material>");
		dest.writelf(4, "\t\t<technique_common>");
		for(s32 i = 0; i < (s32) mesh.submeshes.size(); i++) {
			assert(mesh.submeshes[i].material >= 0);
			const std::string& material_name = scene.materials.at(mesh.submeshes[i].material).name;
			dest.writelf(7, "<instance_material symbol=\"material_symbol_%d\" target=\"#%s\">", i, material_name.c_str());
			dest.writelf(7, "\t<bind_vertex_input semantic=\"%s_texcoord\" input_semantic=\"TEXCOORD\" input_set=\"0\"/>", material_name.c_str());
			dest.writelf(7, "</instance_material>");
		}
		dest.writelf(4, "\t\t</technique_common>");
		dest.writelf(4, "\t</bind_material>");
		dest.writelf(4, "</instance_geometry>");
		dest.writelf("\t\t\t</node>");
	}
	dest.writelf("\t\t</visual_scene>");
	dest.writelf("\t</library_visual_scenes>");
}

static void write_joint_node(OutBuffer dest, const std::vector<Joint>& joints, s32 index, s32 indent) {
	const Joint& joint = joints[index];
	dest.writelf(indent, "<node id=\"joint_%d\" type=\"JOINT\">", index);
	dest.writesf(indent, "\t<matrix sid=\"transform\">");
	dest.writesf("%.9g %.9g %.9g %.9g ", joint.matrix[0][0], joint.matrix[1][0], joint.matrix[2][0], joint.matrix[3][0]);
	dest.writesf("%.9g %.9g %.9g %.9g ", joint.matrix[0][1], joint.matrix[1][1], joint.matrix[2][1], joint.matrix[3][1]);
	dest.writesf("%.9g %.9g %.9g %.9g ", joint.matrix[0][2], joint.matrix[1][2], joint.matrix[2][2], joint.matrix[3][2]);
	dest.writesf("%.9g %.9g %.9g %.9g", joint.matrix[0][3], joint.matrix[1][3], joint.matrix[2][3], joint.matrix[3][3]);
	dest.writelf("</matrix>");
	for(s32 child = joint.first_child; child != -1; child = joints[child].right_sibling) {
		write_joint_node(dest, joints, child, indent + 1);
	}
	dest.writelf(indent, "</node>");
}

s32 add_joint(std::vector<Joint>& joints, Joint joint, s32 parent) {
	s32 index = (s32) joints.size();
	joint.parent = parent;
	if(parent != -1) {
		s32* next = &joints[parent].first_child;
		while(*next != -1) {
			joint.left_sibling = *next;
			next = &joints[*next].right_sibling;
		}
		*next = index;
	}
	joints.push_back(joint);
	return index;
}
