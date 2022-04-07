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

#ifndef EDITOR_UTIL_H
#define EDITOR_UTIL_H

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>

std::string int_to_hex(std::size_t x);
std::size_t hex_to_int(std::string x);

std::vector<std::string> to_hex_dump(uint32_t* data, std::size_t align, std::size_t size_in_u32s);

template <typename T>
bool contains(T container, const typename T::value_type& value) {
	return std::find(container.begin(), container.end(), value) != container.end();
}

int execute_command(std::string executable, std::vector<std::string> arguments);

#endif
