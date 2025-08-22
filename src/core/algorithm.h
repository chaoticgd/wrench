/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2023 chaoticgd

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

#ifndef CORE_ALGORITHM_H
#define CORE_ALGORITHM_H

#include <algorithm>

#include <core/util.h>

// Detects duplicate elements in a container. Assigns each element an index
// pointing to the "canonical" element. If lhs < rhs, the comparator function
// should return a negative number, and if lhs > rhs, it should return a
// non-zero positive function, otherwise it should return zero.
template <typename Container, typename CompareFunc, typename MarkFunc>
s32 mark_duplicates(Container& container, CompareFunc compare, MarkFunc mark)
{
	std::vector<s32> order(container.size());
	for(s32 i = 0; i < (s32) container.size(); i++)
		order[i] = i;
	std::sort(BEGIN_END(order), [&](s32 lhs, s32 rhs)
		{ return compare(container[lhs], container[rhs]) < 0; });
	s32 start_of_group = 0;
	s32 unique_element_count = 0;
	for(s32 i = 0; i < (s32) container.size(); i++) {
		if(i == (s32) container.size() - 1 || compare(container[order[i]], container[order[i + 1]]) != 0) {
			s32 min_index = INT32_MAX;
			for(s32 j = start_of_group; j <= i; j++)
				if(order[j] < min_index)
					min_index = order[j];
			for(s32 j = start_of_group; j <= i; j++)
				mark(order[j], min_index);
			start_of_group = i + 1;
		}
	}
	return unique_element_count;
}

#endif
