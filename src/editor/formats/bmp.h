/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019 chaoticgd

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

#ifndef FORMATS_BMP_H
#define FORMATS_BMP_H

#include "../stream.h"

# /*
#	Convert BMP files to and from textures.
# */

class texture;

struct bmp_file_header;
struct bmp_info_header;
struct bmp_colour_table_entry;

/*
	The BMP file format.
*/
packed_struct(bmp_file_header,
	char              magic[2]; // "BM"
	uint32_t          file_size;
	uint32_t          reserved;
	file_ptr<uint8_t> pixel_data;
)

packed_struct(bmp_info_header,
	uint32_t info_header_size; // 40
	int32_t  width;
	int32_t  height;
	int16_t  num_colour_planes; // Must be 1.
	int16_t  bits_per_pixel;
	uint32_t compression_method; // 0 = RGB
	uint32_t pixel_data_size;
	int32_t  horizontal_resolution; // Pixels per metre.
	int32_t  vertical_resolution; // Pixels per metre.
	uint32_t num_colours;
	uint32_t num_important_colours; // Usually zero.
)

packed_struct(bmp_colour_table_entry,
	uint8_t b;
	uint8_t g;
	uint8_t r;
	uint8_t pad;
)

bool validate_bmp(bmp_file_header header);
void texture_to_bmp(stream& dest, texture* src);
void bmp_to_texture(texture* dest, stream& src);

#endif
