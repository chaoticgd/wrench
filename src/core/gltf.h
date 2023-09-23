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

#include <core/buffer.h>
#include <core/mesh.h>

// TODO: Move elsewhere.
struct Error {
	const char* message;
	s32 line;
};
template <typename Value>
struct Result {
	Value value;
	Error* error;
};
inline Error* result_failure(s32 line, const char* format, ...) {
	va_list args;
	va_start(args, format);
	
	static char message[16 * 1024];
	vsnprintf(message, 16 * 1024, format, args);
	
	// Copy it just in case one of the variadic arguments is a pointer to the
	// last error message.
	static char message_copy[16 * 1024];
	strncpy(message_copy, message, sizeof(message_copy));
	message_copy[sizeof(message_copy) - 1] = '\0';
	
	static Error error;
	error.message = message_copy;
	error.line = line;
	
	return &error;
}
#define RESULT_FAILURE(...) {{}, result_failure(__LINE__, __VA_ARGS__)};
#define RESULT_SUCCESS(value) {value, nullptr}
#define RESULT_PROPAGATE(result) {{}, result.error}

namespace GLTF {

struct AccessorSparseIndices {
	s32 buffer_view;
	Opt<s32> byte_offset;
	s32 component_type;
};

struct AcessorSparseValues {
	s32 buffer_view;
	Opt<s32> byte_offset;
};

struct AccessorSparse {
	s32 count;
	AccessorSparseIndices indices;
	AcessorSparseValues values;
};

struct Accessor {
	Opt<s32> buffer_view;
	Opt<s32> byte_offset;
	s32 component_type;
	Opt<bool> normalized;
	s32 count;
	std::string type;
	Opt<s32> max;
	Opt<s32> min;
	Opt<AccessorSparse> sparse;
	Opt<std::string> name;
};

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

struct Animation {
	AnimationChannel channel;
	AnimationSampler sampler;
	Opt<std::string> name;
};

struct Asset {
	Opt<std::string> copyright;
	Opt<std::string> generator;
	std::string version;
	Opt<std::string> min_version;
};

struct Buffer {
	Opt<std::string> uri;
	s32 byte_length;
	Opt<std::string> name;
};

struct BufferView {
	s32 buffer;
	Opt<s32> byte_offset;
	s32 byte_length;
	Opt<s32> byte_stride;
	Opt<s32> target;
	Opt<std::string> name;
};

struct Image {
	Opt<std::string> uri;
	Opt<std::string> mime_type;
	Opt<std::string> buffer_view;
	Opt<std::string> name;
};

struct Material {
	Opt<std::string> name;
	// unimplemented: pbrMetallicRoughness
	// unimplemented: normalTexture
	// unimplemented: occlusionTexture
	// unimplemented: emissiveTexture
	// unimplemented: emissiveFactor
	// unimplemented: alphaMode
	// unimplemented: alphaCutoff
	// unimplemented: doubleSided
};

struct MeshPrimitive {
	std::vector<std::pair<std::string, s32>> attributes;
	Opt<s32> indices;
	Opt<s32> material;
	Opt<s32> mode;
	// unimplemented: targets
};

struct Mesh {
	std::vector<MeshPrimitive> primitives;
	// unimplemented: weights
	Opt<std::string> name;
};

struct Node {
	// unimplemented: camera
	Opt<std::vector<s32>> children;
	Opt<s32> skin;
	glm::mat4 matrix;
	s32 mesh;
	Opt<glm::vec4> rotation;
	Opt<glm::vec3> scale;
	Opt<glm::vec3> translation;
	// unimplemented: weights
	Opt<std::string> name;
};

struct Sampler {
	Opt<s32> mag_filter;
	Opt<s32> min_filter;
	Opt<s32> wrap_s;
	Opt<s32> wrap_t;
	Opt<std::string> name;
};

struct Scene {
	Opt<std::vector<s32>> nodes;
	Opt<std::string> name;
};

struct Skin {
	Opt<s32> inverse_bind_matrices;
	Opt<s32> skeleton;
	std::vector<s32> joints;
	Opt<std::string> name;
};

struct Texture {
	Opt<s32> sampler;
	Opt<s32> source;
	Opt<std::string> name;
};

struct ModelFile {
	Opt<std::vector<std::string>> extensions_used;
	Opt<std::vector<std::string>> extensions_required;
	std::vector<Accessor> accessors;
	std::vector<Animation> animations;
	Asset asset;
	std::vector<Buffer> buffers;
	std::vector<BufferView> buffer_views;
	// unimplemented: cameras
	std::vector<Image> images;
	std::vector<Material> materials;
	std::vector<Mesh> meshes;
	std::vector<Node> nodes;
	std::vector<Sampler> samplers;
	std::vector<Scene> scenes;
	std::vector<Skin> skins;
	std::vector<Texture> textures;
	
	std::vector<u8> bin_data;
};

Result<ModelFile> read_glb(::Buffer src);
void write_glb(OutBuffer dest, const ModelFile& gltf);

}
