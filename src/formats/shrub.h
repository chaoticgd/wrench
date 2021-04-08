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

#ifndef FORMATS_SHRUB_H
#define FORMATS_SHRUB_H

#include <map>

#include "../model.h"
#include "../stream.h"
#include "../gl_includes.h"
#include "vif.h"

# /*
#	Parse a game model.
# */

struct app;
struct gl_renderer;

packed_struct(shrub_model_header,
	float unknown_0;		// 0x0
	float unknown_4;		// 0x4
	float unknown_8;		// 0x8
	float unknown_c;		// 0xc
	float unknown_10;		// 0x10
	float unknown_14;		// 0x14
	float unknown_18;		// 0x18
	float unknown_1c;		// 0x1c
	float scale;			// 0x20
	uint32_t o_class;		// 0x24
	uint32_t submodel_count;// 0x28
	uint32_t unknown_2c;    // 0x2c
	uint32_t unknown_30;	// 0x30
	uint32_t unknown_34;	// 0x34
	uint32_t unknown_38;	// 0x38
	uint32_t unknown_3c;	// 0x3c
)

packed_struct(shrub_submodel_entry,
	uint32_t vif_list_offset;
	uint32_t vif_list_size;
)

packed_struct(shrub_model_vertex, // Second UNPACK
	int16_t x; // 0x0
	int16_t y; // 0x2
	int16_t z; // 0x4
	uint16_t id; // 0x6
)

packed_struct(shrub_model_st, // Third UNPACK
	int16_t s;
	int16_t t;
	int16_t unknown_4;
	int16_t unknown_6;
)

// A single submodel may contain vertices with different textures. Since it's
// unclear as to whether there's a limit on the number of textures a single
// submodel can have, and for the purposes of simplifying the OpenGL rendering
// code, we split each submodel into subsubmodels.
struct shrub_subsubmodel {
	std::vector<uint8_t> indices;
	gl_buffer index_buffer;
};

struct shrub_submodel {
	std::vector<vif_packet> vif_list;
	std::vector<shrub_subsubmodel> subsubmodels;
	std::vector<shrub_model_vertex> vertices;
	std::vector<shrub_model_st> st_coords;
	gl_buffer vertex_buffer;
	gl_buffer st_buffer;
	bool visible_in_model_viewer = true;
};

class shrub_model : public model {
public:
	shrub_model(
		stream* backing,
		std::size_t base_offset,
		std::size_t size);
	shrub_model(const shrub_model&) = delete;
	shrub_model(shrub_model&&) = default;

	struct interpreted_shrub_vif_list {
		std::vector<shrub_model_vertex> vertices;
		std::vector<shrub_model_st> st_data;
	};

	void read();

	// Reads data from the parsed VIF DMA list into a more suitable structure.
	interpreted_shrub_vif_list interpret_vif_list(
		const std::vector<vif_packet>& vif_list);

	// Splits a submodel into subsubmodels such that each part of a submodel
	// with a different texture has its own subsubmodel. The game will change
	// the applied texture when an index of zero is encountered, so when we
	// split up the index buffer, we need to make cuts at those positions.
	std::vector<shrub_subsubmodel> read_subsubmodels(
		interpreted_shrub_vif_list submodel_data);

	// Check if any of the indices overrun the vertex table.
	bool validate_indices(const shrub_submodel& submodel);

	// Print message along with details of the current submodel.
	void warn_current_submodel(const char* message);

	// adds vertex chain to triangle list
	void add_vertex_chain(
		const shrub_submodel& submodel,
		size_t start,
		size_t end);

	std::vector<shrub_submodel> submodels;
	float scale = 1.f;

	gl_texture thumbnail;

	std::string resource_path() const;

	std::string name() { return _backing.name; }
	void set_name(std::string name) { _backing.name = name; }

	// This is used to index into the relevant array of textures depending on
	// the type of model this is. For example, for moby models this would index
	// into the std::vector of moby textures.
	std::vector<std::size_t> texture_indices;

	std::vector<float> triangles() const override;
	std::vector<float> colors() const override;
private:
	proxy_stream _backing;

	std::vector<float> _triangles;
	std::vector<float> _vertex_colors;
};


#endif
