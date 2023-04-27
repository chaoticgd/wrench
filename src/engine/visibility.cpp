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

#include "visibility.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/include/glad/glad.h>

#define VIS_DEBUG(...) __VA_ARGS__

#define VIS_RENDER_SIZE 128
#define VIS_MAX_MEMORY 4*1024*1024

struct VisVertex {
	glm::vec3 pos;
	u16 id;
};

struct VisAABB {
	glm::vec3 min;
	glm::vec3 max;
};

struct CPUVisMesh {
	std::vector<VisVertex> vertices;
	std::vector<u8> indices;
	VisAABB aabb;
};

struct GPUVisMesh {
	GLuint vertex_buffer;
	GLuint index_buffer;
	s32 vertex_count;
	VisAABB bounding_box;
};

struct GPUHandles {
	GLuint frame_buffer;
	GLuint id_buffer;
	GLuint depth_buffer;
	GLuint program;
	GLuint vertex_array_object;
	std::vector<GPUVisMesh> vis_meshes;
};

struct VisMemoryUsage {
	s32 mask_size_bits;
	s32 mask_size_bytes;
	s32 required_memory_bytes;
};

static VisMemoryUsage calculate_memory_usage(const VisInput& input);
static void startup_opengl(GPUHandles& gpu);
static std::vector<CPUVisMesh> build_vis_meshes(const VisInput& input);
static std::vector<GPUVisMesh> upload_vis_meshes(const std::vector<CPUVisMesh>& cpu_meshes);
static void compute_vis_sample(u8 dest[128], const glm::vec3& sample_point, const GPUHandles& gpu);
static void compress_vis_masks(std::vector<u8>& masks_dest, std::vector<s32>& mapping_dest, const std::vector<u8>& masks_src, size_t mask_size_bits, size_t stride);
static void shutdown_opengl(GPUHandles& gpu);

#define GET_BIT(val_dest, mask_src, index, size) \
	{ \
		s32 byte_index = index >> 3; \
		s32 bit_index = index & 7; \
		VIS_DEBUG(verify(byte_index > -1 || byte_index < size, "Tried to get a bit out of range.")); \
		val_dest = ((mask_src)[byte_index] >> bit_index) & 1; \
	}

#define SET_BIT(mask_dest, index, size) \
	{ \
		s32 byte_index = index >> 3; \
		s32 bit_index = index & 7; \
		VIS_DEBUG(verify(byte_index > -1 || byte_index < size, "Tried to set a bit out of range.")); \
		(mask_dest)[byte_index] |= 1 << bit_index; \
	}

static const glm::vec3 OCTANT_SAMPLES[] = {
	{0.5f, 0.5f, 0.5f}
};

static const char* vis_vertex_shader = R"(
	#version 120
	
	attribute vec3 pos;
	attribute uint id;
	
	varying uint id_frag;
	
	void main() {
		gl_Position = inst_matrix * vec4(position, 1);
		id_frag = id;
	}
)";

static const char* vis_fragment_shader = R"(
	#version 120
	
	varying uint id_frag;
	
	void main() {
		gl_FragColor = id_frag;
	}
)";

VisOutput compute_level_visibility(const VisInput& input) {
	puts("**** Entered visibility routine! ****");
	
	// Calculate required memory and check against a maximum.
	VisMemoryUsage mem_usage = calculate_memory_usage(input);
	
	// Allocate and zero the memory.
	std::vector<u8> uncompressed_vis_masks(mem_usage.required_memory_bytes, 0);
	
	// Do the OpenGL dance.
	GPUHandles gpu;
	startup_opengl(gpu);
	
	VisOutput output;
	
	puts("Building vis meshes...");
	
	// Batch the meshes together and upload them to the GPU.
	std::vector<CPUVisMesh> cpu_meshes = build_vis_meshes(input);
	gpu.vis_meshes = upload_vis_meshes(cpu_meshes);
	cpu_meshes.clear();
	
	puts("Computing visibility...");
	
	// Determine which objects are visible and populate the visibility mask for
	// each octant.
	for(size_t i = 0; i < input.octants.size(); i++) {
		const OcclusionVector& src = input.octants[i];
		printf("%d,%d,%d%c", src.x, src.y, src.z, (i % 4 == 3) ? '\n' : ' ');
		OcclusionOctant& dest = output.octants.emplace_back();
		dest.x = src.x;
		dest.y = src.y;
		dest.z = src.z;
		for(const glm::vec3& sample : OCTANT_SAMPLES) {
			glm::vec3 sample_point = {
				((f32) src.x + sample.x) * input.octant_size_x,
				((f32) src.y + sample.y) * input.octant_size_y,
				((f32) src.z + sample.z) * input.octant_size_z
			};
			if(glm::distance(sample_point, glm::vec3(308.59f, 295.43f, 56.49f)) < 8.f)
			compute_vis_sample(dest.visibility, sample_point, gpu);
		}
	}
	printf("\n");
	
	puts("Compressing vis data...");
	
	// Merge bits based on how well they can be predicted by other bits.
	std::vector<u8> compressed_vis_masks;
	std::vector<s32> compressed_mappings;
	compress_vis_masks(compressed_vis_masks, compressed_mappings, uncompressed_vis_masks, mem_usage.mask_size_bits, mem_usage.mask_size_bytes);
	verify_fatal(compressed_vis_masks.size() == input.octants.size() * 128);
	
	// Separate out the mappings into separate lists for each type of object.
	s32 next_mapping = 0;
	for(s32 i = 0; i < VIS_OBJECT_TYPE_COUNT; i++) {
		for(size_t j = 0; j < input.instances[i].size(); j++) {
			output.mappings[i][j] = compressed_mappings.at(next_mapping++);
		}
	}
	
	// Copy the compressed visibility masks to the output.
	for(size_t i = 0; i < input.octants.size(); i++) {
		const OcclusionVector& src = input.octants[i];
		OcclusionOctant& dest = output.octants.emplace_back();
		dest.x = src.x;
		dest.y = src.y;
		dest.z = src.z;
		memcpy(dest.visibility, &compressed_vis_masks[i * 128], 128);
	}
	
	shutdown_opengl(gpu);
	
	puts("**** Exited visibility routine! ****");
	
	return output;
}

