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

#ifndef FORMATS_TFRAG_H
#define FORMATS_TFRAG_H

#include <map>

#include "../model.h"
#include "../stream.h"
#include "vif.h"
#include "level_types.h"

packed_struct(tfrag_entry,
	uint32_t unknown_0; //0x00
	uint32_t unknown_4; //0x04
	uint32_t unknown_8; //0x08
	uint32_t unknown_c; //0x0c
	uint32_t offset; //0x10 offset from start of tfrag_entry list
	uint16_t unknown_14;
	uint16_t unknown_16;
	uint32_t unknown_18;
	uint16_t unknown_1c;
	uint16_t color_offset;
	uint32_t unknown_20;
	uint8_t unknown_24;
	uint8_t unknown_25;
	uint8_t unknown_26;
	uint8_t unknown_27;
	uint32_t unknown_28;
	uint8_t vertex_count;
	uint8_t unknown_2d;
	uint16_t vertex_offset;
	uint16_t unknown_30;
	uint16_t unknown_32;
	uint32_t unknown_34;
	uint32_t unknown_38;
	uint8_t color_count;
	uint8_t unknown_3d;
	uint8_t unknown_3e;
	uint8_t unknown_3f;
);

packed_struct(tfrag_texture_data, // Third UNPACK.
	uint32_t texture_index;
	uint32_t unknown_4;
	uint32_t unknown_8;
	uint32_t unknown_c;
	uint32_t unknown_10;
	uint32_t unknown_14;
	uint32_t unknown_18;
	uint32_t unknown_1c;
	int32_t unknown_20;
	uint32_t unknown_24;
	uint32_t unknown_28;
	uint32_t unknown_2c;
	uint32_t unknown_30;
	uint32_t unknown_34;
	uint32_t unknown_38;
	uint32_t unknown_3c;
	uint32_t unknown_40;
	uint32_t unknown_44;
	uint32_t unknown_48;
	uint32_t unknown_4c;
)

packed_struct(tfrag_st_index, // Fourth UNPACK
	int16_t s;
	int16_t t;
	int16_t unknown_4;
	int16_t vid;
)

packed_struct(tfrag_displace, // Fifth UNPACK
	int16_t x;
	int16_t y;
	int16_t z;
)

class tfrag : public model {
public:
	struct fmt {
		packed_struct(vertex,
			float x; //0x00
			float y; //0x04
			float z; //0x08
			uint32_t unknown_c; //0x0c
		);
	};

	struct interpreted_tfrag_vif_list {
		std::vector<tfrag_st_index> st_data;
		std::vector<tfrag_displace> displace_data;
		std::vector<uint8_t> indices; // some kind of ngon
		std::array<uint32_t, 4> position; // some kind of ngon
		std::vector<tfrag_texture_data> textures;
	};

	tfrag(stream *backing, std::size_t base_offset, tfrag_entry & entry);

	tfrag::interpreted_tfrag_vif_list interpret_vif_list(
		const std::vector<vif_packet>& vif_list);

	void warn_current_tfrag(const char* message);
	std::vector<float> triangles() const override;

private:
	proxy_stream _backing;

	uint8_t _final_vertex_count;
	size_t _base_offset;

	std::vector<vec3f> _tfrag_points;
	std::vector<vif_packet> _vif_list;
	std::vector<float> _tfrag_triangles;
};

#endif
