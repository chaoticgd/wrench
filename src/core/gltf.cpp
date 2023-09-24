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

#include <nlohmann/json.hpp>
using Json = nlohmann::ordered_json;

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

#define FOURCC(string) ((string)[0] | (string)[1] << 8 | (string)[2] << 16 | (string)[3] << 24)

static ModelFile read_gltf(const Json& src);
static Json write_gltf(const ModelFile& src);
static Asset read_asset(const Json& src);
static Json write_asset(const Asset& src);
static Scene read_scene(const Json& src);
static Json write_scene(const Scene& src);
static Node read_node(const Json& src);
static Json write_node(const Node& src);
static Animation read_animation(const Json& src);
static Json write_animation(const Animation& src);
static AnimationChannel read_animation_channel(const Json& src);
static Json write_animation_channel(const AnimationChannel& src);
static AnimationChannelTarget read_animation_channel_target(const Json& src);
static Json write_animation_channel_target(const AnimationChannelTarget& src);
static AnimationSampler read_animation_sampler(const Json& src);
static Json write_animation_sampler(const AnimationSampler& src);
static Material read_material(const Json& src);
static Json write_material(const Material& src);
static MaterialPbrMetallicRoughness read_material_pbr_metallic_roughness(const Json& src);
static Json write_material_pbr_metallic_roughness(const MaterialPbrMetallicRoughness& src);
static TextureInfo read_texture_info(const Json& src);
static Json write_texture_info(const TextureInfo& src);
static Mesh read_mesh(const Json& src);
static Json write_mesh(const Mesh& src);
static MeshPrimitive read_mesh_primitive(const Json& src);
static Json write_mesh_primitive(const MeshPrimitive& src);
static Texture read_texture(const Json& src);
static Json write_texture(const Texture& src);
static Image read_image(const Json& src);
static Json write_image(const Image& src);
static Skin read_skin(const Json& src);
static Json write_skin(const Skin& src);
static Accessor read_accessor(const Json& src);
static Json write_accessor(const Accessor& src);
static BufferView read_buffer_view(const Json& src);
static Json write_buffer_view(const BufferView& src);
static Sampler read_sampler(const Json& src);
static Json write_sampler(const Sampler& src);
static Buffer read_buffer(const Json& src);
static Json write_buffer(const Buffer& src);

static const char* mesh_primitive_attribute_semantic_to_string(MeshPrimitiveAttributeSemantic semantic);
static Opt<MeshPrimitiveAttributeSemantic> mesh_primitive_attribute_semantic_from_string(const char* string);

ModelFile read_glb(::Buffer src) {
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
		gltf = read_gltf(json);
	} catch(Json::exception& e) {
		verify_not_reached("%s", e.what());
	}
	
	// Copy the data in the BIN chunk.
	gltf.bin_data = src.read_bytes(bin_offset, bin_size, "bin chunk");
	
	return gltf;
}

void write_glb(OutBuffer dest, const ModelFile& gltf) {
	Json root = write_gltf(gltf);
	
	std::string json = root.dump();
	
	json.resize(align64(json.size(), 4), ' ');
	size_t padded_binary_size = align64(gltf.bin_data.size(), 4);
	
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
	dest.write_multiple(gltf.bin_data);
	for(size_t i = gltf.bin_data.size(); (i % 4) != 0; i++) {
		dest.write<u8>(0);
	}
}

// A whole bunch of utility functions.
template <typename T>
static void get_req(T& dest, const Json& src, const char* property) {
	auto iter = src.find(property);
	verify(iter != src.end(), "Missing property '%s'.", property);
	dest = iter->get<T>();
}

template <typename T>
static void set_req(Json& dest, const char* property, const T& value) {
	dest.emplace(property, value);
}