static VisMemoryUsage calculate_memory_usage(const VisInput& input) {
	VisMemoryUsage mem_usage;
	
	mem_usage.mask_size_bits = 0;
	for(s32 i = 0; i < VIS_OBJECT_TYPE_COUNT; i++) {
		mem_usage.mask_size_bits += (s32) input.instances[i].size();
	}
	
	mem_usage.mask_size_bytes = align32(mem_usage.mask_size_bits, 8) / 8;
	
	size_t required_memory_bits = 0;
	size_t uncompressed_vis_mask_base[VIS_OBJECT_TYPE_COUNT];
	for(s32 i = 0; i < VIS_OBJECT_TYPE_COUNT; i++) {
		uncompressed_vis_mask_base[i] = required_memory_bits;
		required_memory_bits += input.octants.size() * align64(input.instances[i].size(), 8);
	}
	
	mem_usage.required_memory_bytes = (s32) (required_memory_bits / 8);
	verify(mem_usage.required_memory_bytes < VIS_MAX_MEMORY, "Visibility computation would use too much memory. Something probably went wrong.");
	
	printf("Uncompressed size: %.2f meg\n", (f32) mem_usage.required_memory_bytes / (1024.f * 1024.f));
	
	return mem_usage;
}

static void startup_opengl(GPUHandles& gpu) {
	verify(glfwInit(), "Failed to load OpenGL.");
	
	// Allocate framebuffer textures.
	glGenTextures(1, &gpu.id_buffer);
	glBindTexture(GL_TEXTURE_2D, gpu.id_buffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_R16UI, VIS_RENDER_SIZE, VIS_RENDER_SIZE, 0, GL_RED, GL_UNSIGNED_SHORT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	
	glGenTextures(1, &gpu.depth_buffer);
	glBindTexture(GL_TEXTURE_2D, gpu.depth_buffer);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, VIS_RENDER_SIZE, VIS_RENDER_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	
	glGenFramebuffers(1, &gpu.frame_buffer);
	glBindFramebuffer(GL_FRAMEBUFFER, gpu.frame_buffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gpu.id_buffer, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gpu.depth_buffer, 0);
	
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	
	// Compile shaders.
	GLint result;
	int log_length;
	
	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vis_vertex_shader, NULL);
	glCompileShader(vertex_shader);
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &result);
	glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &log_length);
	if(log_length > 0) {
		std::string message;
		message.resize(log_length);
		glGetShaderInfoLog(vertex_shader, log_length, NULL, message.data());
		verify_not_reached("Failed to compile vertex shader!\n%s", message.c_str());
	}
	
	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &vis_fragment_shader, NULL);
	glCompileShader(fragment_shader);
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &result);
	glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &log_length);
	if(log_length > 0) {
		std::string message;
		message.resize(log_length);
		glGetShaderInfoLog(fragment_shader, log_length, NULL, message.data());
		verify_not_reached("Failed to compile fragment shader!\n%s", message.c_str());
	}
	
	// Link shaders.
	gpu.program = glCreateProgram();
	glAttachShader(gpu.program, vertex_shader);
	glAttachShader(gpu.program, fragment_shader);
	
	//_before(gpu.program);
	glLinkProgram(gpu.program);
	//_after(gpu.program);

	glGetProgramiv(gpu.program, GL_LINK_STATUS, &result);
	glGetProgramiv(gpu.program, GL_INFO_LOG_LENGTH, &log_length);
	if(log_length > 0) {
		std::string message;
		message.resize(log_length);
		glGetProgramInfoLog(gpu.program, log_length, NULL, message.data());
		verify_not_reached("Failed to link shaders!\n%s", message.c_str());
	}

	glDetachShader(gpu.program, vertex_shader);
	glDetachShader(gpu.program, fragment_shader);
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
	
	// Setup VAO.
	glGenVertexArrays(1, &gpu.vertex_array_object);
	glBindVertexArray(gpu.vertex_array_object);
	
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VisVertex), (void*) offsetof(VisVertex, pos));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 1, GL_UNSIGNED_SHORT, GL_FALSE, sizeof(VisVertex), (void*) offsetof(VisVertex, id));
	
	// Setup viewport.
	glClearColor(0, 0, 0, 1);
	glViewport(0, 0, VIS_RENDER_SIZE, VIS_RENDER_SIZE);
}

