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

std::pair<std::vector<const Texture*>, FlattenedTextureLayout> flatten_textures(const LevelWad& wad) {
	std::vector<const Texture*> pointers;
	pointers.reserve(wad.tfrag_textures.size());
	FlattenedTextureLayout layout;
	layout.tfrags_begin = pointers.size();
	for(const Texture& texture : wad.tfrag_textures) {
		pointers.push_back(&texture);
	}
	layout.mobies_begin = pointers.size();
	for(const auto& [number, moby_class] : wad.moby_classes) {
		for(const Texture& texture : moby_class.textures) {
			pointers.push_back(&texture);
		}
	}
	layout.ties_begin = pointers.size();
	for(const auto& [number, tie_class] : wad.tie_classes) {
		for(const Texture& texture : tie_class.textures) {
			pointers.push_back(&texture);
		}
	}
	layout.shrubs_begin = pointers.size();
	for(const auto& [number, shrub_class] : wad.shrub_classes) {
		for(const Texture& texture : shrub_class.textures) {
			pointers.push_back(&texture);
		}
	}
	return {pointers, layout};
}

PalettedTexture find_suboptimal_palette(const Texture& src) {
	PalettedTexture texture = {0};
	texture.width = src.width;
	texture.height = src.height;
	texture.data.resize(texture.width * texture.height);
	s32 palette_top = 0;
	for(s32 i = 0; i < texture.width * texture.height; i++) {
		s32 match = -1;
		for(s32 j = 0; j < 256; j++) {
			if(texture.palette[j] == src.data[i]) {
				match = j;
				break;
			}
		}
		if(match >= 0) {
			texture.data[i] = (u8) match;
		} else if(palette_top < 256) {
			texture.palette[palette_top] = src.data[i];
			texture.data[i] = palette_top;
			palette_top++;
		} else {
			texture.data[i] = 0;
		}
	}
	return texture;
}

std::vector<PalettedTexture*> deduplicate_textures(std::vector<PalettedTexture>& src) {
	std::vector<size_t> mapping(src.size());
	for(size_t i = 0; i < src.size(); i++) {
		mapping[i] = i;
	}
	
	std::sort(BEGIN_END(mapping), [&](size_t lhs, size_t rhs) {
		return src[lhs].data < src[rhs].data;
	});
	
	std::vector<PalettedTexture*> pointers(src.size());
	pointers[0] = &src[mapping[0]];
	for(size_t i = 1; i < src.size(); i++) {
		PalettedTexture& last = src[mapping[i - 1]];
		PalettedTexture& cur = src[mapping[i]];
		if(last.data == cur.data) {
			pointers[i] = pointers[i - 1];
		} else {
			cur.duplicate_data = false;
			pointers[i] = &cur;
		}
	}
	return pointers;
}

std::vector<PalettedTexture*> deduplicate_palettes(std::vector<PalettedTexture>& src) {
	std::vector<size_t> mapping(src.size());
	for(size_t i = 0; i < src.size(); i++) {
		mapping[i] = i;
	}
	
	std::sort(BEGIN_END(mapping), [&](size_t lhs, size_t rhs) {
		return src[lhs].palette < src[rhs].palette;
	});
	
	std::vector<PalettedTexture*> pointers(src.size());
	pointers[0] = &src[mapping[0]];
	for(size_t i = 1; i < src.size(); i++) {
		PalettedTexture& last = src[mapping[i - 1]];
		PalettedTexture& cur = src[mapping[i]];
		if(last.palette == cur.palette) {
			pointers[i] = pointers[i - 1];
		} else {
			cur.duplicate_palette = false;
			pointers[i] = &cur;
		}
	}
	return pointers;
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
