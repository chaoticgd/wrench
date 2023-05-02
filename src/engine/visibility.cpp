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

#include <map>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/include/glad/glad.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/common.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <core/timer.h>

#define VIS_DEBUG_MISC(...) __VA_ARGS__
#define VIS_DEBUG_DUMP(...) //__VA_ARGS__
#define GL_CALL(...) \
	{ \
		__VA_ARGS__; \
		GLenum error = glGetError(); \
		verify(error == GL_NO_ERROR, "GL Error %x\n", error); \
	}
#define VIS_DEBUG_RENDERDOC

#define VIS_RENDER_SIZE 128

#ifdef VIS_DEBUG_RENDERDOC
#include "renderdoc_app.h"
#include <dlfcn.h>
#endif

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
	std::vector<u32> indices;
	VisAABB aabb;
};

struct GPUVisMesh {
	GLuint vertex_array_object;
	GLuint vertex_buffer;
	s32 vertex_count;
	GLuint index_buffer;
	s32 index_count;
	VisAABB bounding_box;
};

struct GPUHandles {
	GLFWwindow* dummy;
	GLuint frame_buffer;
	GLuint id_buffer;
	GLuint depth_buffer;
	GLuint program;
	GLuint matrix_uniform;
	std::vector<GPUVisMesh> vis_meshes;
#ifdef VIS_DEBUG_RENDERDOC
	RENDERDOC_API_1_1_2* renderdoc = nullptr;
	bool capturing = false;
#endif
};

static void startup_opengl(GPUHandles& gpu);
static std::vector<CPUVisMesh> build_vis_meshes(const VisInput& input);
static std::vector<GPUVisMesh> upload_vis_meshes(const std::vector<CPUVisMesh>& cpu_meshes);
static void compute_vis_sample(u8* mask_dest, s32 mask_size_bytes, const glm::vec3& sample_point, GPUHandles& gpu);
static void compress_objects(std::vector<u8>& masks_dest, std::vector<s32>& mapping_dest, const std::vector<u8>& octant_masks_of_object_bits, s32 octant_count, s32 instance_count, s32 stride);
static void compress_octants(std::vector<u8>& compressed_vis_masks, s32 mask_count, s32 memory_budget_for_masks);
static void shutdown_opengl(GPUHandles& gpu);

#define GET_BIT(val_dest, mask_src, index, size) \
	{ \
		s32 array_index = (index) >> 3; \
		s32 bit_index = (index) & 7; \
		VIS_DEBUG_MISC(verify(array_index > -1 || array_index < size, "Tried to get a bit out of range.")); \
		val_dest = ((mask_src)[array_index] >> bit_index) & 1; \
	}

#define SET_BIT(mask_dest, index, val_src, size) \
	{ \
		s32 array_index = (index) >> 3; \
		s32 bit_index = (index) & 7; \
		VIS_DEBUG_MISC(verify(array_index > -1 || array_index < size, "Tried to set a bit out of range.")); \
		(mask_dest)[array_index] |= val_src << bit_index; \
	}

static const char* vis_vertex_shader = R"(
	#version 330 core
	
	uniform mat4 matrix;
	in vec3 pos;
	in uint id_in;
	flat out uint id_mid;
	
	void main() {
		gl_Position = matrix * vec4(pos, 1);
		id_mid = id_in;
	}
)";

static const char* vis_fragment_shader = R"(
	#version 330 core
	
	flat in uint id_mid;
	out uint id_out;
	
	void main() {
		id_out = id_mid;
	}
)";

static const glm::mat4 RATCHET_TO_OPENGL_MATRIX = {
	0,  0, 1, 0,
	1,  0, 0, 0,
	0, -1, 0, 0,
	0,  0, 0, 1
};