static std::vector<CPUVisMesh> build_vis_meshes(const VisInput& input) {
	std::vector<CPUVisMesh> vis_meshes;
	CPUVisMesh& vismesh = vis_meshes.emplace_back();
	u16 occlusion_id = 0;
	for(s32 i = 0; i < VIS_OBJECT_TYPE_COUNT; i++) {
		for(size_t j = 0; j < input.instances[i].size(); j++) {
			const VisInstance& instance = input.instances[i][j];
			const Mesh& mesh = *input.meshes.at(instance.mesh);
			size_t vertex_base = mesh.vertices.size();
			for(const Vertex& src : mesh.vertices) {
				VisVertex& dest = vismesh.vertices.emplace_back();
				dest.pos = src.pos;
				dest.id = occlusion_id;
			}
			for(const SubMesh& submesh : mesh.submeshes) {
				for(const Face& face : submesh.faces) {
					vismesh.indices.emplace_back(vertex_base + face.v0);
					vismesh.indices.emplace_back(vertex_base + face.v1);
					vismesh.indices.emplace_back(vertex_base + face.v2);
					if(face.is_quad()) {
						vismesh.indices.emplace_back(vertex_base + face.v2);
						vismesh.indices.emplace_back(vertex_base + face.v3);
						vismesh.indices.emplace_back(vertex_base + face.v0);
					}
				}
			}
			occlusion_id++;
		}
	}
	return vis_meshes;
}

static std::vector<GPUVisMesh> upload_vis_meshes(const std::vector<CPUVisMesh>& cpu_meshes) {
	std::vector<GPUVisMesh> gpu_meshes;
	gpu_meshes.reserve(cpu_meshes.size());
	for(const CPUVisMesh& src : cpu_meshes) {
		GPUVisMesh& dest = gpu_meshes.emplace_back();
		
		glGenBuffers(1, &dest.vertex_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, dest.vertex_buffer);
		glBufferData(GL_ARRAY_BUFFER, src.vertices.size() * sizeof(VisVertex), src.vertices.data(), GL_STATIC_DRAW);
		
		glGenBuffers(1, &dest.index_buffer);
		glBindBuffer(GL_ARRAY_BUFFER, dest.index_buffer);
		glBufferData(GL_ARRAY_BUFFER, src.indices.size(), src.indices.data(), GL_STATIC_DRAW);
		
		dest.vertex_count = (s32) src.vertices.size();
	}
	return gpu_meshes;
}

static void compute_vis_sample(u8 dest[128], const glm::vec3& sample_point, const GPUHandles& gpu) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

static void compress_vis_masks(std::vector<u8>& masks_dest, std::vector<s32>& mapping_dest, const std::vector<u8>& masks_src, size_t mask_size_bits, size_t stride) {
	s32 compressed_mask_size_bits = mask_size_bits;
	std::vector<s32> bit_mappings(mask_size_bits, -1);
	
	verify_fatal(masks_src.size() % stride == 0);
	
	// Stupid algorithm I know.
	for(s32 i = compressed_mask_size_bits; i > 1024; i--) {
		s32 min_error = INT32_MAX;
		size_t lhs_least_error = 0;
		size_t rhs_least_error = 0;
		for(size_t lhs = 0; lhs < masks_src.size(); lhs += stride) {
			for(size_t rhs = lhs + 1; rhs < masks_src.size(); rhs += stride) {
				s32 error = 0;
				for(s32 bit = 0; bit < mask_size_bits; bit++) {
					if(bit_mappings[bit] == -1) {
						u8 lhs_bit;
						GET_BIT(lhs_bit, &masks_src[lhs], bit, stride);
						u8 rhs_bit;
						GET_BIT(rhs_bit, &masks_src[rhs], bit, stride);
						if((lhs_bit ^ rhs_bit) == 1) error++;
					}
				}
				if(error < min_error) {
					lhs_least_error = lhs;
					rhs_least_error = rhs;
					min_error = error;
				}
			}
		}
		
		bit_mappings[rhs_least_error] = lhs_least_error;
	}
	
	// Write output and build mapping...
}

static void shutdown_opengl(GPUHandles& gpu) {
	glDeleteFramebuffers(1, &gpu.frame_buffer);
	glDeleteTextures(1, &gpu.id_buffer);
	glDeleteTextures(1, &gpu.depth_buffer);
	
	for(GPUVisMesh& mesh : gpu.vis_meshes) {
		glDeleteBuffers(1, &mesh.vertex_buffer);
		glDeleteBuffers(1, &mesh.index_buffer);
	}
	
	glfwTerminate();
}
