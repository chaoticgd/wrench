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

#ifndef FORMATS_FIP_H
#define FORMATS_FIP_H

#include "../stream.h"

# /*
#	Convert between BMP files and indexed 2FIP textures (used to store GUI images).
# */

struct fip_header;
struct fip_palette_entry;

packed_struct(fip_palette_entry,
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t a;
)

packed_struct(fip_header,
	char              magic[4]; // "2FIP"
	uint8_t           unknown1[0x4];
	uint32_t          width;
	uint32_t          height;
	uint8_t           unknown2[0x10];
	fip_palette_entry palette[0x100];
	uint8_t           data[0];
)

bool validate_fip(char* header);
void fip_to_bmp(stream& dest, stream& src);
void bmp_to_fip(stream& dest, stream& src);
uint8_t decode_palette_index(uint8_t index);

#endif