VisOutput compute_level_visibility(const VisInput& input, s32 memory_budget_for_masks) {
	puts("**** Entered visibility routine! ****");
	
	// Calculate mask size.
	s32 instance_count = 0;
	for(s32 i = 0; i < VIS_OBJECT_TYPE_COUNT; i++) {
		instance_count += (s32) input.instances[i].size();
	}
	
	s32 mask_size_bytes = align32(instance_count, 64) / 8;
	
	std::vector<u8> sample_masks_of_object_bits;
	std::map<OcclusionVector, s32> sample_lookup;
	
	std::vector<u8> octant_masks_of_object_bits(input.octants.size() * mask_size_bytes, 0);
	verify_fatal(((s64) octant_masks_of_object_bits.data()) % 8 == 0);
	
	// Do the OpenGL dance.
	GPUHandles gpu;
	startup_opengl(gpu);
	
	puts("Building vis meshes...");
	
	// Batch the meshes together and upload them to the GPU.
	std::vector<CPUVisMesh> cpu_meshes = build_vis_meshes(input);
	gpu.vis_meshes = upload_vis_meshes(cpu_meshes);
	cpu_meshes.clear();
	
	{
		start_timer("Computing visibility");
		defer([&]() { stop_timer(); });
		
		// Determine which objects are visible and populate the visibility mask for
		// each octant.
		for(size_t i = 0; i < input.octants.size(); i++) {
			const OcclusionVector& src = input.octants[i];
			printf("%3d,%3d,%3d%s", src.x, src.y, src.z, (i % 4 == 3) ? "\n" : "  ");
			for(s32 corner = 0; corner < 8; corner++) {
				s32 x_ofs = corner & 1;
				s32 y_ofs = corner & 2;
				s32 z_ofs = corner & 4;
				OcclusionVector sample_vec = {
					src.x + x_ofs,
					src.y + y_ofs,
					src.z + z_ofs
				};
				s32 sample_ofs;
				auto sample = sample_lookup.find(sample_vec);
				if(sample == sample_lookup.end()) {
					sample_ofs = (s32) sample_masks_of_object_bits.size();
					sample_masks_of_object_bits.resize(sample_masks_of_object_bits.size() + mask_size_bytes);
					verify_fatal(((s64) sample_masks_of_object_bits.data()) % 8 == 0);
					glm::vec3 sample_point = {
						sample_vec.x * input.octant_size_x,
						sample_vec.y * input.octant_size_y,
						sample_vec.z * input.octant_size_z
					};
					compute_vis_sample(&sample_masks_of_object_bits[sample_ofs], mask_size_bytes, sample_point, gpu);
					sample_lookup[sample_vec] = sample_ofs;
				} else {
					sample_ofs = sample->second;
				}
				for(s32 ofs = 0; ofs < mask_size_bytes; ofs += 8) {
					*(u64*) &octant_masks_of_object_bits[i * mask_size_bytes + ofs]
						|= *(u64*) &sample_masks_of_object_bits[sample_ofs + ofs];
				}
			}
		}
		printf("\n");
	}
	
	std::vector<u8> compressed_vis_masks;
	std::vector<s32> compressed_mappings;
	{
		start_timer("Compressing vis data");
		defer([&]() { stop_timer(); });
		
		// Merge bits based on how well they can be predicted by other bits.
		compress_objects(compressed_vis_masks, compressed_mappings, octant_masks_of_object_bits, (s32) input.octants.size(), instance_count, mask_size_bytes);
		if(memory_budget_for_masks > -1) {
			compress_octants(compressed_vis_masks, (s32) input.octants.size(), memory_budget_for_masks);
		}
		verify_fatal(compressed_vis_masks.size() == input.octants.size() * 128);
		verify_fatal(compressed_mappings.size() == instance_count);
	}
	
	VisOutput output;
	
	// Separate out the mappings into separate lists for each type of object.
	s32 next_mapping = 0;
	for(s32 i = 0; i < VIS_OBJECT_TYPE_COUNT; i++) {
		output.mappings[i].reserve(input.instances[i].size());
		for(size_t j = 0; j < input.instances[i].size(); j++) {
			output.mappings[i].emplace_back(compressed_mappings[next_mapping++]);
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

static void startup_opengl(GPUHandles& gpu) {
#ifdef VIS_DEBUG_RENDERDOC
	if(void* mod = dlopen("librenderdoc.so", RTLD_NOW | RTLD_NOLOAD)) {
		pRENDERDOC_GetAPI RENDERDOC_GetAPI = (pRENDERDOC_GetAPI) dlsym(mod, "RENDERDOC_GetAPI");
		int ret = RENDERDOC_GetAPI(eRENDERDOC_API_Version_1_1_2, (void**) &gpu.renderdoc);
		verify_fatal(ret == 1);
	}
#endif
	
	verify(glfwInit(), "Failed to load OpenGL (glfwInit).");
	
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

	gpu.dummy = glfwCreateWindow(1, 1, "vis", NULL, NULL);
	verify(gpu.dummy, "Failed to load OpenGL (glfwCreateWindow).");
	glfwMakeContextCurrent(gpu.dummy);
	
	verify(gladLoadGLLoader((GLADloadproc) glfwGetProcAddress), "Failed to load OpenGL (GLADloadproc).");
	
	// Allocate framebuffer textures.
	GL_CALL(glGenTextures(1, &gpu.id_buffer));
	GL_CALL(glBindTexture(GL_TEXTURE_2D, gpu.id_buffer));
	GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_R16UI, VIS_RENDER_SIZE, VIS_RENDER_SIZE, 0, GL_RED_INTEGER, GL_UNSIGNED_SHORT, 0));
	
	GL_CALL(glGenTextures(1, &gpu.depth_buffer));
	GL_CALL(glBindTexture(GL_TEXTURE_2D, gpu.depth_buffer));
	GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, VIS_RENDER_SIZE, VIS_RENDER_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0));
	
	GL_CALL(glGenFramebuffers(1, &gpu.frame_buffer));
	GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, gpu.frame_buffer));
	GL_CALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gpu.id_buffer, 0));
	GL_CALL(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gpu.depth_buffer, 0));
	
	GL_CALL(glEnable(GL_DEPTH_TEST));
	GL_CALL(glDepthFunc(GL_LESS));
	
	// Compile shaders.
	GLint result;
	int log_length;
	
	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	GL_CALL(glShaderSource(vertex_shader, 1, &vis_vertex_shader, NULL));
	GL_CALL(glCompileShader(vertex_shader));
	GL_CALL(glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &result));
	GL_CALL(glGetShaderiv(vertex_shader, GL_INFO_LOG_LENGTH, &log_length));
	if(log_length > 0) {
		std::string message;
		message.resize(log_length);
		GL_CALL(glGetShaderInfoLog(vertex_shader, log_length, NULL, message.data()));
		verify_not_reached("Failed to compile vertex shader!\n%s", message.c_str());
	}
	
	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	GL_CALL(glShaderSource(fragment_shader, 1, &vis_fragment_shader, NULL));
	GL_CALL(glCompileShader(fragment_shader));
	GL_CALL(glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &result));
	GL_CALL(glGetShaderiv(fragment_shader, GL_INFO_LOG_LENGTH, &log_length));
	if(log_length > 0) {
		std::string message;
		message.resize(log_length);
		GL_CALL(glGetShaderInfoLog(fragment_shader, log_length, NULL, message.data()));
		verify_not_reached("Failed to compile fragment shader!\n%s", message.c_str());
	}
	
	// Link shaders.
	gpu.program = glCreateProgram();
	glAttachShader(gpu.program, vertex_shader);
	glAttachShader(gpu.program, fragment_shader);
	
	glBindAttribLocation(gpu.program, 0, "pos");
	glBindAttribLocation(gpu.program, 1, "id_in");
	glLinkProgram(gpu.program);
	gpu.matrix_uniform = glGetUniformLocation(gpu.program, "matrix");
	glUseProgram(gpu.program);

	glGetProgramiv(gpu.program, GL_LINK_STATUS, &result);
	glGetProgramiv(gpu.program, GL_INFO_LOG_LENGTH, &log_length);
	if(log_length > 0) {
		std::string message;
		message.resize(log_length);
		glGetProgramInfoLog(gpu.program, log_length, NULL, message.data());
		verify_not_reached("Failed to link shaders!\n%s", message.c_str());
	}

	GL_CALL(glDetachShader(gpu.program, vertex_shader));
	GL_CALL(glDetachShader(gpu.program, fragment_shader));
	GL_CALL(glDeleteShader(vertex_shader));
	GL_CALL(glDeleteShader(fragment_shader));
	
	// Setup viewport.
	GL_CALL(glClearColor(0, 0, 0, 1));
	GL_CALL(glViewport(0, 0, VIS_RENDER_SIZE, VIS_RENDER_SIZE));
}

