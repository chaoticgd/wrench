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

#include "fip.h"

#include <cstring>
#include <stdexcept>

#include "bmp.h"

bool validate_fip(char* magic) {
	return std::memcmp(magic, "2FIP", 4) == 0;
}

void fip_to_bmp(stream& dest, stream& src) {

	auto src_header = src.read<fip_header>(0);
	if(!validate_fip(src_header.magic)) {
		throw stream_format_error("Tried to read invalid FIP segment.");
	}

	bmp_file_header header;
	std::memcpy(header.magic, "BM", 2);
	header.pixel_data =
		sizeof(bmp_file_header) +
		sizeof(bmp_info_header) +
		sizeof(bmp_colour_table_entry) * 256;
	header.file_size =
		header.pixel_data.value +
		src_header.width * src_header.height;
	header.reserved = 1337;
	dest.write<bmp_file_header>(0, header);

	bmp_info_header info;
	info.info_header_size      = 40;
	info.width                 = src_header.width;
	info.height                = src_header.height;
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
		auto src_pixel = src_header.palette[i];
		bmp_colour_table_entry pixel;
		pixel.b = src_pixel.b;
		pixel.g = src_pixel.g;
		pixel.r = src_pixel.r;
		pixel.pad = 0;
		dest.write<bmp_colour_table_entry>(pixel);
	}

	uint32_t row_size = ((info.bits_per_pixel * info.width + 31) / 32) * 4;

	uint32_t colour_table = dest.tell();

	for(int y = info.height - 1; y >= 0; y--) {
		dest.seek(colour_table + y * row_size);
		for(int x = 0; x < info.width; x++) {
			uint8_t palette_index = src.read<uint8_t>();
			// Swap middle two bits
			//  e.g. 00010000 becomes 00001000.
			int a = palette_index & 8;
			int b = palette_index & 16;
			if(a && !b) {
				palette_index += 8;
			} else if(!a && b) {
				palette_index -= 8;
			}
			dest.write<uint8_t>(palette_index);
		}
	}
}
