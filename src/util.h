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

// Convert a container of owned pointers to a container of raw pointers.
template <typename T_result, typename T_param>
std::vector<T_result*> unique_to_raw(const std::vector<std::unique_ptr<T_param>>& x) {
	std::vector<T_result*> result(x.size());
	std::transform(x.begin(), x.end(), result.begin(),
		[](auto& ptr) { return ptr.get(); });
	return result;
}

#endif
