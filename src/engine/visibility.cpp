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

struct VisVertex {
	f32 x;
	f32 y;
	f32 z;
	u32 id; // 0..5 = object index, 6..7 = object type
};

struct VisAABB {
	glm::vec3 min;
	glm::vec3 max;
};

struct VisMesh {
	VisMesh(VisMesh&& rhs) = delete;
	~VisMesh() {
		if(vertex_buffer_handle != 0) glDeleteBuffers(1, &vertex_buffer_handle);
		if(index_buffer_handle != 0) glDeleteBuffers(1, &index_buffer_handle);
	}
	GLuint vertex_buffer_handle = 0;
	GLuint index_buffer_handle = 0;
	VisAABB bounding_box;
};

static void generate_mappings(std::vector<s32> dest[VIS_OBJECT_TYPE_COUNT], const VisInput& input);
static std::vector<VisMesh> build_vis_meshes(const VisInput& input, const std::vector<s32>* mappings);
static void compute_vis_sample(u8 dest[128], const glm::vec3& sample_point, const std::vector<VisMesh>& meshes);
static void set_bit(u8 dest[128], s32 index);

static const glm::vec3 OCTANT_SAMPLES[] = {
	{0.5f, 0.5f, 0.5f}
};

VisOutput compute_level_visibility(const VisInput& input) {
	puts("Computing visibility...");
	
	verify(glfwInit(), "Failed to load GLFW.");
	defer([&]() { glfwTerminate(); });
	
	VisOutput output;
	
	// Map different objects to a particular bit in the visibility mask stored
	// for each octant.
	generate_mappings(output.mappings, input);
	
	// Batch the meshes together and upload them to the GPU.
	std::vector<VisMesh> meshes = build_vis_meshes(input, output.mappings);
	srand(time(NULL));
	// Determine which objects are visible and populate the visibility mask for
	// each octant.
	for(size_t i = 0; i < input.octants.size(); i++) {
		const OcclusionVector& src = input.octants[i];
		printf("%d,%d,%d%c", src.x, src.y, src.z, (i % 4 == 3) ? '\n' : ' ');
		OcclusionOctant& dest = output.octants.emplace_back();
		dest.x = src.x;
		dest.y = src.y;
		dest.z = src.z;
		for(s32 j = 0; j < 1024; j++) {
			if(rand() % 2 == 0) {
				set_bit(dest.visibility, j);
			}
		}
		for(const glm::vec3& sample : OCTANT_SAMPLES) {
			glm::vec3 sample_point = {
				((f32) src.x + sample.x) * input.octant_size_x,
				((f32) src.y + sample.y) * input.octant_size_y,
				((f32) src.z + sample.z) * input.octant_size_z
			};
			compute_vis_sample(dest.visibility, sample_point, meshes);
		}
	}
	printf("\n");
	
	puts("Done computing visibility!");
	
	return output;
}

static void generate_mappings(std::vector<s32> dest[VIS_OBJECT_TYPE_COUNT], const VisInput& input) {
	return;
}

static std::vector<VisMesh> build_vis_meshes(const VisInput& input, const std::vector<s32>* mappings) {
	return {};
}

static void compute_vis_sample(u8 dest[128], const glm::vec3& sample_point, const std::vector<VisMesh>& meshes) {
	return;
}

static void set_bit(u8 dest[128], s32 index) {
	s32 byte_index = index >> 3;
	s32 bit_index = index & 7;
	verify(byte_index > -1 || byte_index < 128, "Tried to set set bit out of range.");
	dest[byte_index] |= 1 << bit_index;
}
