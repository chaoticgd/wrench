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

#ifndef FORMATS_TEXTURE_H
#define FORMATS_TEXTURE_H

#include <array>
#include <vector>
#include <stdint.h>
#include <glm/glm.hpp>

#include "../stream.h"

# /*
#	Stream-backed indexed texture.
# */

struct colour {
	uint8_t r, g, b, a;
};

struct vec2i {
	int x, y;
	
	bool operator==(vec2i& rhs) {
		return x == rhs.x && y == rhs.y;
	}
	
	bool operator!=(vec2i& rhs) {
		return x != rhs.x || y != rhs.y;
	}
};

class texture {
public:
	texture(
		stream* pixel_backing,
		std::size_t pixel_data_offset,
		stream* palette_backing,
		std::size_t palette_offset,
		vec2i size);

	vec2i size() const;

	std::array<colour, 256> palette() const;
	void set_palette(std::array<colour, 256> palette_);

	std::vector<uint8_t> pixel_data() const;
	void set_pixel_data(std::vector<uint8_t> pixel_data_);

	std::string palette_path() const;
	std::string pixel_data_path() const;
	
	std::string name;
	
private:
	stream* _pixel_backing;
	std::size_t _pixel_data_offset;
	stream* _palette_backing;
	std::size_t _palette_offset;
	vec2i _size;
};

// Won't affect the position indicator of backing.
std::optional<texture> create_fip_texture(stream* backing, std::size_t offset);

#endif
