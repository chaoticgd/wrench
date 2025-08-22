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

enum class PixelFormat
{
	RGBA,
	GRAYSCALE,
	PALETTED_4,
	PALETTED_8
};

struct TextureMipmaps
{
	s32 mip_levels = 0;
	std::vector<u8> mips[4];
	std::vector<u32> palette;
};

class Texture
{
public:
	Texture();
	
	static Texture create_rgba(s32 width, s32 height, std::vector<u8> data);
	static Texture create_grayscale(s32 width, s32 height, std::vector<u8> data);
	static Texture create_4bit_paletted(s32 width, s32 height, std::vector<u8> data, std::vector<u32> palette);
	static Texture create_8bit_paletted(s32 width, s32 height, std::vector<u8> data, std::vector<u32> palette);
	
	s32 width;
	s32 height;
	PixelFormat format;
	std::vector<u8> data;
	
	s32 bits_per_component() const;
	s32 bits_per_pixel() const;
	std::vector<u32>& palette();
	const std::vector<u32>& palette() const;
	
	void to_rgba();
	void to_grayscale();
	void to_8bit_paletted();
	void to_4bit_paletted();
	
	void reswizzle();
	void swizzle();
	void swizzle_palette();
	
	void multiply_alphas(); // Maps [0,0x80] to [0,0xff].
	void divide_alphas(bool handle_80s = true); // Maps [0,0xff] to [0,0x80].
	
	TextureMipmaps generate_mipmaps(s32 max_mip_levels);
	void reduce();
	
	void destroy();
	
	bool operator<(const Texture& rhs) const;
	bool operator==(const Texture& rhs) const;
	
private:
	std::vector<u32> m_palette;
};

#endif
