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

#/*
#	Parse a game model.
# */


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

	tfrag(stream *backing, std::size_t base_offset, uint16_t vertex_count, uint16_t vertex_offset);
	tfrag(tfrag&& rhs);

	std::vector<float> triangles() const override;

private:
	proxy_stream _backing;

	uint16_t _vertex_offset;
	uint16_t _vertex_count;
};

#endif
