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
#include "vif.h"

# /*
#	Parse a game model.
# */

struct gl_renderer;

packed_struct(moby_model_submodel_entry,
	uint32_t vif_list_offset;
	uint16_t vif_list_quadword_count; // Size in 16 byte units.
	uint16_t vif_list_texture_unpack_offset; // No third UNPACK if zero.
	uint32_t vertex_offset;
	uint8_t vertex_data_quadword_count; // Includes header, in 16 byte units.
	uint8_t unknown_d;
	uint8_t unknown_e;
	uint8_t unknown_f;
)

packed_struct(moby_model_vertex_table_header,
	uint32_t unknown_0;
	uint16_t unknown_4;
	uint16_t unknown_6;
	uint32_t unknown_8;
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

struct moby_model_opengl_vertex {
	float x;
	float y;
	float z;
};

packed_struct(moby_model_st, // First UNPACK.
	int16_t s;
	int16_t t;
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

struct moby_model_submodel {
	std::vector<vif_packet> vif_list;
	std::vector<moby_model_st> st_data;
	std::vector<int8_t> index_data;
	std::optional<moby_model_texture_data> texture; // If empty use last submodel.
	std::vector<moby_model_vertex> vertex_data;
	bool visible_in_model_viewer;
	GLuint vertex_buffer;
	GLuint st_buffer;
};

class moby_model {
public:
	moby_model(
		stream* backing,
		std::size_t base_offset,
		std::size_t size,
		std::size_t submodel_table_offset,
		std::vector<std::size_t> submodel_counts_);
	moby_model(const moby_model& rhs) = delete;
	moby_model(moby_model&& rhs);
	~moby_model();

	void read();
	void write();
	
	std::vector<std::size_t> submodel_counts;
	std::vector<moby_model_submodel> submodels;
	
	void upload_vertex_buffer(); // Must be called from main thread.
	void setup_vertex_attributes() const;
	GLuint vertex_buffer() const;
	std::size_t vertex_count() const; // Includes degenerate tris between submodels.
	
	GLuint thumbnail;

	std::string resource_path() const;
	
	std::string name() { return _backing.name; }
	void set_name(std::string name) { _backing.name = name; }
	
private:
	GLuint _vertex_buffer;
	std::size_t _vertex_count;

	proxy_stream _backing;
	std::size_t _submodel_table_offset; // Relative to base_offset.
};

std::vector<moby_model_opengl_vertex> moby_vertex_data_to_opengl(const moby_model_submodel& submodel);

#endif