static std::vector<CPUVisMesh> build_vis_meshes(const VisInput& input) {
	std::vector<CPUVisMesh> vis_meshes;
	CPUVisMesh& vismesh = vis_meshes.emplace_back();
	u16 occlusion_id = 1;
	for(s32 i = 0; i < VIS_OBJECT_TYPE_COUNT; i++) {
		for(size_t j = 0; j < input.instances[i].size(); j++) {
			const VisInstance& instance = input.instances[i][j];
			const Mesh& mesh = *input.meshes.at(instance.mesh);
			size_t vertex_base = vismesh.vertices.size();
			for(const Vertex& src : mesh.vertices) {
				VisVertex& dest = vismesh.vertices.emplace_back();
				dest.pos = glm::vec3(instance.matrix * glm::vec4(src.pos, 1.f));
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
			verify(occlusion_id != 0xffff, "Too many objects to compute visibility!");
			occlusion_id++;
		}
	}
	verify(vismesh.vertices.size() < UINT32_MAX, "Too many vertices to compute visiblity!");
	return vis_meshes;
}

static std::vector<GPUVisMesh> upload_vis_meshes(const std::vector<CPUVisMesh>& cpu_meshes) {
	std::vector<GPUVisMesh> gpu_meshes;
	gpu_meshes.reserve(cpu_meshes.size());
	for(const CPUVisMesh& src : cpu_meshes) {
		GPUVisMesh& dest = gpu_meshes.emplace_back();
		
		// Setup vertex array object.
		GL_CALL(glGenVertexArrays(1, &dest.vertex_array_object));
		GL_CALL(glBindVertexArray(dest.vertex_array_object));
		
		// Allocate buffers.
		GL_CALL(glGenBuffers(1, &dest.vertex_buffer));
		GL_CALL(glBindBuffer(GL_ARRAY_BUFFER, dest.vertex_buffer));
		GL_CALL(glBufferData(GL_ARRAY_BUFFER, src.vertices.size() * sizeof(VisVertex), src.vertices.data(), GL_STATIC_DRAW));
		dest.vertex_count = (s32) src.vertices.size();
		
		GL_CALL(glGenBuffers(1, &dest.index_buffer));
		GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dest.index_buffer));
		GL_CALL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, src.indices.size() * 4, src.indices.data(), GL_STATIC_DRAW));
		dest.index_count = (s32) src.indices.size();
		
		// Declare vertex buffer layout.
		GL_CALL(glEnableVertexAttribArray(0));
		GL_CALL(glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VisVertex), (void*) offsetof(VisVertex, pos)));
		GL_CALL(glEnableVertexAttribArray(1));
		GL_CALL(glVertexAttribIPointer(1, 1, GL_UNSIGNED_SHORT, sizeof(VisVertex), (void*) offsetof(VisVertex, id)));
	}
	return gpu_meshes;
}
#include <core/png.h>
static void compute_vis_sample(u8* mask_dest, s32 mask_size_bytes, const glm::vec3& sample_point, GPUHandles& gpu) {
	static const glm::mat4 directions[6] = {
		glm::mat4(1.f),
		glm::rotate(glm::mat4(1.f), glm::radians(90.f), glm::vec3(0.f, 0.f, 1.f)),
		glm::rotate(glm::mat4(1.f), glm::radians(180.f), glm::vec3(0.f, 0.f, 1.f)),
		glm::rotate(glm::mat4(1.f), glm::radians(270.f), glm::vec3(0.f, 0.f, 1.f)),
		glm::rotate(glm::mat4(1.f), glm::radians(90.f), glm::vec3(0.f, 1.f, 0.f)),
		glm::rotate(glm::mat4(1.f), glm::radians(270.f), glm::vec3(0.f, 1.f, 0.f))
	};
	
	s32 render_size = VIS_RENDER_SIZE * VIS_RENDER_SIZE;
	std::vector<u16> buffer(render_size * 6);
	
	for(s32 i = 0; i < 6; i++) {
		GL_CALL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
		
		glm::mat4 perspective = glm::perspective(glm::radians(90.f), 1.f, 0.1f, 10000.0f);
		glm::mat4 translate = glm::translate(glm::mat4(1.0f), -sample_point);
		glm::mat4 matrix = perspective * RATCHET_TO_OPENGL_MATRIX * directions[i] * translate;
		
		GL_CALL(glUniformMatrix4fv(gpu.matrix_uniform, 1, GL_FALSE, &matrix[0][0]));
		
		for(const GPUVisMesh& vis_mesh : gpu.vis_meshes) {
			GL_CALL(glBindVertexArray(vis_mesh.vertex_array_object));
			GL_CALL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vis_mesh.index_buffer));
			GL_CALL(glDrawElements(GL_TRIANGLES, vis_mesh.vertex_count, GL_UNSIGNED_INT, nullptr));
		}
		
		GL_CALL(glFlush());
		GL_CALL(glFinish());
		GL_CALL(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
		
		
		GL_CALL(glReadPixels(0, 0, VIS_RENDER_SIZE, VIS_RENDER_SIZE, GL_RED_INTEGER, GL_UNSIGNED_SHORT, &buffer[i * render_size]));
		
		for(u16 id : buffer) {
			if(id > 0 && ((id) >> 3) < mask_size_bytes) {
				SET_BIT(mask_dest, id - 1, 1, mask_size_bytes);
			}
		}
	}
	
	VIS_DEBUG_DUMP(
		Texture texture;
		texture.width = VIS_RENDER_SIZE * 4;
		texture.height = VIS_RENDER_SIZE * 2;
		texture.format = PixelFormat::GRAYSCALE;
		texture.data.resize(texture.width * texture.height);
		for(s32 i = 0; i < 6; i++) {
			s32 base_x = (i % 4) * VIS_RENDER_SIZE;
			s32 base_y = (i / 4) * VIS_RENDER_SIZE;
			// Combine the renders into a panorama.
			for(s32 y = 0; y < VIS_RENDER_SIZE; y++) {
				for(s32 x = 0; x < VIS_RENDER_SIZE; x++) {
					texture.data[(base_y + y) * texture.width + base_x + x] = buffer[i * render_size + y * VIS_RENDER_SIZE + x];
				}
			}
		}
		FileOutputStream out;
		out.open(fs::path(stringf("/tmp/visout/%f_%f_%f.png", sample_point.x, sample_point.y, sample_point.z)));
		write_png(out, texture);
	)
}

