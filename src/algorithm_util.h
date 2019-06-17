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

#ifndef ALGORITHM_UTIL_H
#define ALGORITHM_UTIL_H

#include <algorithm>

template <typename T_output, typename T_input, typename T_op>
T_output transform_all(T_input in, T_op f) {
	T_output result(std::distance(in.end() - in.begin()));
	std::transform(in.begin(), in.end(), result.begin(), f);
	return result;
}

#endif
