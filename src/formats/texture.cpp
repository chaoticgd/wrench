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

#include "texture.h"

#include "../util.h" // int_to_hex
#include "../iso_stream.h"
#include "fip.h" // decode_palette_index

#ifdef WRENCH_EDITOR
void texture::upload_to_opengl() {
	std::vector<uint8_t> colour_data(pixels.size() * 4);
	for(std::size_t i = 0; i < pixels.size(); i++) {
		colour c = palette[pixels[i]];
		colour_data[i * 4] = c.r;
		colour_data[i * 4 + 1] = c.g;
		colour_data[i * 4 + 2] = c.b;
		colour_data[i * 4 + 3] = static_cast<int>(c.a) * 2 - 1;
	}
	
	glDeleteTextures(1, &opengl_texture.id);
	glGenTextures(1, &opengl_texture.id);
	glBindTexture(GL_TEXTURE_2D, opengl_texture.id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, size.x, size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, colour_data.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}
#endif

int remap_pixel_index_rac4(int i, int width) {
	int s = i / (width * 2);
	int r = 0;
	if (s % 2 == 0)
		r = s * 2;
	else
		r = (s - 1) * 2 + 1;

	int q = ((i % (width * 2)) / 32);

	int m = i % 4;
	int n = (i / 4) % 4;
	int o = i % 2;
	int p = (i / 16) % 2;

	if ((s / 2) % 2 == 1)
		p = 1 - p;

	if (o == 0)
		m = (m + p) % 4;
	else
		m = ((m - p) + 4) % 4;


	int x = n + ((m + q * 4) * 4);
	int y = r + (o * 2);

	return (x%width) + (y * width);
}

texture create_texture_from_streams_rac4(vec2i size, stream* pixel_src, size_t pixel_offset, stream* palette_src, size_t palette_offset) {
	texture result;
	int buffer_size = size.x * size.y;
	result.size = size;
	result.pixels.resize(buffer_size);
	pixel_src->seek(pixel_offset);
	if (size.x >= 32 && size.y >= 4) {
		for (int i = 0; i < buffer_size; ++i) {
			int map = remap_pixel_index_rac4(i, size.x);
			if (map >= buffer_size) {
				map = buffer_size - 1;
			}
			result.pixels[map] = pixel_src->read<uint8_t>();
		}
	}
	else {
		pixel_src->read_v(result.pixels);
	}
	colour temp_palette[256];
	palette_src->seek(palette_offset);
	palette_src->read_n((char*)temp_palette, sizeof(temp_palette));
	for (int i = 0; i < 256; i++) {
		result.palette[i] = temp_palette[decode_palette_index(i)];
	}
	return result;
}

texture create_texture_from_streams(vec2i size, stream* pixel_src, size_t pixel_offset, stream* palette_src, size_t palette_offset) {
	texture result;
	int buffer_size = size.x * size.y;
	result.size = size;
	result.pixels.resize(buffer_size);
	pixel_src->seek(pixel_offset);
	pixel_src->read_v(result.pixels);
	colour temp_palette[256];
	palette_src->seek(palette_offset);
	palette_src->read_n((char*) temp_palette, sizeof(temp_palette));
	for(int i = 0; i < 256; i++) {
		result.palette[i] = temp_palette[decode_palette_index(i)];
	}
	return result;
}

std::optional<texture> create_fip_texture(stream* backing, std::size_t offset) {
	fip_header header = backing->peek<fip_header>(offset);
	if(!validate_fip(header.magic)) {
		return {};
	}
	vec2i size { header.width, header.height };
	std::size_t pixel_offset = offset + sizeof(fip_header);
	std::size_t palette_offset = offset + offsetof(fip_header, palette);
	size_t pos = backing->tell();
	texture tex = create_texture_from_streams(size, backing, pixel_offset, backing, palette_offset);
	backing->seek(pos);
	return std::move(tex);
}

std::vector<texture> read_pif_list(stream* backing, std::size_t offset) {
	uint32_t count = backing->read<uint32_t>(0);
	
	std::vector<uint32_t> offsets(count);
	backing->read_v(offsets);
	
	std::vector<texture> textures;
	for(uint32_t texture_offset : offsets) {
		auto texture = create_fip_texture(backing, offset + texture_offset);
		if(texture) {
			textures.emplace_back(std::move(*texture));
		}
	}
	
	return textures;
}