static void compress_objects(std::vector<u8>& masks_dest, std::vector<s32>& mapping_dest, const std::vector<u8>& octant_masks_of_object_bits, s32 octant_count, s32 instance_count, s32 stride) {
	std::vector<s32> bit_mappings(instance_count, -1);
	
	verify_fatal(octant_masks_of_object_bits.size() % stride == 0);
	
	s32 object_mask_size = align32(octant_count, 64) / 8;
	
	// Convert the data into a form that should make the code below run faster:
	//  octant masks of object bits -> object masks of octant bits
	std::vector<u8> object_masks_of_octant_bits(object_mask_size * instance_count);
	for(s32 src_bit = 0; src_bit < instance_count; src_bit++) {
		for(s32 dest_bit = 0; dest_bit < octant_count; dest_bit++) {
			u8 bit;
			GET_BIT(bit, &octant_masks_of_object_bits[dest_bit * stride], src_bit, stride);
			SET_BIT(&object_masks_of_octant_bits[src_bit * object_mask_size], dest_bit, bit, object_mask_size);
		}
	}
	
	VIS_DEBUG_DUMP(write_file("/tmp/octantmasks.bin", octant_masks_of_object_bits));
	VIS_DEBUG_DUMP(write_file("/tmp/objectmasks.bin", object_masks_of_octant_bits));
	
	s32 bits_required = instance_count;
	
	for(s32 acceptable_error = 0;; acceptable_error++) {
		s32 prev_bits_required = bits_required;
		for(s32 lhs = 0; lhs < instance_count; lhs++) {
			s32 lhs_ofs = lhs * object_mask_size;
			for(s32 rhs = lhs + 1; rhs < instance_count; rhs++) {
				s32 rhs_ofs = rhs * object_mask_size;
				if(bit_mappings[rhs] == -1) {
					s32 error = 0;
					for(s32 ofs = 0; ofs < object_mask_size; ofs += 8) {
						u64 lhs_value = *(u64*) &object_masks_of_octant_bits[lhs_ofs + ofs];
						u64 rhs_value = *(u64*) &object_masks_of_octant_bits[rhs_ofs + ofs];
						error += std::popcount(lhs_value ^ rhs_value);
						if(error > acceptable_error) {
							break;
						}
					}
					if(error == acceptable_error) {
						bit_mappings[rhs] = lhs;
						bits_required--;
						if(bits_required <= 1024) {
							break;
						}
					}
				}
			}
			if(bits_required <= 1024) {
				break;
			}
		}
		if(bits_required <= 1024) {
			break;
		}
		if(acceptable_error > 0 && acceptable_error % 8 == 0) {
			printf("\n");
		}
		printf("%4d %4d ", acceptable_error, prev_bits_required - bits_required);
		fflush(stdout);
	}
	printf("\n");
	
	masks_dest.resize(octant_count * 128);
	mapping_dest.resize(instance_count, -1);
	
	// OR the merged bits together i.e. if at least one of the objects is
	// visible all merged objects will be drawn.
	for(size_t i = 0; i < bit_mappings.size(); i++) {
		if(bit_mappings[i] > -1) {
			for(s32 ofs = 0; ofs < object_mask_size; ofs += 8) {
				*(u64*) &object_masks_of_octant_bits[bit_mappings[i] * object_mask_size + ofs]
					|= *(u64*) &object_masks_of_octant_bits[i * object_mask_size + ofs];
			}
		}
	}
	
	// Write the output masks.
	for(s32 octant = 0; octant < octant_count; octant++) {
		s32 dest_bit = 0;
		for(s32 instance = 0; instance < instance_count; instance++) {
			if(bit_mappings[instance] == -1) {
				s32 bit;
				GET_BIT(bit, &object_masks_of_octant_bits[instance * object_mask_size], octant, stride);
				SET_BIT(&masks_dest[octant * 128], dest_bit, bit, stride);
				dest_bit++;
			}
		}
		verify_fatal(dest_bit <= 1024);
	}
	
	VIS_DEBUG_DUMP(write_file("/tmp/outmasks.bin", masks_dest));
	
	// Write the output mapping.
	s32 dest_bit = 0;
	for(s32 src_bit = 0; src_bit < instance_count; src_bit++) {
		if(bit_mappings[src_bit] == -1) {
			mapping_dest[src_bit] = dest_bit++;
		} else {
			mapping_dest[src_bit] = mapping_dest[bit_mappings[src_bit]];
			verify_fatal(mapping_dest[src_bit] > -1);
		}
	}
	verify_fatal(dest_bit <= 1024);
}

