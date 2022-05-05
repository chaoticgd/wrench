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

#include <sstream>

const char* UTIL_ERROR_CONTEXT_STRING = "";

void assert_impl(const char* file, int line, const char* arg_str, bool condition) {
	if(!condition) {
		fprintf(stderr, "[%s:%d] assert%s: ", file, line, UTIL_ERROR_CONTEXT_STRING);
		fprintf(stderr, "%s", arg_str);
		fprintf(stderr, "\n");
		exit(1);
	}
}

std::string string_format(const char* format, va_list args) {
	static char buffer[16 * 1024];
	vsnprintf(buffer, 16 * 1024, format, args);
	return std::string(buffer);
}

std::string stringf(const char* format, ...) {
	va_list args;
	va_start(args, format);
	std::string string = string_format(format, args);
	va_end(args);
	return string;
}

static std::vector<std::string> error_context_stack;
static std::string error_context_alloc;

static void update_error_context() {
	error_context_alloc = "";
	for(auto& str : error_context_stack) {
		error_context_alloc += str;
	}
	UTIL_ERROR_CONTEXT_STRING = error_context_alloc.c_str();
}

ErrorContext::ErrorContext(const char* format, ...) {
	va_list args;
	va_start(args, format);
	error_context_stack.emplace_back(" " + string_format(format, args));
	va_end(args);
	update_error_context();
}

ErrorContext::~ErrorContext() {
	error_context_stack.pop_back();
	update_error_context();
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

std::size_t parse_number(std::string x) {
	std::stringstream ss;
	if(x.size() >= 2 && x[0] == '0' && x[1] == 'x') {
		ss << std::hex << x.substr(2);
	} else {
		ss << x;
	}
	std::size_t result;
	ss >> result;
	return result;
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

f32 lerp(f32 min, s32 max, f32 value) {
	return min + (max - min) * value;
}
