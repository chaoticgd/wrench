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

#include "texture.h"

static Texture paletted_texture_to_full_colour(Buffer data, Buffer palette, s32 width, s32 height);
static u8 decode_palette_index(u8 index);

std::vector<Texture> read_tfrag_textures(BufferArray<TextureEntry> texture_table, Buffer data, Buffer gs_ram) {
	std::vector<Texture> textures;
	for(const TextureEntry& entry : texture_table) {
		Buffer texture = data.subbuf(entry.data_offset);
		Buffer palette = gs_ram.subbuf(entry.palette * 0x100);
		textures.emplace_back(paletted_texture_to_full_colour(texture, palette, entry.width, entry.height));
	}
	return textures;
}

std::vector<Texture> read_instance_textures(BufferArray<TextureEntry> texture_table, const u8 indices[16], Buffer data, Buffer gs_ram) {
	std::vector<Texture> textures;
	for(s32 i = 0; i < 16; i++) {
		if(indices[i] == 0xff) {
			break;
		}
		const TextureEntry& entry = texture_table[indices[i]];
		Buffer texture = data.subbuf(entry.data_offset);
		Buffer palette = gs_ram.subbuf(entry.palette * 0x100);
		textures.emplace_back(paletted_texture_to_full_colour(texture, palette, entry.width, entry.height));
	}
	return textures;
}

static Texture paletted_texture_to_full_colour(Buffer data, Buffer palette, s32 width, s32 height) {
	Texture texture;
	texture.width = width;
	texture.height = height;
	texture.data.resize(width * height);
	for(s32 i = 0; i < width * height; i++) {
		texture.data[i] = palette.read<Colour32>(decode_palette_index(data.read<u8>(i, "texture")) * 4, "palette");
		texture.data[i].a = std::min(2 * (s32) texture.data[i].a, 0xff);
	}
	return texture;
}

static u8 decode_palette_index(u8 index) {
	// Swap middle two bits
	//  e.g. 00010000 becomes 00001000.
	return (((index & 16) >> 1) != (index & 8)) ? (index ^ 0b00011000) : index;
}
