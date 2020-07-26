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

#ifndef UTIL_H
#define UTIL_H

#include <memory>
#include <string>
#include <vector>
#include <algorithm>

# /*
#	For things that should be in the standard library, but aren't.
# */

std::string int_to_hex(std::size_t x);
std::size_t hex_to_int(std::string x);
std::size_t parse_number(std::string x);

std::vector<std::string> to_hex_dump(uint32_t* data, std::size_t align, std::size_t size_in_u32s);

#define	MD5_DIGEST_LENGTH 16
std::string md5_to_printable_string(uint8_t in[MD5_DIGEST_LENGTH]);

template <typename T>
bool contains(T container, typename T::value_type value) {
	return std::find(container.begin(), container.end(), value) != container.end();
}

template <typename T>
struct array_view {
	T* begin_ptr;
	T* end_ptr;
	array_view() : begin_ptr(0), end_ptr(0) {}
	array_view(T* b, T* e) : begin_ptr(b), end_ptr(e) {}
	array_view(T* ptr, std::size_t size) : begin_ptr(ptr), end_ptr(ptr + size) {}
	array_view(std::vector<T>& vec) : begin_ptr(&(*vec.begin())), end_ptr(&(*vec.end())) {}
	T* begin() { return begin_ptr; }
	T* end() { return end_ptr; }
	T& at(std::size_t i) {
		if(begin_ptr + i < end_ptr) {
			return *(begin_ptr + i);
		 } else {
			throw std::runtime_error("array_view::at: Out of bounds access!");
		 }
	}
};

#endif
