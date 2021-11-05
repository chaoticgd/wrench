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

#include "util.h"

std::string string_format(const char* format, va_list args) {
	static char buffer[16 * 1024];
	vsnprintf(buffer, 16 * 1024, format, args);
	return std::string(buffer);
}

u16 byte_swap_16(u16 val) {
	return (val >> 8) | (val << 8);
}

u32 byte_swap_32(u32 val) {
	u32 swapped = 0;
	swapped |= (val >> 24) & 0xff;
	swapped |= (val << 8) & 0xff0000;
	swapped |= (val >> 8) & 0xff00;
	swapped |= (val << 24) & 0xff000000;
	return swapped;
}

std::string md5_to_printable_string(uint8_t in[16]) {
	const char* HEX_DIGITS = "0123456789abcdef";
	std::string result;
	for(int i = 0; i < 16; i++) {
		result += HEX_DIGITS[(in[i] >> 4) & 0xf];
		result += HEX_DIGITS[in[i] & 0xf];
	}
	return result;
}
