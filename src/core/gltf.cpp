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

#include "gltf.h"

#include <limits>
#include <nlohmann/json.hpp>
using Json = nlohmann::ordered_json;
#include <core/algorithm.h>

namespace GLTF {

packed_struct(GLBHeader,
	u32 magic;
	u32 version;
	u32 length;
)

packed_struct(GLBChunk,
	u32 length;
	u32 type;
)

struct AnimationChannelTarget {
	Opt<s32> node;
	std::string path;
};

struct AnimationChannel {
	s32 sampler;
	AnimationChannelTarget target;
};

struct AnimationSampler {
	s32 input;
	Opt<std::string> interpolation;
	s32 output;
};

enum AccessorComponentType {
	SIGNED_BYTE = 5120,
	UNSIGNED_BYTE = 5121,
	SIGNED_SHORT = 5122,
	UNSIGNED_SHORT = 5123,
	UNSIGNED_INT = 5125,
	FLOAT = 5126
};

enum AccessorType {
	SCALAR,
	VEC2,
	VEC3,
	VEC4,
	MAT2,
	MAT3,
	MAT4
};

enum BufferViewTarget {
	ARRAY_BUFFER = 34962,
	ELEMENT_ARRAY_BUFFER = 34963
};

struct Accessor {
	std::vector<u8> bytes;
	AccessorComponentType component_type;
	Opt<bool> normalized;
	s32 count;
	AccessorType type;
	std::vector<f32> max;
	std::vector<f32> min;
	// unimplemented: sparse
	Opt<std::string> name;
	Opt<BufferViewTarget> target;
};

struct GLTFBufferView {
	s32 buffer;
	Opt<s32> byte_offset;
	s32 byte_length;
	Opt<s32> byte_stride;
	Opt<s32> target;
	Opt<std::string> name;
};

struct GLTFBuffer {
	Opt<std::string> uri;
	s32 byte_length;
	Opt<std::string> name;
};

// GLTF, Scenes & Nodes
static ModelFile read_gltf(const Json& src, Buffer bin_chunk);
static Json write_gltf(const ModelFile& src, OutBuffer bin_chunk);
static Asset read_asset(const Json& src);
static Json write_asset(const Asset& src);
static Scene read_scene(const Json& src);
static Json write_scene(const Scene& src);
static Node read_node(const Json& src);
static Json write_node(const Node& src);

// Meshes
static Mesh read_mesh(const Json& src, const std::vector<Accessor>& accessors);
static Json write_mesh(const Mesh& src, std::vector<Accessor>& accessors);
static MeshPrimitive read_mesh_primitive(const Json& src, std::vector<Vertex>& vertices_dest, const std::vector<Accessor>& accessors);
static Json write_mesh_primitive(const MeshPrimitive& src, const std::vector<Vertex>& vertices_src, std::vector<Accessor>& accessors);
static bool read_attribute(Vertex* dest, MeshPrimitiveAttribute semantic, const Accessor& accessor);
static Json write_attributes(const MeshPrimitive& src, const std::vector<Vertex>& vertices, std::vector<Accessor>& accessors);
static std::vector<glm::vec2> convert_tex_coords(const Accessor& accessor);
static std::vector<ColourAttribute> convert_colours(const Accessor& accessor);
static std::vector<std::array<s8, 3>> convert_joints(const Accessor& accessor);
static std::vector<std::array<u8, 3>> convert_weights(const Accessor& accessor);
static std::vector<u32> read_indices(const Json& src, const std::vector<Accessor>& accessors);
static void write_indices(Json& dest, const std::vector<u32>& indices, std::vector<Accessor>& accessors);

// Materials & Textures
static Material read_material(const Json& src);
static Json write_material(const Material& src);
static MaterialPbrMetallicRoughness read_material_pbr_metallic_roughness(const Json& src);
static Json write_material_pbr_metallic_roughness(const MaterialPbrMetallicRoughness& src);
static TextureInfo read_texture_info(const Json& src);
static Json write_texture_info(const TextureInfo& src);
static Texture read_texture(const Json& src);
static Json write_texture(const Texture& src);
static Image read_image(const Json& src);
static Json write_image(const Image& src);
static Sampler read_sampler(const Json& src);
static Json write_sampler(const Sampler& src);

// Animation
static Skin read_skin(const Json& src, const std::vector<Accessor>& accessors);
static Json write_skin(const Skin& src, std::vector<Accessor>& accessors);
static Animation read_animation(const Json& src, const std::vector<Accessor>& accessors);
static Json write_animation(const Animation& src, std::vector<Accessor>& accessors);
static void convert_animation_sampler_output(std::vector<AnimationAttributes>& dest, const char* path, const Accessor& accessor);
template <typename AttributeType>
static void build_animation_channel(
		std::vector<AnimationChannel>& channels,
		std::vector<AnimationSampler>& samplers,
		std::vector<Accessor>& accessors,
		const AnimationChannelGroup& group,
		AttributeType AnimationAttributes::*attribute,
		const char* path,
		AccessorType type,
		s32 input_accessor_index);
static AnimationChannel read_animation_channel(const Json& src);
static Json write_animation_channel(const AnimationChannel& src);
static AnimationChannelTarget read_animation_channel_target(const Json& src);
static Json write_animation_channel_target(const AnimationChannelTarget& src);
static AnimationSampler read_animation_sampler(const Json& src);
static Json write_animation_sampler(const AnimationSampler& src);

// Accessors & Buffers
static Accessor read_accessor(const Json& src, const std::vector<GLTFBufferView>& buffer_views, Buffer bin_chunk);
static Json write_accessor(const Accessor& src, std::vector<GLTFBufferView>& buffer_views, OutBuffer bin_chunk);
static GLTFBufferView read_buffer_view(const Json& src);
static Json write_buffer_view(const GLTFBufferView& src);
static GLTFBuffer read_buffer(const Json& src);
static Json write_buffer(const GLTFBuffer& src);

// Miscellaneous
template <typename T> static void get_req(T& dest, const Json& src, const char* property);
template <typename T> static void set_req(Json& dest, const char* property, const T& value);
template <typename T> static void get_opt(Opt<T>& dest, const Json& src, const char* property);
template <typename T> static void set_opt(Json& dest, const char* property, const Opt<T>& value);
template <typename T> static void get_array(std::vector<T>& dest, const Json& src, const char* property);
template <typename T> static void set_array(Json& dest, const char* property, const std::vector<T>& value);
template <typename T> static void get_vec(Opt<T>& dest, const Json& src, const char* property);
template <typename T> static void set_vec(Json& dest, const char* property, const Opt<T>& value);
template <typename T> static void get_mat(Opt<T>& dest, const Json& src, const char* property);
template <typename T> static void set_mat(Json& dest, const char* property, const Opt<T>& value);
template <typename Object, typename ReadFunc>
static void read_object(Object& dest, const Json& src, const char* property, ReadFunc read_func);
template <typename Object, typename WriteFunc, typename... Args>
static void write_object(Json& dest, const char* property, const Object& src, WriteFunc write_func);
template <typename Element, typename ReadFunc, typename... Args>
static void read_array(std::vector<Element>& dest, const Json& src, const char* property, ReadFunc read_func, Args&... args);
template <typename Element, typename WriteFunc, typename... Args>
static void write_array(Json& dest, const char* property, const std::vector<Element>& src, WriteFunc write_func, Args&... args);
static Opt<MeshPrimitiveAttribute> mesh_primitive_attribute_from_string(const char* string);
static const char* accessor_type_to_string(AccessorType type);
static Opt<AccessorType> accessor_type_from_string(const char* string);
static const char* material_alpha_mode_to_string(MaterialAlphaMode alpha_mode);
static Opt<MaterialAlphaMode> material_alpha_mode_from_string(const char* string);
static s32 accessor_attribute_size(const Accessor& accessor);
static s32 accessor_component_size(AccessorComponentType component_type);
static s32 accessor_component_count(AccessorType type);

#define FOURCC(string) ((string)[0] | (string)[1] << 8 | (string)[2] << 16 | (string)[3] << 24)

ModelFile read_glb(Buffer src) {
	const GLBHeader& header = src.read<GLBHeader>(0);
	
	// The format is made up of a stream of chunks. Find them.
	s64 json_offset = -1;
	s64 json_size = -1;
	s64 bin_offset = -1;
	s64 bin_size = -1;
	for(s64 offset = sizeof(GLBHeader); offset < header.length;) {
		const GLBChunk& chunk = src.read<GLBChunk>(offset);
		if(chunk.type == FOURCC("JSON")) {
			json_offset = offset + sizeof(GLBChunk);
			json_size = chunk.length;
		} else if(chunk.type == FOURCC("BIN\x00")) {
			bin_offset = offset + sizeof(GLBChunk);
			bin_size = chunk.length;
		}
		offset += sizeof(GLBChunk) + chunk.length;
	}
	
	verify(json_offset >= 0 && json_size >= 0 && src.lo + json_offset + json_size <= src.hi, "No valid JSON chunk present.");
	verify(bin_offset >= 0 && bin_size >= 0 && src.lo + bin_offset + bin_size <= src.hi, "No valid BIN chunk present.");
	
	ModelFile gltf;
	try {
		auto json = Json::parse(src.lo + json_offset, src.lo + json_offset + json_size);
		gltf = read_gltf(json, src.subbuf(bin_offset, bin_size));
	} catch(Json::exception& e) {
		verify_not_reached("%s", e.what());
	}
	
	return gltf;
}

std::vector<u8> write_glb(const ModelFile& gltf) {
	std::vector<u8> result;
	OutBuffer dest(result);
	
	std::vector<u8> bin_chunk;
	Json root = write_gltf(gltf, bin_chunk);
	
	std::string json = root.dump();
	
	json.resize(align64(json.size(), 4), ' ');
	size_t padded_binary_size = align64(bin_chunk.size(), 4);
	
	GLBHeader header;
	header.magic = FOURCC("glTF");
	header.version = 2;
	header.length = sizeof(GLBHeader) + sizeof(GLBChunk) + json.size() + sizeof(GLBChunk) + padded_binary_size;
	dest.write(header);
	
	GLBChunk json_header;
	json_header.length = json.size();
	json_header.type = FOURCC("JSON");
	dest.write(json_header);
	dest.write_multiple(json);
	
	GLBChunk binary_header;
	binary_header.length = padded_binary_size;
	binary_header.type = FOURCC("BIN\00");
	dest.write(binary_header);
	dest.write_multiple(bin_chunk);
	for(size_t i = bin_chunk.size(); (i % 4) != 0; i++) {
		dest.write<u8>(0);
	}
	
	return result;
}

DefaultScene create_default_scene(const char* generator) {
	DefaultScene result;
	result.gltf.asset.generator = generator;
	result.gltf.asset.version = "2.0";
	result.gltf.scene = 0;
	result.scene = &result.gltf.scenes.emplace_back();
	return result;
}

Node* lookup_node(ModelFile& gltf, const char* name) {
	for(Node& node : gltf.nodes) {
		if(node.name.has_value() && strcmp(node.name->c_str(), name) == 0) {
			return &node;
		}
	}
	return nullptr;
}

Mesh* lookup_mesh(ModelFile& gltf, const char* name) {
	for(Mesh& mesh : gltf.meshes) {
		if(mesh.name.has_value() && strcmp(mesh.name->c_str(), name) == 0) {
			return &mesh;
		}
	}
	return nullptr;
}

Material* lookup_material(ModelFile& gltf, const char* name) {
	for(Material& material : gltf.materials) {
		if(material.name.has_value() && strcmp(material.name->c_str(), name) == 0) {
			return &material;
		}
	}
	return nullptr;
}

void deduplicate_vertices(Mesh& mesh) {
	size_t old_vertex_count = mesh.vertices.size();
	
	// Map duplicate vertices onto their "canonical" equivalents.
	std::vector<u32> canonical_vertices(old_vertex_count);
	s32 unique_vertex_count = mark_duplicates(mesh.vertices,
		[](const Vertex& lhs, const Vertex& rhs) {
			if(lhs < rhs) {
				return -1;
			} else if(lhs == rhs) {
				return 0;
			} else {
				return 1;
			}
		},
		[&](s32 index, s32 canonical) {
			canonical_vertices[index] = (u32) canonical;
		});
	
	// Copy over the unique vertices, preserving their original ordering.
	std::vector<Vertex> new_vertices;
	new_vertices.reserve(unique_vertex_count);
	for(size_t i = 0; i < old_vertex_count; i++) {
		if(i == canonical_vertices[i]) {
			canonical_vertices[i] = (s32) new_vertices.size();
			new_vertices.emplace_back(mesh.vertices[i]);
		} else {
			canonical_vertices[i] = canonical_vertices[canonical_vertices[i]];
		}
	}
	
	mesh.vertices = std::move(new_vertices);
	
	// Map the indices.
	for(MeshPrimitive& primitive : mesh.primitives) {
		for(u32& index : primitive.indices) {
			verify(index < old_vertex_count, "Index too large.");
			index = canonical_vertices[index];
		}
	}
}

void remove_zero_area_triangles(Mesh& mesh) {
	for(MeshPrimitive& primitive : mesh.primitives) {
		std::vector<u32> old_indices = std::move(primitive.indices);
		primitive.indices = {};
		for(size_t i = 0; i < old_indices.size() / 3; i++) {
			u32 v0 = old_indices[i * 3 + 0];
			u32 v1 = old_indices[i * 3 + 1];
			u32 v2 = old_indices[i * 3 + 2];
			if(!(v0 == v1 || v0 == v2 || v1 == v2)) {
				primitive.indices.emplace_back(v0);
				primitive.indices.emplace_back(v1);
				primitive.indices.emplace_back(v2);
			}
		}
	}
}

void fix_winding_orders_of_triangles_based_on_normals(Mesh& mesh) {
	for(MeshPrimitive& primitive : mesh.primitives) {
		for(size_t i = 0; i < primitive.indices.size() / 3; i++) {
			Vertex& v0 = mesh.vertices[primitive.indices[i * 3 + 0]];
			Vertex& v1 = mesh.vertices[primitive.indices[i * 3 + 1]];
			Vertex& v2 = mesh.vertices[primitive.indices[i * 3 + 2]];
			glm::vec3 stored_normal = (v0.normal + v1.normal + v2.normal) / 3.f;
			glm::vec3 calculated_normal = glm::cross(v1.pos - v0.pos, v2.pos - v0.pos);
			if(glm::dot(calculated_normal, stored_normal) < 0.f) {
				std::swap(primitive.indices[i * 3 + 0], primitive.indices[i * 3 + 2]);
			}
		}
	}
}

void map_gltf_materials_to_wrench_materials(ModelFile& gltf, const std::vector<::Material>& materials) {
	// Generate mapping.
	std::vector<s32> mapping(gltf.materials.size(), -1);
	for(size_t i = 0; i < gltf.materials.size(); i++) {
		bool mapped = false;
		for(size_t j = 0; j < materials.size(); j++) {
			if(gltf.materials[i].name.has_value() && materials[j].name == *gltf.materials[i].name) {
				mapping[i] = (s32) j;
				mapped = true;
			}
		}
		if(gltf.materials[i].name.has_value()) {
			verify(mapped, "GLTF material '%s' has no corresponding Material asset.", gltf.materials[i].name->c_str());
		} else {
			verify(mapped, "GLTF material %d has no corresponding Material asset.", (s32) i);
		}
	}
	
	// Apply mapping.
	for(Mesh& mesh : gltf.meshes) {
		for(MeshPrimitive& primitive : mesh.primitives) {
			if(primitive.material.has_value()) {
				primitive.material = mapping[*primitive.material];
			}
		}
	}
}

// *****************************************************************************
// GLTF, Scenes & Nodes
// *****************************************************************************

static ModelFile read_gltf(const Json& src, Buffer bin_chunk) {
	std::vector<GLTFBufferView> buffer_views;
	read_array(buffer_views, src, "bufferViews", read_buffer_view);
	
	std::vector<GLTFBuffer> buffers;
	read_array(buffers, src, "buffers", read_buffer);
	verify(buffers.size() <= 1, "GLB file has more than one buffer.");
	
	std::vector<Accessor> accessors;
	read_array(accessors, src, "accessors", read_accessor, buffer_views, bin_chunk);
	
	ModelFile dest;
	read_object(dest.asset, src, "asset", read_asset);
	get_array(dest.extensions_used, src, "extensionsUsed");
	get_array(dest.extensions_required, src, "extensionsRequired");
	get_opt(dest.scene, src, "scene");
	read_array(dest.scenes, src, "scenes", read_scene);
	read_array(dest.nodes, src, "nodes", read_node);
	read_array(dest.meshes, src, "meshes", read_mesh, accessors);
	read_array(dest.materials, src, "materials", read_material);
	read_array(dest.textures, src, "textures", read_texture);
	read_array(dest.images, src, "images", read_image);
	read_array(dest.samplers, src, "samplers", read_sampler);
	read_array(dest.skins, src, "skins", read_skin, accessors);
	read_array(dest.animations, src, "animations", read_animation, accessors);
	return dest;
}

static Json write_gltf(const ModelFile& src, OutBuffer bin_chunk) {
	std::vector<Accessor> accessors;
	std::vector<GLTFBufferView> buffer_views;
	
	Json dest = Json::object();
	write_object(dest, "asset", src.asset, write_asset);
	set_opt(dest, "scene", src.scene);
	write_array(dest, "scenes", src.scenes, write_scene);
	write_array(dest, "nodes", src.nodes, write_node);
	write_array(dest, "meshes", src.meshes, write_mesh, accessors);
	write_array(dest, "materials", src.materials, write_material);
	write_array(dest, "textures", src.textures, write_texture);
	write_array(dest, "images", src.images, write_image);
	write_array(dest, "samplers", src.samplers, write_sampler);
	write_array(dest, "skins", src.skins, write_skin, accessors);
	write_array(dest, "animations", src.animations, write_animation, accessors);
	write_array(dest, "accessors", accessors, write_accessor, buffer_views, bin_chunk);
	write_array(dest, "bufferViews", buffer_views, write_buffer_view);
	std::vector<GLTFBuffer> gltf_buffers {{std::nullopt, (s32) bin_chunk.tell(), std::nullopt}};
	write_array(dest, "buffers", gltf_buffers, write_buffer);
	set_array(dest, "extensionsUsed", src.extensions_used);
	set_array(dest, "extensionsRequired", src.extensions_required);
	
	return dest;
}

static Asset read_asset(const Json& src) {
	Asset dest;
	get_opt(dest.copyright, src, "copyright");
	get_opt(dest.generator, src, "generator");
	get_opt(dest.min_version, src, "minVersion");
	get_req(dest.version, src, "version");
	return dest;
}

static Json write_asset(const Asset& src) {
	Json dest = Json::object();
	set_opt(dest, "copyright", src.copyright);
	set_opt(dest, "generator", src.generator);
	set_opt(dest, "minVersion", src.min_version);
	set_req(dest, "version", src.version);
	return dest;
}

static Scene read_scene(const Json& src) {
	Scene dest;
	get_opt(dest.name, src, "name");
	get_array(dest.nodes, src, "nodes");
	return dest;
}

static Json write_scene(const Scene& src) {
	Json dest = Json::object();
	set_opt(dest, "name", src.name);
	set_array(dest, "nodes", src.nodes);
	return dest;
}

static Node read_node(const Json& src) {
	Node dest;
	get_array(dest.children, src, "children");
	get_mat(dest.matrix, src, "matrix");
	get_opt(dest.mesh, src, "mesh");
	get_opt(dest.name, src, "name");
	get_vec(dest.rotation, src, "rotation");
	get_vec(dest.scale, src, "scale");
	get_opt(dest.skin, src, "skin");
	get_vec(dest.translation, src, "translation");
	return dest;
}

static Json write_node(const Node& src) {
	Json dest = Json::object();
	set_array(dest, "children", src.children);
	set_mat(dest, "matrix", src.matrix);
	set_opt(dest, "mesh", src.mesh);
	set_opt(dest, "name", src.name);
	set_vec(dest, "rotation", src.rotation);
	set_vec(dest, "scale", src.scale);
	set_opt(dest, "skin", src.skin);
	set_vec(dest, "translation", src.translation);
	return dest;
}

// *****************************************************************************
// Meshes
// *****************************************************************************

static const glm::mat3 GLTF_TO_RATCHET_MATRIX = {
	0, 1, 0,
	0, 0, 1,
	1, 0, 0
};

static const glm::mat3 RATCHET_TO_GLTF_MATRIX = {
	0, 0, 1,
	1, 0, 0,
	0, 1, 0
};

static Mesh read_mesh(const Json& src, const std::vector<Accessor>& accessors) {
	Mesh dest;
	get_opt(dest.name, src, "name");
	read_array(dest.primitives, src, "primitives", read_mesh_primitive, dest.vertices, accessors);
	GLTF::deduplicate_vertices(dest);
	return dest;
}

static Json write_mesh(const Mesh& src, std::vector<Accessor>& accessors) {
	Json dest = Json::object();
	set_opt(dest, "name", src.name);
	write_array(dest, "primitives", src.primitives, write_mesh_primitive, src.vertices, accessors);
	return dest;
}

static MeshPrimitive read_mesh_primitive(const Json& src, std::vector<Vertex>& vertices_dest, const std::vector<Accessor>& accessors) {
	MeshPrimitive dest;
	
	auto attributes = src.find("attributes");
	verify(attributes != src.end(), "Missing 'attributes' property.");
	
	s32 vertex_count = 0;
	for(auto& [string, accessor_index] : attributes->items()) {
		verify(accessor_index >= 0 && accessor_index < accessors.size(),
			"Mesh primitive has an attribute accessor index which is out of range.");
		const Accessor& accessor = accessors[accessor_index];
		vertex_count = std::max(accessor.count, vertex_count);
	}
	u32 base_index = (u32) vertices_dest.size();
	vertices_dest.resize(base_index + vertex_count);
	
	for(auto& [string, accessor_index] : attributes->items()) {
		Opt<MeshPrimitiveAttribute> semantic = mesh_primitive_attribute_from_string(string.c_str());
		if(semantic.has_value()) {
			const Accessor& accessor = accessors[accessor_index];
			if(read_attribute(&vertices_dest[base_index], *semantic, accessor)) {
				dest.attributes_bitfield |= *semantic;
			}
		}
	}
	
	dest.indices = read_indices(src, accessors);
	for(u32& index : dest.indices) {
		index += base_index;
	}
	
	get_opt(dest.material, src, "material");
	Opt<s32> mode;
	get_opt(mode, src, "mode");
	if(mode.has_value()) {
		dest.mode = (MeshPrimitiveMode) *mode;
	}
	return dest;
}

static Json write_mesh_primitive(const MeshPrimitive& src, const std::vector<Vertex>& vertices_src, std::vector<Accessor>& accessors) {
	// Filter out vertices that are not included in this primitive.
	std::vector<Vertex> vertices;
	std::vector<u32> indices;
	std::vector<s32> mappings(vertices_src.size(), -1);
	indices.reserve(src.indices.size());
	for(u32 src_index : src.indices) {
		s32& dest_index = mappings.at(src_index);
		if(dest_index == -1) {
			dest_index = (s32) vertices.size();
			vertices.emplace_back(vertices_src[src_index]);
		}
		indices.emplace_back(dest_index);
	}
	
	Json dest = Json::object();
	dest["attributes"] = write_attributes(src, vertices, accessors);
	write_indices(dest, indices, accessors);
	set_opt(dest, "material", src.material);
	if(src.mode.has_value()) {
		set_opt(dest, "mode", Opt<s32>((s32) *src.mode));
	}
	return dest;
}

static bool read_attribute(Vertex* dest, MeshPrimitiveAttribute semantic, const Accessor& accessor) {
	switch(semantic) {
		case POSITION: {
			verify(accessor.type == VEC3 && accessor.component_type == FLOAT,
				"POSITION attribute is not a VEC3 of FLOAT components.");
			for(size_t i = 0; i < accessor.count; i++) {
				dest[i].pos = GLTF_TO_RATCHET_MATRIX * *(glm::vec3*) &accessor.bytes[i * sizeof(glm::vec3)];
			}
			break;
		}
		case TEXCOORD_0: {
			std::vector<glm::vec2> tex_coords = convert_tex_coords(accessor);
			for(size_t i = 0; i < tex_coords.size(); i++) {
				dest[i].tex_coord = tex_coords[i];
			}
			break;
		}
		case NORMAL: {
			verify(accessor.type == VEC3 && accessor.component_type == FLOAT,
				"NORMAL attribute is not a VEC3 of FLOAT components.");
			for(size_t i = 0; i < accessor.count; i++) {
				dest[i].normal = *(glm::vec3*) &accessor.bytes[i * sizeof(glm::vec3)];
			}
			break;
		}
		case COLOR_0: {
			std::vector<ColourAttribute> colours = convert_colours(accessor);
			for(size_t i = 0; i < colours.size(); i++) {
				dest[i].colour = colours[i];
			}
			break;
		}
		case JOINTS_0: {
			std::vector<std::array<s8, 3>> joints = convert_joints(accessor);
			for(size_t i = 0; i < joints.size(); i++) {
				dest[i].skin.joints[0] = joints[i][0];
				dest[i].skin.joints[1] = joints[i][1];
				dest[i].skin.joints[2] = joints[i][2];
			}
			break;
		}
		case WEIGHTS_0: {
			std::vector<std::array<u8, 3>> weights = convert_weights(accessor);
			for(size_t i = 0; i < weights.size(); i++) {
				dest[i].skin.weights[0] = weights[i][0];
				dest[i].skin.weights[1] = weights[i][1];
				dest[i].skin.weights[2] = weights[i][2];
			}
			break;
		}
		default: {
			return false;
		}
	}
	return true;
}

static Json write_attributes(const MeshPrimitive& src, const std::vector<Vertex>& vertices, std::vector<Accessor>& accessors) {
	Json dest = Json::object();
	if(src.attributes_bitfield & POSITION) {
		dest["POSITION"] = accessors.size();
		Accessor& accessor = accessors.emplace_back();
		accessor.bytes.resize(vertices.size() * sizeof(glm::vec3));
		for(size_t i = 0; i < vertices.size(); i++) {
			*(glm::vec3*) &accessor.bytes[i * sizeof(glm::vec3)] = RATCHET_TO_GLTF_MATRIX * vertices[i].pos;
		}
		accessor.component_type = FLOAT;
		accessor.count = (s32) vertices.size();
		accessor.type = VEC3;
		if(!vertices.empty()) {
			for(s32 i = 0; i < 3; i++) {
				f32 max = std::numeric_limits<f32>::min();
				for(size_t j = 0; j < vertices.size(); j++) {
					max = std::max(*(f32*) &accessor.bytes[j * sizeof(glm::vec3) + i * 4], max);
				}
				accessor.max.emplace_back(max);
			}
			for(s32 i = 0; i < 3; i++) {
				f32 min = std::numeric_limits<f32>::max();
				for(size_t j = 0; j < vertices.size(); j++) {
					min = std::min(*(f32*) &accessor.bytes[j * sizeof(glm::vec3) + i * 4], min);
				}
				accessor.min.emplace_back(min);
			}
		}
		accessor.target = ARRAY_BUFFER;
	}
	if(src.attributes_bitfield & TEXCOORD_0) {
		dest["TEXCOORD_0"] = accessors.size();
		Accessor& accessor = accessors.emplace_back();
		accessor.bytes.resize(vertices.size() * sizeof(glm::vec2));
		for(size_t i = 0; i < vertices.size(); i++) {
			*(glm::vec2*) &accessor.bytes[i * sizeof(glm::vec2)] = vertices[i].tex_coord;
		}
		accessor.component_type = FLOAT;
		accessor.count = (s32) vertices.size();
		accessor.type = VEC2;
		accessor.target = ARRAY_BUFFER;
	}
	if(src.attributes_bitfield & NORMAL) {
		dest["NORMAL"] = accessors.size();
		Accessor& accessor = accessors.emplace_back();
		accessor.bytes.resize(vertices.size() * sizeof(glm::vec3));
		for(size_t i = 0; i < vertices.size(); i++) {
			*(glm::vec3*) &accessor.bytes[i * sizeof(glm::vec3)] = vertices[i].normal;
		}
		accessor.component_type = FLOAT;
		accessor.count = (s32) vertices.size();
		accessor.type = VEC3;
		accessor.target = ARRAY_BUFFER;
	}
	if(src.attributes_bitfield & COLOR_0) {
		dest["COLOR_0"] = accessors.size();
		Accessor& accessor = accessors.emplace_back();
		accessor.bytes.resize(vertices.size() * 4);
		for(size_t i = 0; i < vertices.size(); i++) {
			accessor.bytes[i * 4 + 0] = vertices[i].colour.r;
			accessor.bytes[i * 4 + 1] = vertices[i].colour.g;
			accessor.bytes[i * 4 + 2] = vertices[i].colour.b;
			accessor.bytes[i * 4 + 3] = vertices[i].colour.a;
		}
		accessor.component_type = UNSIGNED_BYTE;
		accessor.normalized = true;
		accessor.count = (s32) vertices.size();
		accessor.type = VEC4;
		accessor.target = ARRAY_BUFFER;
	}
	if(src.attributes_bitfield & JOINTS_0) {
		dest["JOINTS_0"] = accessors.size();
		Accessor& accessor = accessors.emplace_back();
		accessor.bytes.resize(vertices.size() * 4, 0);
		for(size_t i = 0; i < vertices.size(); i++) {
			accessor.bytes[i * 4 + 0] = vertices[i].skin.joints[0];
			accessor.bytes[i * 4 + 1] = vertices[i].skin.joints[1];
			accessor.bytes[i * 4 + 2] = vertices[i].skin.joints[2];
		}
		accessor.component_type = UNSIGNED_BYTE;
		accessor.count = (s32) vertices.size();
		accessor.type = VEC4;
		accessor.target = ARRAY_BUFFER;
	}
	if(src.attributes_bitfield & WEIGHTS_0) {
		dest["WEIGHTS_0"] = accessors.size();
		Accessor& accessor = accessors.emplace_back();
		accessor.bytes.resize(vertices.size() * 4, 0);
		for(size_t i = 0; i < vertices.size(); i++) {
			accessor.bytes[i * 4 + 0] = vertices[i].skin.weights[0];
			accessor.bytes[i * 4 + 1] = vertices[i].skin.weights[1];
			accessor.bytes[i * 4 + 2] = vertices[i].skin.weights[2];
		}
		accessor.component_type = UNSIGNED_BYTE;
		accessor.normalized = true;
		accessor.count = (s32) vertices.size();
		accessor.type = VEC4;
		accessor.target = ARRAY_BUFFER;
	}
	return dest;
}

static std::vector<glm::vec2> convert_tex_coords(const Accessor& accessor) {
	verify(accessor.type == VEC2, "TEXCOORD attribute is not a VEC2.");
	std::vector<glm::vec2> tex_coords(accessor.count);
	switch(accessor.component_type) {
		case FLOAT: {
			for(s32 i = 0; i < accessor.count; i++) {
				tex_coords[i][0] = *(f32*) &accessor.bytes.at(i * 8 + 0);
				tex_coords[i][1] = *(f32*) &accessor.bytes.at(i * 8 + 4);
			}
			break;
		}
		case UNSIGNED_BYTE: {
			for(s32 i = 0; i < accessor.count; i++) {
				tex_coords[i][0] = (*(u8*) &accessor.bytes.at(i * 2 + 0)) / 255.f;
				tex_coords[i][1] = (*(u8*) &accessor.bytes.at(i * 2 + 1)) / 255.f;
			}
			break;
		}
		case UNSIGNED_SHORT: {
			for(s32 i = 0; i < accessor.count; i++) {
				tex_coords[i][0] = (*(u16*) &accessor.bytes.at(i * 4 + 0)) / 65535.f;
				tex_coords[i][1] = (*(u16*) &accessor.bytes.at(i * 4 + 2)) / 65535.f;
			}
			break;
		}
		default: {
			verify_not_reached("TEXCOORD attribute has an invalid component type (should be FLOAT, UNSIGNED_BYTE or UNSIGNED_SHORT)");
		}
	}
	return tex_coords;
}

static std::vector<ColourAttribute> convert_colours(const Accessor& accessor) {
	verify(accessor.type == VEC3 || accessor.type == VEC4, "TEXCOORD attribute is not a VEC3 or a VEC4.");
	std::vector<ColourAttribute> colours(accessor.count);
	if(accessor.type == VEC3) {
		switch(accessor.component_type) {
			case FLOAT: {
				for(s32 i = 0; i < accessor.count; i++) {
					colours[i].r = (u8) ((*(f32*) &accessor.bytes.at(i * 12 + 0)) * 255.f);
					colours[i].g = (u8) ((*(f32*) &accessor.bytes.at(i * 12 + 4)) * 255.f);
					colours[i].b = (u8) ((*(f32*) &accessor.bytes.at(i * 12 + 8)) * 255.f);
					colours[i].a = 255;
				}
				break;
			}
			case UNSIGNED_BYTE: {
				for(s32 i = 0; i < accessor.count; i++) {
					colours[i].r = accessor.bytes.at(i * 3 + 0);
					colours[i].g = accessor.bytes.at(i * 3 + 1);
					colours[i].b = accessor.bytes.at(i * 3 + 2);
					colours[i].a = 255;
				}
				break;
			}
			case UNSIGNED_SHORT: {
				for(s32 i = 0; i < accessor.count; i++) {
					colours[i].r = (u8) ((*(u16*) &accessor.bytes.at(i * 6 + 0)) * (1.f / 256.f));
					colours[i].g = (u8) ((*(u16*) &accessor.bytes.at(i * 6 + 2)) * (1.f / 256.f));
					colours[i].b = (u8) ((*(u16*) &accessor.bytes.at(i * 6 + 4)) * (1.f / 256.f));
					colours[i].a = 255;
				}
				break;
			}
			default: {
				verify_not_reached("COLOR_0 attribute has an invalid component type (should be FLOAT, UNSIGNED_BYTE or UNSIGNED_SHORT)");
			}
		}
	} else {
		switch(accessor.component_type) {
			case FLOAT: {
				for(s32 i = 0; i < accessor.count; i++) {
					colours[i].r = (u8) ((*(f32*) &accessor.bytes.at(i * 16 + 0)) * 255.f);
					colours[i].g = (u8) ((*(f32*) &accessor.bytes.at(i * 16 + 4)) * 255.f);
					colours[i].b = (u8) ((*(f32*) &accessor.bytes.at(i * 16 + 8)) * 255.f);
					colours[i].a = (u8) ((*(f32*) &accessor.bytes.at(i * 16 + 12)) * 255.f);
				}
				break;
			}
			case UNSIGNED_BYTE: {
				for(s32 i = 0; i < accessor.count; i++) {
					colours[i].r = accessor.bytes.at(i * 4 + 0);
					colours[i].g = accessor.bytes.at(i * 4 + 1);
					colours[i].b = accessor.bytes.at(i * 4 + 2);
					colours[i].a = accessor.bytes.at(i * 4 + 3);
				}
				break;
			}
			case UNSIGNED_SHORT: {
				for(s32 i = 0; i < accessor.count; i++) {
					colours[i].r = (u8) ((*(u16*) &accessor.bytes.at(i * 8 + 0)) * (1.f / 256.f));
					colours[i].g = (u8) ((*(u16*) &accessor.bytes.at(i * 8 + 2)) * (1.f / 256.f));
					colours[i].b = (u8) ((*(u16*) &accessor.bytes.at(i * 8 + 4)) * (1.f / 256.f));
					colours[i].a = (u8) ((*(u16*) &accessor.bytes.at(i * 8 + 6)) * (1.f / 256.f));
				}
				break;
			}
			default: {
				verify_not_reached("COLOR_0 attribute has an invalid component type (should be FLOAT, UNSIGNED_BYTE or UNSIGNED_SHORT)");
			}
		}
	}
	return colours;
}

static std::vector<std::array<s8, 3>> convert_joints(const Accessor& accessor) {
	verify(accessor.type == VEC4, "JOINTS_0 attribute is not a VEC4.");
	std::vector<std::array<s8, 3>> joints(accessor.count);
	switch(accessor.component_type) {
		case UNSIGNED_BYTE: {
			for(s32 i = 0; i < accessor.count; i++) {
				joints[i][0] = accessor.bytes.at(i * 4 + 0);
				joints[i][1] = accessor.bytes.at(i * 4 + 1);
				joints[i][2] = accessor.bytes.at(i * 4 + 2);
				joints[i][3] = accessor.bytes.at(i * 4 + 3);
			}
			break;
		}
		case UNSIGNED_SHORT: {
			for(s32 i = 0; i < accessor.count; i++) {
				joints[i][0] = accessor.bytes.at(i * 8 + 0);
				joints[i][1] = accessor.bytes.at(i * 8 + 2);
				joints[i][2] = accessor.bytes.at(i * 8 + 4);
				joints[i][3] = accessor.bytes.at(i * 8 + 6);
			}
			break;
		}
		default: {
			verify_not_reached("JOINTS_0 attribute has an invalid component type (should be UNSIGNED_BYTE or UNSIGNED_SHORT)");
		}
	}
	return joints;
}

static std::vector<std::array<u8, 3>> convert_weights(const Accessor& accessor) {
	verify(accessor.type == VEC4, "WEIGHTS_0 attribute is not a VEC4.");
	std::vector<std::array<u8, 3>> weights(accessor.count);
	switch(accessor.component_type) {
		case FLOAT: {
			for(s32 i = 0; i < accessor.count; i++) {
				weights[i][0] = (*(f32*) &accessor.bytes.at(i * 8 + 0)) / 255.f;
				weights[i][1] = (*(f32*) &accessor.bytes.at(i * 8 + 4)) / 255.f;
			}
			break;
		}
		case UNSIGNED_BYTE: {
			for(s32 i = 0; i < accessor.count; i++) {
				weights[i][0] = *(u8*) &accessor.bytes.at(i * 2 + 0);
				weights[i][1] = *(u8*) &accessor.bytes.at(i * 2 + 1);
			}
			break;
		}
		case UNSIGNED_SHORT: {
			for(s32 i = 0; i < accessor.count; i++) {
				weights[i][0] = (u8) ((*(u16*) &accessor.bytes.at(i * 4 + 0)) * (1 / 256.f));
				weights[i][1] = (u8) ((*(u16*) &accessor.bytes.at(i * 4 + 2)) * (1 / 256.f));
			}
			break;
		}
		default: {
			verify_not_reached("WEIGHTS_0 attribute has an invalid component type (should be FLOAT, UNSIGNED_BYTE or UNSIGNED_SHORT)");
		}
	}
	return weights;
}

static std::vector<u32> read_indices(const Json& src, const std::vector<Accessor>& accessors) {
	std::vector<u32> indices;
	
	Opt<s32> indices_accessor_index;
	get_opt(indices_accessor_index, src, "indices");
	verify(indices_accessor_index.has_value(), "Support for non-indexed geometry not yet implemented.");
	verify(*indices_accessor_index >= 0 && *indices_accessor_index < accessors.size(),
		"Mesh primitive has indices accessor index which is out of range.");
	
	const Accessor& indices_accessor = accessors[*indices_accessor_index];
	verify(indices_accessor.type == SCALAR, "Indices accessor has an invalid type (must be a SCALAR).");
	indices.resize(indices_accessor.count);
	switch(indices_accessor.component_type) {
		case UNSIGNED_BYTE: {
			for(s32 i = 0; i < indices_accessor.count; i++) {
				indices[i] = indices_accessor.bytes[i];
			}
			break;
		}
		case UNSIGNED_SHORT: {
			for(s32 i = 0; i < indices_accessor.count; i++) {
				indices[i] = *(u16*) &indices_accessor.bytes[i * 2];
			}
			break;
		}
		case UNSIGNED_INT: {
			for(s32 i = 0; i < indices_accessor.count; i++) {
				indices[i] = *(u32*) &indices_accessor.bytes[i * 4];
			}
			break;
		}
		default: {
			verify_not_reached("Indices accessor has an invalid component type (must be UNSIGNED_BYTE, UNSIGNED_SHORT or UNSIGNED_INT).");
		}
	}
	
	return indices;
}

static void write_indices(Json& dest, const std::vector<u32>& indices, std::vector<Accessor>& accessors) {
	u32 max_index = 0;
	for(u32 index : indices) {
		max_index = std::max(index, max_index);
	}
	
	set_opt(dest, "indices", Opt<s32>((s32) accessors.size()));
	Accessor& index_accessor = accessors.emplace_back();
	index_accessor.count = (s32) indices.size();
	index_accessor.type = SCALAR;
	if(max_index < 255) {
		index_accessor.bytes.resize(indices.size());
		for(size_t i = 0; i < indices.size(); i++) {
			*(u8*) &index_accessor.bytes[i] = (u8) indices[i];
		}
		index_accessor.component_type = UNSIGNED_BYTE;
	} else if(max_index < 65535) {
		index_accessor.bytes.resize(indices.size() * 2);
		for(size_t i = 0; i < indices.size(); i++) {
			*(u16*) &index_accessor.bytes[i * 2] = (u16) indices[i];
		}
		index_accessor.component_type = UNSIGNED_SHORT;
	} else if(max_index < 4294967295) {
		index_accessor.bytes.resize(indices.size() * 4);
		for(size_t i = 0; i < indices.size(); i++) {
			*(u32*) &index_accessor.bytes[i * 4] = indices[i];
		}
		index_accessor.component_type = UNSIGNED_INT;
	} else {
		verify_not_reached("Index out of range.");
	}
	index_accessor.target = ELEMENT_ARRAY_BUFFER;
}

// *****************************************************************************
// Materials & Textures
// *****************************************************************************

static Material read_material(const Json& src) {
	Material dest;
	get_opt(dest.name, src, "name");
	if(src.contains("pbrMetallicRoughness")) {
		dest.pbr_metallic_roughness.emplace();
		read_object(*dest.pbr_metallic_roughness, src, "pbrMetallicRoughness", read_material_pbr_metallic_roughness);
	}
	Opt<std::string> alpha_mode_string;
	get_opt(alpha_mode_string, src, "alphaMode");
	if(alpha_mode_string.has_value()) {
		Opt<MaterialAlphaMode> alpha_mode = material_alpha_mode_from_string(alpha_mode_string->c_str());
		verify(alpha_mode.has_value(), "Material has unknown alpha mode '%s'.", alpha_mode_string->c_str());
		dest.alpha_mode = *alpha_mode;
	}
	get_opt(dest.double_sided, src, "doubleSided");
	return dest;
}

static Json write_material(const Material& src) {
	Json dest = Json::object();
	set_opt(dest, "name", src.name);
	if(src.pbr_metallic_roughness.has_value()) {
		write_object(dest, "pbrMetallicRoughness", *src.pbr_metallic_roughness, write_material_pbr_metallic_roughness);
	}
	if(src.alpha_mode.has_value()) {
		set_opt(dest, "alphaMode", Opt<const char*>(material_alpha_mode_to_string(*src.alpha_mode)));
	}
	set_opt(dest, "doubleSided", src.double_sided);
	return dest;
}

static MaterialPbrMetallicRoughness read_material_pbr_metallic_roughness(const Json& src) {
	MaterialPbrMetallicRoughness dest;
	get_vec(dest.base_color_factor, src, "baseColorFactor");
	if(src.contains("baseColorTexture")) {
		dest.base_color_texture.emplace();
		read_object(*dest.base_color_texture, src, "baseColorTexture", read_texture_info);
	}
	return dest;
}

static Json write_material_pbr_metallic_roughness(const MaterialPbrMetallicRoughness& src) {
	Json dest = Json::object();
	set_vec(dest, "baseColorFactor", src.base_color_factor);
	if(src.base_color_texture.has_value()) {
		write_object(dest, "baseColorTexture", *src.base_color_texture, write_texture_info);
	}
	return dest;
}

static TextureInfo read_texture_info(const Json& src) {
	TextureInfo dest;
	get_req(dest.index, src, "index");
	get_opt(dest.tex_coord, src, "texCoord");
	return dest;
}

static Json write_texture_info(const TextureInfo& src) {
	Json dest = Json::object();
	set_req(dest, "index", src.index);
	set_opt(dest, "texCoord", src.tex_coord);
	return dest;
}

static Texture read_texture(const Json& src) {
	Texture dest;
	get_opt(dest.name, src, "name");
	get_opt(dest.sampler, src, "sampler");
	get_opt(dest.source, src, "source");
	return dest;
}

static Json write_texture(const Texture& src) {
	Json dest = Json::object();
	set_opt(dest, "name", src.name);
	set_opt(dest, "sampler", src.sampler);
	set_opt(dest, "source", src.source);
	return dest;
}

static Image read_image(const Json& src) {
	Image dest;
	get_opt(dest.buffer_view, src, "bufferView");
	get_opt(dest.mime_type, src, "mimeType");
	get_opt(dest.name, src, "name");
	get_opt(dest.uri, src, "uri");
	return dest;
}

static Json write_image(const Image& src) {
	Json dest = Json::object();
	set_opt(dest, "bufferView", src.buffer_view);
	set_opt(dest, "mimeType", src.mime_type);
	set_opt(dest, "name", src.name);
	set_opt(dest, "uri", src.uri);
	return dest;
}

static Sampler read_sampler(const Json& src) {
	Sampler dest;
	get_opt(dest.mag_filter, src, "magFilter");
	get_opt(dest.min_filter, src, "minFilter");
	get_opt(dest.name, src, "name");
	get_opt(dest.wrap_s, src, "wrapS");
	get_opt(dest.wrap_t, src, "wrapT");
	return dest;
}

static Json write_sampler(const Sampler& src) {
	Json dest = Json::object();
	set_opt(dest, "magFilter", src.mag_filter);
	set_opt(dest, "minFilter", src.min_filter);
	set_opt(dest, "name", src.name);
	set_opt(dest, "wrapS", src.wrap_s);
	set_opt(dest, "wrapT", src.wrap_t);
	return dest;
}

// *****************************************************************************
// Animation
// *****************************************************************************

static Skin read_skin(const Json& src, const std::vector<Accessor>& accessors) {
	Skin dest;
	Opt<s32> inverse_bind_matrices_index;
	get_opt(inverse_bind_matrices_index, src, "inverseBindMatrices");
	if(inverse_bind_matrices_index.has_value()) {
		verify(*inverse_bind_matrices_index >= 0 && inverse_bind_matrices_index < accessors.size(),
			"Skin has invalid accessor index for the inverse bind matrices.");
		const Accessor& accessor = accessors[*inverse_bind_matrices_index];
		dest.inverse_bind_matrices.resize(accessor.count);
		verify(accessor.bytes.size() >= accessor.count * sizeof(glm::mat4), "Invalid accessor for inverse bind matrices.");
		memcpy(dest.inverse_bind_matrices.data(), accessor.bytes.data(), accessor.count * sizeof(glm::mat4));
	}
	get_array(dest.joints, src, "joints");
	get_opt(dest.name, src, "name");
	get_opt(dest.skeleton, src, "skeleton");
	return dest;
}

static Json write_skin(const Skin& src, std::vector<Accessor>& accessors) {
	Json dest = Json::object();
	if(!src.inverse_bind_matrices.empty()) {
		set_opt(dest, "inverseBindMatrices", Opt<s32>((s32) accessors.size()));
		Accessor& accessor = accessors.emplace_back();
		s64 size = src.inverse_bind_matrices.size() * sizeof(glm::mat4);
		accessor.bytes.resize(size);
		memcpy(accessor.bytes.data(), src.inverse_bind_matrices.data(), size);
		accessor.component_type = FLOAT;
		accessor.count = src.inverse_bind_matrices.size();
		accessor.type = MAT4;
	}
	
	set_array(dest, "joints", src.joints);
	set_opt(dest, "name", src.name);
	set_opt(dest, "skeleton", src.skeleton);
	return dest;
}

static Animation read_animation(const Json& src, const std::vector<Accessor>& accessors) {
	std::vector<AnimationSampler> samplers;
	read_array(samplers, src, "samplers", read_animation_sampler);
	s32 input_accessor_index = samplers.at(0).input;
	for(AnimationSampler& sampler : samplers) {
		verify(sampler.input == input_accessor_index,
			"Animation has samplers with different input accessor, which is not supported.");
	}
	verify(input_accessor_index >= 0 && input_accessor_index < accessors.size(),
		"Animation sampler has out of range input accessor index.")
	
	const Accessor& input_accessor = accessors[input_accessor_index];
	verify(input_accessor.type == SCALAR && input_accessor.component_type == FLOAT,
		"Animation sampler has an input accessor of the wrong type.");
	
	std::vector<AnimationChannel> channels;
	read_array(channels, src, "channels", read_animation_channel);
	
	std::unordered_map<s32, s32> group_lookup;
	
	Animation dest;
	get_opt(dest.name, src, "name");
	for(const AnimationChannel& channel : channels) {
		verify(channel.sampler >= 0 && channel.sampler < samplers.size(),
			"Animation has out of range sampler index.");
		const AnimationSampler& sampler = samplers[channel.sampler];
		verify(sampler.output >= 0 && sampler.output < accessors.size(),
			"Animation sampler has out of range output accessor index.");
		const Accessor& output_accessor = accessors[sampler.output];
		
		verify(channel.target.node.has_value(), "Animation channel target has no node property.");
		auto iter = group_lookup.find(*channel.target.node);
		AnimationChannelGroup* group;
		if(iter == group_lookup.end()) {
			group_lookup.emplace(*channel.target.node, (s32) dest.channel_groups.size());
			group = &dest.channel_groups.emplace_back();
			group->node = *channel.target.node;
		} else {
			group = &dest.channel_groups[iter->second];
		}
		
		group->frames.resize(std::max((s32) group->frames.size(), output_accessor.count));
		convert_animation_sampler_output(group->frames, channel.target.path.c_str(), output_accessor);
		return dest;
	}
	dest.sampler_input.resize(input_accessor.count);
	memcpy(dest.sampler_input.data(), input_accessor.bytes.data(), input_accessor.count * 4);
	return dest;
}

static Json write_animation(const Animation& src, std::vector<Accessor>& accessors) {
	s32 input_accessor_index = (s32) accessors.size();
	Accessor& input_accessor = accessors.emplace_back();
	input_accessor.bytes.resize(src.sampler_input.size() * 4);
	memcpy(input_accessor.bytes.data(), src.sampler_input.data(), src.sampler_input.size() * 4);
	input_accessor.component_type = FLOAT;
	input_accessor.count = (s32) src.sampler_input.size();
	if(!src.sampler_input.empty()) {
		f32 max = std::numeric_limits<f32>::min();
		for(f32 f : src.sampler_input) {
			max = std::max(f, max);
		}
		input_accessor.max = {max};
		
		f32 min = std::numeric_limits<f32>::max();
		for(f32 f : src.sampler_input) {
			min = std::min(f, min);
		}
		input_accessor.min = {min};
	}
	input_accessor.type = SCALAR;
	
	std::vector<AnimationChannel> channels;
	std::vector<AnimationSampler> samplers;
	for(const AnimationChannelGroup& group : src.channel_groups) {
		build_animation_channel(channels, samplers, accessors, group, &AnimationAttributes::translation, "translation", VEC3, input_accessor_index);
		build_animation_channel(channels, samplers, accessors, group, &AnimationAttributes::rotation, "rotation", VEC4, input_accessor_index);
		build_animation_channel(channels, samplers, accessors, group, &AnimationAttributes::scale, "scale", VEC3, input_accessor_index);
	}
	
	Json dest = Json::object();
	set_opt(dest, "name", src.name);
	write_array(dest, "channels", channels, write_animation_channel);
	write_array(dest, "samplers", samplers, write_animation_sampler);
	return dest;
}

static void convert_animation_sampler_output(std::vector<AnimationAttributes>& dest, const char* path, const Accessor& accessor) {
	if(strcmp(path, "translation") == 0) {
		verify(accessor.type == VEC3 && accessor.component_type == FLOAT,
			"Animation translation accessor is not of type VEC3 of FLOATs.");
		for(s32 i = 0; i < accessor.count; i++) {
			dest[i].translation = *(glm::vec3*) &accessor.bytes[i * sizeof(glm::vec3)];
		}
	} else if(strcmp(path, "rotation") == 0) {
		verify(accessor.type == VEC4, "Animation rotation accessor is not of type VEC4.");
		switch(accessor.component_type) {
			case FLOAT: {
				for(s32 i = 0; i < accessor.count; i++) {
					dest[i].rotation = *(glm::vec4*) &accessor.bytes[i * sizeof(glm::vec4)];
				}
				break;
			}
			case SIGNED_BYTE: {
				for(s32 i = 0; i < accessor.count; i++) {
					s8 x = *(s8*) &accessor.bytes[i * 4 + 0];
					s8 y = *(s8*) &accessor.bytes[i * 4 + 1];
					s8 z = *(s8*) &accessor.bytes[i * 4 + 2];
					s8 w = *(s8*) &accessor.bytes[i * 4 + 3];
					dest[i].rotation.x = std::max(x / 127.f, -1.f);
					dest[i].rotation.y = std::max(y / 127.f, -1.f);
					dest[i].rotation.z = std::max(z / 127.f, -1.f);
					dest[i].rotation.w = std::max(w / 127.f, -1.f);
				}
				break;
			}
			case UNSIGNED_BYTE: {
				for(s32 i = 0; i < accessor.count; i++) {
					u8 x = accessor.bytes[i * 4 + 0];
					u8 y = accessor.bytes[i * 4 + 1];
					u8 z = accessor.bytes[i * 4 + 2];
					u8 w = accessor.bytes[i * 4 + 3];
					dest[i].rotation.x = x / 255.f;
					dest[i].rotation.y = y / 255.f;
					dest[i].rotation.z = z / 255.f;
					dest[i].rotation.w = w / 255.f;
				}
				break;
			}
			case SIGNED_SHORT: {
				for(s32 i = 0; i < accessor.count; i++) {
					s16 x = *(s16*) &accessor.bytes[i * 8 + 0];
					s16 y = *(s16*) &accessor.bytes[i * 8 + 2];
					s16 z = *(s16*) &accessor.bytes[i * 8 + 4];
					s16 w = *(s16*) &accessor.bytes[i * 8 + 6];
					dest[i].rotation.x = std::max(x / 32767.f, -1.f);
					dest[i].rotation.y = std::max(y / 32767.f, -1.f);
					dest[i].rotation.z = std::max(z / 32767.f, -1.f);
					dest[i].rotation.w = std::max(w / 32767.f, -1.f);
				}
				break;
			}
			case UNSIGNED_SHORT: {
				for(s32 i = 0; i < accessor.count; i++) {
					u16 x = *(u16*) &accessor.bytes[i * 8 + 0];
					u16 y = *(u16*) &accessor.bytes[i * 8 + 2];
					u16 z = *(u16*) &accessor.bytes[i * 8 + 4];
					u16 w = *(u16*) &accessor.bytes[i * 8 + 6];
					dest[i].rotation.x = x / 65535.f;
					dest[i].rotation.y = y / 65535.f;
					dest[i].rotation.z = z / 65535.f;
					dest[i].rotation.w = w / 65535.f;
				}
				break;
			}
			default: {
				verify_not_reached("Animation rotation accessor has an invalid component type.");
			}
		}
	} else if(strcmp(path, "scale") == 0) {
		verify(accessor.type == VEC3 && accessor.component_type == FLOAT,
			"Animation scale accessor is not of type VEC3 of FLOATs.");
		for(s32 i = 0; i < accessor.count; i++) {
			dest[i].scale = *(glm::vec3*) &accessor.bytes[i * sizeof(glm::vec3)];
		}
	}
}

template <typename AttributeType>
static void build_animation_channel(
		std::vector<AnimationChannel>& channels,
		std::vector<AnimationSampler>& samplers,
		std::vector<Accessor>& accessors,
		const AnimationChannelGroup& group,
		AttributeType AnimationAttributes::*attribute,
		const char* path,
		AccessorType type,
		s32 input_accessor_index) {
	s32 sampler_index = (s32) samplers.size();
	AnimationSampler& sampler = samplers.emplace_back();
	sampler.input = input_accessor_index;
	sampler.interpolation = "LINEAR";
	sampler.output = (s32) accessors.size();
	
	Accessor& accessor = accessors.emplace_back();
	accessor.bytes.resize(group.frames.size() * sizeof(AttributeType));
	for(size_t i = 0; i < group.frames.size(); i++) {
		*(AttributeType*) &accessor.bytes[i * sizeof(AttributeType)] = group.frames[i].*attribute;
	}
	accessor.component_type = FLOAT;
	accessor.count = (s32) group.frames.size();
	accessor.type = VEC3;

	AnimationChannel& channel = channels.emplace_back();
	channel.sampler = sampler_index;
	channel.target.node = group.node;
	channel.target.path = path;
}

static AnimationChannel read_animation_channel(const Json& src) {
	AnimationChannel dest;
	get_req(dest.sampler, src, "sampler");
	read_object(dest.target, src, "target", read_animation_channel_target);
	return dest;
}

static Json write_animation_channel(const AnimationChannel& src) {
	Json dest = Json::object();
	set_req(dest, "sampler", src.sampler);
	write_object(dest, "target", src.target, write_animation_channel_target);
	return dest;
}

static AnimationChannelTarget read_animation_channel_target(const Json& src) {
	AnimationChannelTarget dest;
	get_opt(dest.node, src, "node");
	get_req(dest.path, src, "path");
	return dest;

}
static Json write_animation_channel_target(const AnimationChannelTarget& src) {
	Json dest = Json::object();
	set_opt(dest, "node", src.node);
	set_req(dest, "path", src.path);
	return dest;
}

static AnimationSampler read_animation_sampler(const Json& src) {
	AnimationSampler dest;
	get_req(dest.input, src, "input");
	get_opt(dest.interpolation, src, "interpolation");
	get_req(dest.output, src, "output");
	return dest;
}

static Json write_animation_sampler(const AnimationSampler& src) {
	Json dest = Json::object();
	set_req(dest, "input", src.input);
	set_opt(dest, "interpolation", src.interpolation);
	set_req(dest, "output", src.output);
	return dest;
}

// *****************************************************************************
// Accessors & Buffers
// *****************************************************************************

static Accessor read_accessor(const Json& src, const std::vector<GLTFBufferView>& buffer_views, Buffer bin_chunk) {
	Opt<s32> buffer_view_index;
	get_opt(buffer_view_index, src, "bufferView");
	verify(buffer_view_index.has_value(), "Accessor without a buffer view (unimplemented).");
	verify(*buffer_view_index >= 0 && *buffer_view_index < buffer_views.size(), "Accessor has invalid buffer view index.");
	
	Opt<s32> byte_offset;
	get_opt(byte_offset, src, "byteOffset");
	if(!byte_offset.has_value()) {
		byte_offset = 0;
	}
	
	Accessor dest;
	get_req(dest.component_type, src, "componentType");
	get_req(dest.count, src, "count");
	get_array(dest.max, src, "max");
	get_array(dest.min, src, "min");
	get_opt(dest.name, src, "name");
	get_opt(dest.normalized, src, "normalized");
	std::string type_string;
	get_req(type_string, src, "type");
	Opt<AccessorType> type = accessor_type_from_string(type_string.c_str());
	verify(type.has_value(), "Accessor has unknown type '%s'.", type_string.c_str());
	dest.type = *type;
	
	s32 attribute_size = accessor_attribute_size(dest);
	dest.bytes.resize(dest.count * attribute_size);
	
	const GLTFBufferView& buffer_view = buffer_views.at(*buffer_view_index);
	s32 byte_stride = buffer_view.byte_stride.has_value() ? *buffer_view.byte_stride : attribute_size;
	verify(buffer_view.buffer == 0, "GLB file has more than one buffer.");
	verify(buffer_view.byte_offset.has_value(), "Buffer view without a byte offset.");
	
	for(s32 i = 0; i < dest.count; i++) {
		s32 source_offset = *buffer_view.byte_offset + i * byte_stride + *byte_offset;
		verify(source_offset >= 0 && source_offset + attribute_size <= bin_chunk.size(), "Buffer view out of range.");
		memcpy(dest.bytes.data() + i * attribute_size, bin_chunk.lo + source_offset, attribute_size);
	}
	
	if(buffer_view.target.has_value()) dest.target = (BufferViewTarget) *buffer_view.target;
	
	return dest;
}

static Json write_accessor(const Accessor& src, std::vector<GLTFBufferView>& buffer_views, OutBuffer bin_chunk) {
	Json dest = Json::object();
	set_opt(dest, "bufferView", Opt<s32>((s32) buffer_views.size()));
	// byteOffset
	set_req(dest, "componentType", src.component_type);
	set_req(dest, "count", src.count);
	set_array(dest, "max", src.max);
	set_array(dest, "min", src.min);
	set_opt(dest, "name", src.name);
	set_opt(dest, "normalized", src.normalized);
	set_req(dest, "type", accessor_type_to_string(src.type));
	
	s32 alignment = 1;
	switch(src.component_type) {
		case SIGNED_BYTE:
		case UNSIGNED_BYTE: {
			alignment = 1;
			break;
		}
		case SIGNED_SHORT:
		case UNSIGNED_SHORT: {
			alignment = 2;
			break;
		}
		case UNSIGNED_INT:
		case FLOAT: {
			alignment = 4;
			break;
		}
	}
	bin_chunk.pad(alignment, 0);
	
	GLTFBufferView& buffer_view = buffer_views.emplace_back();
	buffer_view.buffer = 0;
	buffer_view.byte_offset = (s32) bin_chunk.tell();
	buffer_view.byte_length = (s32) src.bytes.size();
	if(src.target.has_value()) buffer_view.target = (s32) *src.target;
	
	bin_chunk.write_multiple(src.bytes);
	
	return dest;
}

static GLTFBufferView read_buffer_view(const Json& src) {
	GLTFBufferView dest;
	get_req(dest.buffer, src, "buffer");
	get_req(dest.byte_length, src, "byteLength");
	get_opt(dest.byte_offset, src, "byteOffset");
	get_opt(dest.byte_stride, src, "byteStride");
	get_opt(dest.name, src, "name");
	get_opt(dest.target, src, "target");
	return dest;
}

static Json write_buffer_view(const GLTFBufferView& src) {
	Json dest = Json::object();
	set_req(dest, "buffer", src.buffer);
	set_req(dest, "byteLength", src.byte_length);
	set_opt(dest, "byteOffset", src.byte_offset);
	set_opt(dest, "byteStride", src.byte_stride);
	set_opt(dest, "name", src.name);
	set_opt(dest, "target", src.target);
	return dest;
}

static GLTFBuffer read_buffer(const Json& src) {
	GLTFBuffer dest;
	get_req(dest.byte_length, src, "byteLength");
	get_opt(dest.name, src, "name");
	get_opt(dest.uri, src, "uri");
	return dest;
}

static Json write_buffer(const GLTFBuffer& src) {
	Json dest = Json::object();
	set_req(dest, "byteLength", src.byte_length);
	set_opt(dest, "name", src.name);
	set_opt(dest, "uri", src.uri);
	return dest;
}

// *****************************************************************************
// Miscellaneous
// *****************************************************************************

template <typename T>
static void get_req(T& dest, const Json& src, const char* property) {
	auto iter = src.find(property);
	verify(iter != src.end(), "Missing property '%s'.", property);
	try {
		dest = iter->get<T>();
	} catch(Json::type_error& e) {
		verify_not_reached("Required property '%s' is of the incorrect type (%s).", property, e.what());
	}
}

template <typename T>
static void set_req(Json& dest, const char* property, const T& value) {
	dest.emplace(property, value);
}

template <typename T>
static void get_opt(Opt<T>& dest, const Json& src, const char* property) {
	auto iter = src.find(property);
	if(iter != src.end()) {
		try {
			dest = iter->get<T>();
		} catch(Json::type_error& e) {
			verify_not_reached("Optional property '%s' is of the incorrect type (%s).", property, e.what());
		}
	}
}

template <typename T>
static void set_opt(Json& dest, const char* property, const Opt<T>& value) {
	if(value.has_value()) {
		dest.emplace(property, *value);
	}
}

template <typename T>
static void get_array(std::vector<T>& dest, const Json& src, const char* property) {
	auto iter = src.find(property);
	if(iter != src.end()) {
		dest = iter->get<std::vector<T>>();
	}
}

template <typename T>
static void set_array(Json& dest, const char* property, const std::vector<T>& value) {
	if(!value.empty()) {
		dest.emplace(property, value);
	}
}

template <typename T>
static void get_vec(Opt<T>& dest, const Json& src, const char* property) {
	auto iter = src.find(property);
	if(iter != src.end()) {
		dest.emplace();
		for(s32 i = 0; i < T::length(); i++) {
			(*dest)[i] = iter->at(i).get<f32>();
		}
	}
}

template <typename T>
static void set_vec(Json& dest, const char* property, const Opt<T>& value) {
	if(value.has_value()) {
		Json& array = dest[property];
		array = Json::array();
		for(s32 i = 0; i < T::length(); i++) {
			array.emplace_back((*value)[i]);
		}
	}
}

template <typename T>
static void get_mat(Opt<T>& dest, const Json& src, const char* property) {
	auto iter = src.find(property);
	if(iter != src.end()) {
		dest.emplace();
		for(s32 i = 0; i < T::length(); i++) {
			for(s32 j = 0; j < T::col_type::length(); j++) {
				(*dest)[i][j] = iter->at(i * 4 + j).get<f32>();
			}
		}
	}
}

template <typename T>
static void set_mat(Json& dest, const char* property, const Opt<T>& value) {
	if(value.has_value()) {
		Json& array = dest["property"];
		array = Json::array();
		for(s32 i = 0; i < T::length(); i++) {
			for(s32 j = 0; j < T::col_type::length(); j++) {
				array.emplace_back((*value)[i][j]);
			}
		}
	}
}

template <typename Object, typename ReadFunc>
static void read_object(Object& dest, const Json& src, const char* property, ReadFunc read_func) {
	auto iter = src.find(property);
	verify(iter != src.end(), "Missing property '%s'.", property);
	dest = read_func(*iter);
}

template <typename Object, typename WriteFunc, typename... Args>
static void write_object(Json& dest, const char* property, const Object& src, WriteFunc write_func) {
	dest.emplace(property, write_func(src));
}

template <typename Element, typename ReadFunc, typename... Args>
static void read_array(std::vector<Element>& dest, const Json& src, const char* property, ReadFunc read_func, Args&... args) {
	auto iter = src.find(property);
	if(iter != src.end()) {
		for(const Json& element : *iter) {
			dest.emplace_back(read_func(element, args...));
		}
	}
}

template <typename Element, typename WriteFunc, typename... Args>
static void write_array(Json& dest, const char* property, const std::vector<Element>& src, WriteFunc write_func, Args&... args) {
	if(!src.empty()) {
		Json& array = dest[property];
		array = Json::array();
		for(const Element& element : src) {
			array.emplace_back(write_func(element, args...));
		}
	}
}

static Opt<MeshPrimitiveAttribute> mesh_primitive_attribute_from_string(const char* string) {
	if(strcmp(string, "POSITION") == 0) return POSITION;
	if(strcmp(string, "NORMAL") == 0) return NORMAL;
	if(strcmp(string, "TEXCOORD_0") == 0) return TEXCOORD_0;
	if(strcmp(string, "COLOR_0") == 0) return COLOR_0;
	if(strcmp(string, "JOINTS_0") == 0) return JOINTS_0;
	if(strcmp(string, "WEIGHTS_0") == 0) return WEIGHTS_0;
	return std::nullopt;
}

static const char* accessor_type_to_string(AccessorType type) {
	switch(type) {
		case SCALAR: return "SCALAR";
		case VEC2: return "VEC2";
		case VEC3: return "VEC3";
		case VEC4: return "VEC4";
		case MAT2: return "MAT2";
		case MAT3: return "MAT3";
		case MAT4: return "MAT4";
	}
	return "";
}

static Opt<AccessorType> accessor_type_from_string(const char* string) {
	if(strcmp(string, "SCALAR") == 0) return SCALAR;
	if(strcmp(string, "VEC2") == 0) return VEC2;
	if(strcmp(string, "VEC3") == 0) return VEC3;
	if(strcmp(string, "VEC4") == 0) return VEC4;
	if(strcmp(string, "MAT2") == 0) return MAT2;
	if(strcmp(string, "MAT3") == 0) return MAT3;
	if(strcmp(string, "MAT4") == 0) return MAT4;
	return std::nullopt;
}

static const char* material_alpha_mode_to_string(MaterialAlphaMode alpha_mode) {
	switch(alpha_mode) {
		case OPAQUE: return "OPAQUE";
		case MASK: return "MASK";
		case BLEND: return "BLEND";
	}
	return "";
}

static Opt<MaterialAlphaMode> material_alpha_mode_from_string(const char* string) {
	if(strcmp(string, "OPAQUE") == 0) return OPAQUE;
	if(strcmp(string, "MASK") == 0) return MASK;
	if(strcmp(string, "BLEND") == 0) return BLEND;
	return std::nullopt;
}

static s32 accessor_attribute_size(const Accessor& accessor) {
	return accessor_component_size(accessor.component_type) * accessor_component_count(accessor.type);
}

static s32 accessor_component_size(AccessorComponentType component_type) {
	switch(component_type) {
		case SIGNED_BYTE: return 1;
		case UNSIGNED_BYTE: return 1;
		case SIGNED_SHORT: return 2;
		case UNSIGNED_SHORT: return 2;
		case UNSIGNED_INT: return 4;
		case FLOAT: return 4;
	}
	return 0;
}

static s32 accessor_component_count(AccessorType type) {
	switch(type) {
		case SCALAR: return 1;
		case VEC2: return 2;
		case VEC3: return 3;
		case VEC4: return 4;
		case MAT2: return 4;
		case MAT3: return 9;
		case MAT4: return 16;
	}
	return 0;
}

}
