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

namespace GLTF {

struct Asset {
	Opt<std::string> copyright;
	Opt<std::string> generator;
	std::string version;
	Opt<std::string> min_version;
};

struct Scene {
	std::vector<s32> nodes;
	Opt<std::string> name;
};

struct Node {
	// unimplemented: camera
	std::vector<s32> children;
	Opt<s32> skin;
	Opt<glm::mat4> matrix;
	Opt<s32> mesh;
	Opt<glm::vec4> rotation;
	Opt<glm::vec3> scale;
	Opt<glm::vec3> translation;
	// unimplemented: weights
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
	std::vector<AnimationChannel> channels;
	std::vector<AnimationSampler> samplers;
	Opt<std::string> name;
};

struct TextureInfo {
	s32 index;
	Opt<s32> tex_coord;
};

struct MaterialPbrMetallicRoughness {
	// unimplemented: baseColorFactor
	Opt<TextureInfo> base_color_texture;
	// unimplemented: metallicFactor
	// unimplemented: roughnessFactor
	// unimplemented: metallicRoughnessTexture
};

struct Material {
	Opt<std::string> name;
	Opt<MaterialPbrMetallicRoughness> pbr_metallic_roughness;
	// unimplemented: normalTexture
	// unimplemented: occlusionTexture
	// unimplemented: emissiveTexture
	// unimplemented: emissiveFactor
	// unimplemented: alphaMode
	// unimplemented: alphaCutoff
	// unimplemented: doubleSided
};

enum MeshPrimitiveAttributeSemantic {
	POSITION,
	NORMAL,
	TANGENT,
	TEXCOORD_0,
	COLOR_0,
	JOINTS_0,
	WEIGHTS_0
};

struct MeshPrimitiveAttribute {
	MeshPrimitiveAttributeSemantic semantic;
	s32 accessor;
};

struct MeshPrimitive {
	std::vector<MeshPrimitiveAttribute> attributes;
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

struct Texture {
	Opt<s32> sampler;
	Opt<s32> source;
	Opt<std::string> name;
};

struct Image {
	Opt<std::string> uri;
	Opt<std::string> mime_type;
	Opt<std::string> buffer_view;
	Opt<std::string> name;
};

struct Skin {
	Opt<s32> inverse_bind_matrices;
	Opt<s32> skeleton;
	std::vector<s32> joints;
	Opt<std::string> name;
};

enum AccessorComponentType {
	SIGNED_BYTE = 5120,
	UNSIGNED_BYTE = 5121,
	SIGNED_SHORT = 5122,
	UNSIGNED_SHORT = 5123,
	UNSIGNED_INT= 5125,
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

struct Sampler {
	Opt<s32> mag_filter;
	Opt<s32> min_filter;
	Opt<s32> wrap_s;
	Opt<s32> wrap_t;
	Opt<std::string> name;
};

struct ModelFile {
	Asset asset;
	std::vector<std::string> extensions_used;
	std::vector<std::string> extensions_required;
	// unimplemented: extensions
	// unimplemented: extras
	Opt<s32> scene;
	std::vector<Scene> scenes;
	std::vector<Node> nodes;
	// unimplemented: cameras
	std::vector<Animation> animations;
	std::vector<Material> materials;
	std::vector<Mesh> meshes;
	std::vector<Texture> textures;
	std::vector<Image> images;
	std::vector<Skin> skins;
	std::vector<Accessor> accessors;
	std::vector<Sampler> samplers;
};

ModelFile read_glb(Buffer src);
void write_glb(OutBuffer dest, const ModelFile& gltf);

}
