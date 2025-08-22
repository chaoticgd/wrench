/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2022 chaoticgd

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

#include "basic_util.h"

#include <sstream>

std::string string_format(const char* format, va_list args)
{
	static char buffer[16 * 1024];
	vsnprintf(buffer, 16 * 1024, format, args);
	return std::string(buffer);
}

std::string stringf(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	std::string string = string_format(format, args);
	va_end(args);
	return string;
}

u16 byte_swap_16(u16 val)
{
	return (val >> 8) | (val << 8);
}

u32 byte_swap_32(u32 val)
{
	u32 swapped = 0;
	swapped |= (val >> 24) & 0xff;
	swapped |= (val << 8) & 0xff0000;
	swapped |= (val >> 8) & 0xff00;
	swapped |= (val << 24) & 0xff000000;
	return swapped;
}

std::size_t parse_number(std::string x)
{
	std::stringstream ss;
	if (x.size() >= 2 && x[0] == '0' && x[1] == 'x') {
		ss << std::hex << x.substr(2);
	} else {
		ss << x;
	}
	std::size_t result;
	ss >> result;
	return result;
}

std::string md5_to_printable_string(uint8_t in[16])
{
	const char* HEX_DIGITS = "0123456789abcdef";
	std::string result;
	for (int i = 0; i < 16; i++) {
		result += HEX_DIGITS[(in[i] >> 4) & 0xf];
		result += HEX_DIGITS[in[i] & 0xf];
	}
	return result;
}

f32 lerp(f32 min, s32 max, f32 value)
{
	return min + (max - min) * value;
}

s32 bit_range(u64 val, s32 lo, s32 hi)
{
	return (val >> lo) & ((1 << (hi - lo + 1)) - 1);
}

std::string to_snake_case(const char* src)
{
	std::string result;
	size_t size = strlen(src);
	for (size_t i = 0; i < size; i++) {
		if (src[i] == ' ') {
			result += '_';
		} else if (isalnum(src[i])) {
			result += tolower(src[i]);
		}
	}
	return result;
}

s32 align32(s32 value, s32 alignment)
{
	if (value % alignment != 0) {
		value += alignment - (value % alignment);
	}
	return value;
}

s64 align64(s64 value, s64 alignment)
{
	if (value % alignment != 0) {
		value += alignment - (value % alignment);
	}
	return value;
}

bool find_case_insensitive_substring(const char* haystack, const char* needle)
{
	size_t haystack_size = strlen(haystack);
	size_t needle_size = strlen(needle);
	for (size_t i = 0; i < haystack_size - needle_size + 1; i++) {
		size_t j;
		for (j = 0; j < needle_size; j++) {
			if (toupper(needle[j]) != toupper(haystack[i + j])) {
				break;
			}
		}
		if (j == needle_size) {
			return true;
		}
	}
	return false;
}
