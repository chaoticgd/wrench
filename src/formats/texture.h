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
#include "../gl_includes.h"

struct colour {
	uint8_t r, g, b, a;
};

struct vec2i {
	std::size_t x, y;
	
	bool operator==(vec2i& rhs) {
		return x == rhs.x && y == rhs.y;
	}
	
	bool operator!=(vec2i& rhs) {
		return x != rhs.x || y != rhs.y;
	}
};

struct texture {
	vec2i size;
	std::vector<uint8_t> pixels;
	colour palette[256];
	std::string name;
	
#ifdef WRENCH_EDITOR
	void upload_to_opengl();
	gl_texture opengl_texture;
#endif
};

texture create_texture_from_streams(vec2i size, stream* pixel_src, size_t pixel_offset, stream* palette_src, size_t palette_offset);

// Won't affect the position indicator of backing.
std::optional<texture> create_fip_texture(stream* backing, std::size_t offset);

// Read a list of textures in the following format:
//  uint32_t count;
//  uint32_t offsets[count];
//  ... PIF textures ...
std::vector<texture> read_pif_list(stream* backing, std::size_t offset);

#endif
