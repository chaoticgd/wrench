/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2020 chaoticgd

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

#ifndef FORMATS_GAME_MODEL_H
#define FORMATS_GAME_MODEL_H

#include <map>

#include "../model.h"
#include "../stream.h"
#include "../gl_includes.h"
#include "glm/ext/vector_float3.hpp"
#include "glm/vec3.hpp"
#include "glm/fwd.hpp"
#include "vif.h"

# /*
#	Parse a game model.
# */

struct app;
struct gl_renderer;

enum class moby_model_header_type {
	LEVEL, ARMOR
};

packed_struct(moby_model_level_header,
	uint32_t rel_offset;    // 0x0
	uint8_t unknown_4;      // 0x4
	uint8_t unknown_5;      // 0x5
	uint8_t unknown_6;      // 0x6
	uint8_t submodel_count; // 0x7
	uint32_t unknown_8;     // 0x8
	uint32_t unknown_c;     // 0xc
	uint32_t unknown_10;    // 0x10
	uint32_t unknown_14;    // 0x14
	uint32_t unknown_18;    // 0x18
	uint32_t unknown_1c;    // 0x1c
	uint32_t unknown_20;    // 0x20
	float scale;            // 0x24
)

packed_struct(moby_model_armor_header,
	uint8_t submodel_count_1;             // 0x0
	uint8_t submodel_count_2;             // 0x1
	uint8_t submodel_count_3;             // 0x2
	uint8_t submodel_count_1_plus_2;      // 0x3
	uint32_t submodel_table_offset;       // 0x4
	uint32_t texture_applications_offset; // 0x8
)

packed_struct(moby_submodel_entry,
	uint32_t vif_list_offset;
	uint16_t vif_list_quadword_count; // Size in 16 byte units.
	uint16_t vif_list_texture_unpack_offset; // No third UNPACK if zero.
	uint32_t vertex_offset;
	uint8_t vertex_data_quadword_count; // Includes header, in 16 byte units.
	uint8_t unknown_d; // unknown_d == (0xf + transfer_vertex_count * 6) / 0x10
	uint8_t unknown_e; // unknown_e == (3 + transfer_vertex_count) / 4
	uint8_t transfer_vertex_count; // Number of vertices sent to VU1.
)

packed_struct(moby_model_vertex_table_header,
	uint16_t unknown_0;
	uint16_t vertex_count_2;
	uint16_t vertex_count_4;
	uint16_t main_vertex_count;
	uint16_t vertex_count_8;
	uint16_t transfer_vertex_count; // transfer_vertex_count == vertex_count_2 + vertex_count_4 + main_vertex_count + vertex_count_8
	uint16_t vertex_table_offset;
	uint16_t unknown_e;
	// More stuff comes between this and the vertex table.
)

packed_struct(moby_model_vertex,
	uint8_t unknown_0; // 0x0
	uint8_t unknown_1; // 0x1
	uint8_t unknown_2; // 0x2
	uint8_t unknown_3; // 0x3
	uint8_t unknown_4; // 0x4
	uint8_t unknown_5; // 0x5
	uint8_t unknown_6; // 0x6
	uint8_t unknown_7; // 0x7
	uint8_t unknown_8; // 0x8
	uint8_t unknown_9; // 0x9
	int16_t x;         // 0xa
	int16_t y;         // 0xc
	int16_t z;         // 0xe
)

packed_struct(moby_model_st, // First UNPACK.
	int16_t s;
	int16_t t;
)

packed_struct(moby_model_index_header, // Second UNPACK header.
	uint8_t unknown_0;
	uint8_t texture_unpack_offset_quadwords; // Offset of texture data relative to decompressed index buffer in VU mem.
	uint8_t unknown_2;
	uint8_t unknown_3;
	// Indices directly follow.
)

packed_struct(moby_model_texture_data, // Third UNPACK.
	uint32_t unknown_0;
	uint32_t unknown_4;
	uint32_t unknown_8;
	uint32_t unknown_c;
	uint32_t unknown_10;
	uint32_t unknown_14;
	uint32_t unknown_18;
	uint32_t unknown_1c;
	int32_t texture_index; // Overwritten with the texture address by the game at runtime.
	uint32_t unknown_24;
	uint32_t unknown_28;
	uint32_t unknown_2c;
	uint32_t unknown_30;
	uint32_t unknown_34;
	uint32_t unknown_38;
	uint32_t unknown_3c;
)

// A single submodel may contain vertices with different textures. Since it's
// unclear as to whether there's a limit on the number of textures a single
// submodel can have, and for the purposes of simplifying the OpenGL rendering
// code, we split each submodel into subsubmodels.
struct moby_subsubmodel {
	std::vector<uint8_t> indices;
	std::optional<moby_model_texture_data> texture; // If empty use last texture from last submodel with one.
	gl_buffer index_buffer;
};

struct moby_submodel {
	std::vector<vif_packet> vif_list;
	moby_model_index_header index_header;
	std::vector<moby_subsubmodel> subsubmodels;
	std::vector<moby_model_vertex> vertices;
	std::vector<moby_model_st> st_coords;
	gl_buffer vertex_buffer;
	gl_buffer st_buffer;
	bool visible_in_model_viewer = true;
};

//Axis aligned bounding box of verts
struct moby_bounding_box {
	glm::vec3 min = glm::vec3(INT16_MAX);
	glm::vec3 max = glm::vec3(-INT16_MAX);
};

class moby_model {
public:
	moby_model(
		stream* backing,
		std::size_t base_offset,
		std::size_t size,
		moby_model_header_type type);
	moby_model(const moby_model&) = delete;
	moby_model(moby_model&&) = default;

	struct interpreted_moby_vif_list {
		std::vector<moby_model_st> st_data;
		moby_model_index_header index_header;
		std::vector<int8_t> indices;
		std::vector<moby_model_texture_data> textures;
	};

	void read();

	// Reads data from the parsed VIF DMA list into a more suitable structure.
	interpreted_moby_vif_list interpret_vif_list(
		const std::vector<vif_packet>& vif_list);
	
	// Splits a submodel into subsubmodels such that each part of a submodel
	// with a different texture has its own subsubmodel. The game will change
	// the applied texture when an index of zero is encountered, so when we
	// split up the index buffer, we need to make cuts at those positions.
	std::vector<moby_subsubmodel> read_subsubmodels(
		interpreted_moby_vif_list submodel_data);
	
	// Print message along with details of the current submodel.
	void warn_current_submodel(const char* message);
	
	void import_ply(std::string path);
	static std::vector<vif_packet> regenerate_submodel_vif_list(moby_submodel& submodel);
	void write();
	
	std::vector<moby_submodel> submodels;
	float scale = 1.f;
	
	moby_bounding_box bounding_box;

	gl_texture thumbnail;

	std::string resource_path() const;
	
	std::string name() { return _backing.name; }
	void set_name(std::string name) { _backing.name = name; }
	
	// This is used to index into the relevant array of textures depending on
	// the type of model this is. For example, for moby models this would index
	// into the std::vector of moby textures.
	std::vector<std::size_t> texture_indices;
private:
	proxy_stream _backing;
	moby_model_header_type _type;
};

#endif