static void compress_octants(std::vector<u8>& compressed_vis_masks, s32 mask_count, s32 memory_budget_for_masks) {
	std::vector<s32> mappings(mask_count, -1);
	s32 max_masks = memory_budget_for_masks / 128;
	s32 masks_required = mask_count;
	
	// Determine which octant masks should be merged together.
	for(s32 acceptable_error = 0;; acceptable_error++) {
		printf("%d\n", acceptable_error);
		for(s32 lhs = 0; lhs < mask_count; lhs++) {
			s32 lhs_ofs = lhs * 128;
			for(s32 rhs = lhs + 1; rhs < mask_count; rhs++) {
				s32 rhs_ofs = rhs * 128;
				if(mappings[rhs] == -1) {
					s32 error = 0;
					for(s32 ofs = 0; ofs < 128; ofs += 8) {
						u64 lhs_value = *(u64*) &compressed_vis_masks[lhs_ofs + ofs];
						u64 rhs_value = *(u64*) &compressed_vis_masks[rhs_ofs + ofs];
						error += std::popcount(lhs_value ^ rhs_value);
						if(error > acceptable_error) {
							break;
						}
					}
					if(error == acceptable_error) {
						mappings[rhs] = lhs;
						masks_required--;
						if(masks_required <= max_masks) {
							break;
						}
					}
				}
			}
			if(masks_required <= max_masks) {
				break;
			}
		}
		if(masks_required <= max_masks) {
			break;
		}
	}
	
	// OR all the merged octants together.
	for(s32 i = 0; i < mask_count; i++) {
		if(mappings[i] > -1) {
			for(s32 ofs = 0; ofs < 128; ofs += 8) {
				*(u64*) &compressed_vis_masks[mappings[i] * 128 + ofs]
					|= *(u64*) &compressed_vis_masks[i * 128 + ofs];
			}
		}
	}
	
	// Overwrite all the mapped masks to the masks they're mapped to so they can
	// be deduplicated later.
	for(s32 i = 0; i < mask_count; i++) {
		if(mappings[i] > -1) {
			for(s32 ofs = 0; ofs < 128; ofs += 8) {
				*(u64*) &compressed_vis_masks[i * 128 + ofs]
					= *(u64*) &compressed_vis_masks[mappings[i] * 128 + ofs];
			}
		}
	}
}

static void shutdown_opengl(GPUHandles& gpu) {
	GL_CALL(glDeleteFramebuffers(1, &gpu.frame_buffer));
	GL_CALL(glDeleteTextures(1, &gpu.id_buffer));
	GL_CALL(glDeleteTextures(1, &gpu.depth_buffer));
	GL_CALL(glDeleteProgram(gpu.program));
	
	for(GPUVisMesh& mesh : gpu.vis_meshes) {
		GL_CALL(glDeleteBuffers(1, &mesh.vertex_buffer));
		GL_CALL(glDeleteBuffers(1, &mesh.index_buffer));
	}
	
	glfwDestroyWindow(gpu.dummy);
	
	glfwTerminate();
}