template <typename T>
static void get_opt(Opt<T>& dest, const Json& src, const char* property) {
	auto iter = src.find(property);
	if(iter != src.end()) {
		dest = iter->get<T>();
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

template <typename Object, typename WriteFunc>
static void write_object(Json& dest, const char* property, const Object& src, WriteFunc write_func) {
	dest.emplace(property, write_func(src));
}

template <typename Element, typename ReadFunc>
static void read_array(std::vector<Element>& dest, const Json& src, const char* property, ReadFunc read_func) {
	auto iter = src.find(property);
	if(iter != src.end()) {
		for(const Json& element : *iter) {
			dest.emplace_back(read_func(element));
		}
	}
}

template <typename Element, typename WriteFunc>
static void write_array(Json& dest, const char* property, const std::vector<Element>& src, WriteFunc write_func) {
	if(!src.empty()) {
		Json& array = dest[property];
		array = Json::array();
		for(const Element& element : src) {
			array.emplace_back(write_func(element));
		}
	}
}

static ModelFile read_gltf(const Json& src) {
	ModelFile dest;
	read_object(dest.asset, src, "asset", read_asset);
	get_array(dest.extensions_used, src, "extensionsUsed");
	get_array(dest.extensions_required, src, "extensionsRequired");
	get_opt(dest.scene, src, "scene");
	read_array(dest.scenes, src, "scenes", read_scene);
	read_array(dest.nodes, src, "nodes", read_node);
	read_array(dest.animations, src, "animations", read_animation);
	read_array(dest.materials, src, "materials", read_material);
	read_array(dest.meshes, src, "meshes", read_mesh);
	read_array(dest.textures, src, "textures", read_texture);
	read_array(dest.images, src, "images", read_image);
	read_array(dest.skins, src, "skins", read_skin);
	read_array(dest.accessors, src, "accessors", read_accessor);
	read_array(dest.buffer_views, src, "bufferViews", read_buffer_view);
	read_array(dest.samplers, src, "samplers", read_sampler);
	read_array(dest.buffers, src, "buffers", read_buffer);
	return dest;
}

static Json write_gltf(const ModelFile& src) {
	// The order of properties here is the same as for the Blender exporter.
	Json dest = Json::object();
	write_object(dest, "asset", src.asset, write_asset);
	set_array(dest, "extensionsUsed", src.extensions_used);
	set_array(dest, "extensionsRequired", src.extensions_required);
	set_opt(dest, "scene", src.scene);
	write_array(dest, "scenes", src.scenes, write_scene);
	write_array(dest, "nodes", src.nodes, write_node);
	write_array(dest, "animations", src.animations, write_animation);
	write_array(dest, "materials", src.materials, write_material);
	write_array(dest, "meshes", src.meshes, write_mesh);
	write_array(dest, "textures", src.textures, write_texture);
	write_array(dest, "images", src.images, write_image);
	write_array(dest, "skins", src.skins, write_skin);
	write_array(dest, "accessors", src.accessors, write_accessor);
	write_array(dest, "bufferViews", src.buffer_views, write_buffer_view);
	write_array(dest, "samplers", src.samplers, write_sampler);
	write_array(dest, "buffers", src.buffers, write_buffer);
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

static Animation read_animation(const Json& src) {
	Animation dest;
	read_array(dest.channels, src, "channels", read_animation_channel);
	get_opt(dest.name, src, "name");
	read_array(dest.samplers, src, "samplers", read_animation_sampler);
	return dest;
}

static Json write_animation(const Animation& src) {
	Json dest = Json::object();
	write_array(dest, "channels", src.channels, write_animation_channel);
	set_opt(dest, "name", src.name);
	write_array(dest, "samplers", src.samplers, write_animation_sampler);
	return dest;
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

static Material read_material(const Json& src) {
	Material dest;
	get_opt(dest.name, src, "name");
	if(src.contains("pbrMetallicRoughness")) {
		dest.pbr_metallic_roughness.emplace();
		read_object(*dest.pbr_metallic_roughness, src, "pbrMetallicRoughness", read_material_pbr_metallic_roughness);
	}
	return dest;
}

static Json write_material(const Material& src) {
	Json dest = Json::object();
	if(src.pbr_metallic_roughness.has_value()) {
		write_object(dest, "pbrMetallicRoughness", *src.pbr_metallic_roughness, write_material_pbr_metallic_roughness);
	}
	return dest;
}

static MaterialPbrMetallicRoughness read_material_pbr_metallic_roughness(const Json& src) {
	MaterialPbrMetallicRoughness dest;
	if(src.contains("baseColorTexture")) {
		dest.base_color_texture.emplace();
		read_object(*dest.base_color_texture, src, "baseColorTexture", read_texture_info);
	}
	return dest;
}

static Json write_material_pbr_metallic_roughness(const MaterialPbrMetallicRoughness& src) {
	Json dest = Json::object();
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

static Mesh read_mesh(const Json& src) {
	Mesh dest;
	get_opt(dest.name, src, "name");
	read_array(dest.primitives, src, "primitives", read_mesh_primitive);
	return dest;
}

static Json write_mesh(const Mesh& src) {
	Json dest = Json::object();
	set_opt(dest, "name", src.name);
	write_array(dest, "primitives", src.primitives, write_mesh_primitive);
	return dest;
}


static MeshPrimitive read_mesh_primitive(const Json& src) {
	MeshPrimitive dest;
	for(auto& [string, accessor] : src.at("attributes").items()) {
		Opt<MeshPrimitiveAttributeSemantic> semantic = mesh_primitive_attribute_semantic_from_string(string.c_str());
		if(semantic.has_value()) {
			MeshPrimitiveAttribute& attribute = dest.attributes.emplace_back();
			attribute.semantic = *semantic;
			attribute.accessor = accessor;
		}
	}
	get_opt(dest.indices, src, "indices");
	get_opt(dest.material, src, "material");
	get_opt(dest.mode, src, "mode");
	return dest;
}

static Json write_mesh_primitive(const MeshPrimitive& src) {
	Json dest = Json::object();
	Json& attributes = dest["attributes"];
	attributes = Json::object();
	for(auto& [semantic, accessor] : src.attributes) {
		attributes.emplace(mesh_primitive_attribute_semantic_to_string(semantic), accessor);
	}
	set_opt(dest, "indices", src.indices);
	set_opt(dest, "material", src.material);
	set_opt(dest, "mode", src.mode);
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

static Skin read_skin(const Json& src) {
	Skin dest;
	get_opt(dest.inverse_bind_matrices, src, "inverseBindMatrices");
	get_array(dest.joints, src, "joints");
	get_opt(dest.name, src, "name");
	get_opt(dest.skeleton, src, "skeleton");
	return dest;
}

static Json write_skin(const Skin& src) {
	Json dest = Json::object();
	set_opt(dest, "inverseBindMatrices", src.inverse_bind_matrices);
	set_array(dest, "joints", src.joints);
	set_opt(dest, "name", src.name);
	set_opt(dest, "skeleton", src.skeleton);
	return dest;
}

static Accessor read_accessor(const Json& src) {
	Accessor dest;
	get_opt(dest.buffer_view, src, "bufferView");
	get_opt(dest.byte_offset, src, "bufferOffset");
	get_req(dest.component_type, src, "componentType");
	get_req(dest.count, src, "count");
	get_array(dest.max, src, "max");
	get_array(dest.min, src, "min");
	get_opt(dest.name, src, "name");
	get_opt(dest.normalized, src, "normalized");
	get_req(dest.type, src, "type");
	return dest;
}

static Json write_accessor(const Accessor& src) {
	Json dest = Json::object();
	set_opt(dest, "bufferView", src.buffer_view);
	set_opt(dest, "bufferOffset", src.byte_offset);
	set_req(dest, "componentType", src.component_type);
	set_req(dest, "count", src.count);
	set_array(dest, "max", src.max);
	set_array(dest, "min", src.min);
	set_opt(dest, "name", src.name);
	set_opt(dest, "normalized", src.normalized);
	set_req(dest, "type", src.type);
	return dest;
}

static BufferView read_buffer_view(const Json& src) {
	BufferView dest;
	get_req(dest.buffer, src, "buffer");
	get_req(dest.byte_length, src, "byteLength");
	get_opt(dest.byte_offset, src, "byteOffset");
	get_opt(dest.byte_stride, src, "byteStride");
	get_opt(dest.name, src, "name");
	get_opt(dest.target, src, "target");
	return dest;
}

static Json write_buffer_view(const BufferView& src) {
	Json dest = Json::object();
	set_req(dest, "buffer", src.buffer);
	set_req(dest, "byteLength", src.byte_length);
	set_opt(dest, "byteOffset", src.byte_offset);
	set_opt(dest, "byteStride", src.byte_stride);
	set_opt(dest, "name", src.name);
	set_opt(dest, "target", src.target);
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

static Buffer read_buffer(const Json& src) {
	Buffer dest;
	get_req(dest.byte_length, src, "byteLength");
	get_opt(dest.name, src, "name");
	get_opt(dest.uri, src, "uri");
	return dest;
}

static Json write_buffer(const Buffer& src) {
	Json dest = Json::object();
	set_req(dest, "byteLength", src.byte_length);
	set_opt(dest, "name", src.name);
	set_opt(dest, "uri", src.uri);
	return dest;
}

static const char* mesh_primitive_attribute_semantic_to_string(MeshPrimitiveAttributeSemantic semantic) {
	switch(semantic) {
		case POSITION: return "POSITION";
		case NORMAL: return "NORMAL";
		case TANGENT: return "TANGENT";
		case TEXCOORD_0: return "TEXCOORD_0";
		case COLOR_0: return "COLOR_0";
		case JOINTS_0: return "JOINTS_0";
		case WEIGHTS_0: return "WEIGHTS_0";
	}
	return "";
}

static Opt<MeshPrimitiveAttributeSemantic> mesh_primitive_attribute_semantic_from_string(const char* string) {
	if(strcmp(string, "POSITION") == 0) return POSITION;
	if(strcmp(string, "NORMAL") == 0) return NORMAL;
	if(strcmp(string, "TANGENT") == 0) return TANGENT;
	if(strcmp(string, "TEXCOORD_0") == 0) return TEXCOORD_0;
	if(strcmp(string, "COLOR_0") == 0) return COLOR_0;
	if(strcmp(string, "JOINTS_0") == 0) return JOINTS_0;
	if(strcmp(string, "WEIGHTS_0") == 0) return WEIGHTS_0;
	return std::nullopt;
}

}
