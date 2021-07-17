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

#include "bmp.h"

#include <cstring>

#include "texture.h"

bool validate_bmp(bmp_file_header header) {
	return std::memcmp(header.magic, "BM", 2) == 0;
}

void texture_to_bmp(stream& dest, texture* src) {
	auto size = src->size;

	bmp_file_header header;
	std::memcpy(header.magic, "BM", 2);
	header.pixel_data =
		sizeof(bmp_file_header) +
		sizeof(bmp_info_header) +
		sizeof(bmp_colour_table_entry) * 256;
	header.file_size =
		header.pixel_data.value +
		size.x * size.y;
	header.reserved = 0x3713;
	dest.write<bmp_file_header>(0, header);

	bmp_info_header info;
	info.info_header_size      = 40;
	info.width                 = size.x;
	info.height                = size.y;
	info.num_colour_planes     = 1;
	info.bits_per_pixel        = 8;
	info.compression_method    = 0;
	info.pixel_data_size       = info.width * info.height;
	info.horizontal_resolution = 0;
	info.vertical_resolution   = 0;
	info.num_colours           = 256;
	info.num_important_colours = 0;
	dest.write<bmp_info_header>(info);

	for(int i = 0; i < 256; i++) {
		bmp_colour_table_entry pixel;
		pixel.b = src->palette[i].b;
		pixel.g = src->palette[i].g;
		pixel.r = src->palette[i].r;
		pixel.pad = 0;
		dest.write<bmp_colour_table_entry>(pixel);
	}

	uint32_t row_size = ((info.bits_per_pixel * info.width + 31) / 32) * 4;
	for(int y = info.height - 1; y >= 0; y--) {
		dest.write_n(reinterpret_cast<char*>(src->pixels.data()) + y * row_size, row_size);
	}
}


void bmp_to_texture(texture* dest, stream& src) {
	auto file_header = src.read<bmp_file_header>(0);

	if(!validate_bmp(file_header)) {
		throw stream_format_error("Invalid BMP header.");
	}

	uint32_t secondary_header_offset = src.tell();
	auto info_header = src.read<bmp_info_header>();

	if(info_header.bits_per_pixel != 8) {
		throw stream_format_error("The BMP file must use indexed colour (with at most 256 colours).");
	}

	if(info_header.num_colours > 256) {
		throw stream_format_error("The BMP colour palette must contain at most 256 colours.");
	}

	vec2i size {
		(std::size_t) std::abs(info_header.width),
		(std::size_t) std::abs(info_header.height)
	};
	if(dest->size != size) {
		throw stream_format_error("Texture size mismatch.");
	}

	// Some BMP files have a larger header.
	src.seek(secondary_header_offset + info_header.info_header_size);
	
	uint32_t i;
	for(i = 0; i < info_header.num_colours; i++) {
		auto src_pixel = src.read<bmp_colour_table_entry>();
		dest->palette[i] = { src_pixel.r, src_pixel.g, src_pixel.b, 0x80 };
	}
	for(; i < 256; i++) {
		// Set unused palette entries to black.
		dest->palette[i] = { 0, 0, 0, 0x80 };
	}

	uint32_t row_size = ((info_header.bits_per_pixel * info_header.width + 31) / 32) * 4;
	std::vector<uint8_t> pixels(info_header.width * info_header.height);
	for(int y = info_header.height - 1; y >= 0; y--) {
		src.read_n(reinterpret_cast<char*>(pixels.data()) + y * row_size, row_size);
	}
	dest->pixels = pixels;
}
