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

#ifndef FORMATS_TERRAIN_COLLISION_H
#define FORMATS_TERRAIN_COLLISION_H

#include <map>

#include "../model.h"
#include "../stream.h"
#include "level_types.h"

packed_struct(tcol_header,
	uint32_t collision_offset;	//0x00
	uint32_t unknown_4;			//0x04
	uint32_t unknown_8;			//0x08
	uint32_t unknown_c;			//0x0c
);

class tcol : public model {
public:

	template <typename T>
	struct tcol_list {
		int16_t coordinate_value;			// Value in file is divided by 4
		std::vector<T> list;
	};

	struct tcol_face {
		uint8_t v0;
		uint8_t v1;
		uint8_t v2;
		uint8_t v3;
		uint8_t collision_id;
		bool is_quad;
	};

	struct tcol_data {
		std::vector<vec3f> vertices; // these are relative to coordinate_values
		std::vector<tcol_face> faces;
	};

	tcol(stream *backing, std::size_t base_offset);

	void push_face(vec3f offset, tcol::tcol_face face, tcol::tcol_data data);

	vec3f get_collision_color(uint8_t colId);
	vec3f unpack_vertex(uint32_t vertex);

	std::vector<float> triangles() const override;
	std::vector<float> colors() const override;

	tcol_list<tcol_list<tcol_list<tcol_data>>> data;

private:
	proxy_stream _backing;

	uint8_t _final_vertex_count;
	size_t _base_offset;

	std::vector<float> _tcol_triangles;
	std::vector<float> _tcol_vertex_colors;
};

#endif
