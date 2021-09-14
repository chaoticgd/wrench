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

void write_nonshared_texture_data(OutBuffer data, std::vector<Texture>& stexturesc);
static Texture read_paletted_texture(Buffer data, Buffer palette, s32 width, s32 height);
static u8 map_palette_index(u8 index);

void read_texture_table(std::vector<Texture>& dest, BufferArray<TextureEntry> texture_table, Buffer data, Buffer gs_ram) {
	for(const TextureEntry& entry : texture_table) {
		Buffer texture = data.subbuf(entry.data_offset);
		Buffer palette = gs_ram.subbuf(entry.palette * 0x100);
		dest.emplace_back(read_paletted_texture(texture, palette, entry.width, entry.height));
	}
}

std::vector<Texture> read_particle_textures(BufferArray<ParticleTextureEntry> texture_table, Buffer data) {
	std::vector<Texture> textures;
	for(const ParticleTextureEntry& entry : texture_table) {
		Buffer palette = data.subbuf(entry.palette);
		Buffer texture = data.subbuf(entry.texture);
		textures.emplace_back(read_paletted_texture(texture, palette, entry.side, entry.side));
	}
	return textures;
}

ArrayRange write_particle_textures(OutBuffer header, OutBuffer data, std::vector<Texture>& textures) {
	s64 particle_base = data.tell();
	write_nonshared_texture_data(data, textures);
	ArrayRange range;
	range.count = textures.size();
	range.offset = header.tell();
	for(Texture& texture : textures) {
		Texture* palette_texture = &texture;
		if(palette_texture->palette_out_edge > -1) {
			palette_texture = &textures[palette_texture->palette_out_edge];
		}
		
		ParticleTextureEntry entry;
		entry.palette = palette_texture->palette_offset - particle_base;
		entry.unknown_4 = 0;
		entry.texture = texture.texture_offset - particle_base;
		entry.side = texture.width;
		header.write(entry);
	}
	return range;
}

std::vector<Texture> read_fx_textures(BufferArray<FXTextureEntry> texture_table, Buffer data) {
	std::vector<Texture> textures;
	for(const FXTextureEntry& entry : texture_table) {
		Buffer palette = data.subbuf(entry.palette);
		Buffer texture = data.subbuf(entry.texture);
		textures.emplace_back(read_paletted_texture(texture, palette, entry.width, entry.height));
	}
	return textures;
}

ArrayRange write_fx_textures(OutBuffer header, OutBuffer data, std::vector<Texture>& textures) {
	s64 fx_base = data.tell();
	write_nonshared_texture_data(data, textures);
	ArrayRange range;
	range.count = textures.size();
	range.offset = header.tell();
	for(Texture& texture : textures) {
		Texture* palette_texture = &texture;
		if(palette_texture->palette_out_edge > -1) {
			palette_texture = &textures[palette_texture->palette_out_edge];
		}
		
		FXTextureEntry entry;
		entry.palette = palette_texture->palette_offset - fx_base;
		entry.texture = texture.texture_offset - fx_base;
		entry.width = texture.width;
		entry.height = texture.height;
		header.write(entry);
	}
	return range;
}

void write_nonshared_texture_data(OutBuffer data, std::vector<Texture>& textures) {
	deduplicate_palettes(textures);
	for(Texture& texture : textures) {
		assert(texture.palette_out_edge == -1 || textures[texture.palette_out_edge].palette_out_edge == -1);
	}
	
	for(Texture& texture : textures) {
		if(texture.palette_out_edge == -1) {
			texture.palette_offset = write_palette(data, texture.palette);
		}
		texture.texture_offset = data.write_multiple(texture.pixels);
	}
}

static Texture read_paletted_texture(Buffer data, Buffer palette, s32 width, s32 height) {
	Texture texture;
	texture.width = width;
	texture.height = height;
	texture.palette.top = 256;
	for(s32 i = 0; i < 256; i++) {
		texture.palette.colours[i] = palette.read<u32>(map_palette_index(i) * 4, "palette");
		u32 alpha = (texture.palette.colours[i] & 0xff000000) >> 24;
		alpha = std::min(alpha * 2, 0xffu);
		texture.palette.colours[i] = (texture.palette.colours[i] & 0x00ffffff) | (alpha << 24);
	}
	texture.pixels = data.read_multiple<u8>(0, width * height, "texture").copy();
	return texture;
}

void deduplicate_palettes(std::vector<Texture>& textures) {
	std::vector<size_t> mapping(textures.size());
	for(size_t i = 0; i < textures.size(); i++) {
		mapping[i] = i;
	}
	
	std::sort(BEGIN_END(mapping), [&](size_t lhs, size_t rhs) {
		return textures[lhs].palette.colours < textures[rhs].palette.colours;
	});
	
	std::vector<size_t> group{mapping[0]};
	auto merge_group = [&]() {
		size_t lowest = SIZE_MAX;
		for(size_t index : group) {
			lowest = std::min(lowest, index);
		}
		assert(lowest != SIZE_MAX);
		for(size_t index : group) {
			if(index != lowest) {
				textures[index].palette_out_edge = lowest;
			}
		}
	};
	
	for(size_t i = 1; i < textures.size(); i++) {
		Texture& last = textures[mapping[i - 1]];
		Texture& cur = textures[mapping[i]];
		if(!(last.palette == cur.palette)) {
			merge_group();
			group.clear();
		}
		group.push_back(mapping[i]);
	}
	if(group.size() > 0) {
		merge_group();
	}
}

s64 write_palette(OutBuffer dest, Palette& palette) {
	s64 ofs = dest.tell();
	for(s32 i = 0; i < 256; i++) {
		u32 colour = palette.colours[map_palette_index(i)];
		u32 alpha = (colour & 0xff000000) >> 24;
		if(alpha != 0xff) {
			alpha = alpha / 2;
		} else {
			alpha = 0x80;
		}
		dest.write((colour & 0x00ffffff) | (alpha << 24));
	}
	return ofs;
}

static u8 map_palette_index(u8 index) {
	// Swap middle two bits
	//  e.g. 00010000 becomes 00001000.
	return (((index & 16) >> 1) != (index & 8)) ? (index ^ 0b00011000) : index;
}
