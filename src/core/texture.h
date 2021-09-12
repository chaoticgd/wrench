/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2021 chaoticgd

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

#ifndef TEXTURE_H
#define TEXTURE_H

#include "util.h"

enum class PixelFormat {
	IDTEX8 = 0
};

struct Palette {
	std::array<u32, 256> colours;
	s32 top;
	
	bool operator==(const Palette& rhs) const {
		return colours == rhs.colours && top == rhs.top;
	}
};

struct Texture {
	s32 width;
	s32 height;
	PixelFormat format;
	Palette palette;
	std::vector<u8> pixels;
};

std::string hash_texture(const Texture& texture);

#endif
