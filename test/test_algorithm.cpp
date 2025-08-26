/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2025 chaoticgd

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

#include <catch2/catch_amalgamated.hpp>
#include <core/algorithm.h>

TEST_CASE("mark_duplicates empty list", "[algorithm]")
{
	std::vector<s32> empty;
	bool marked = false;
	mark_duplicates(empty,
		[](s32 lhs, s32 rhs) { return memcmp(&lhs, &rhs, 4); },
		[&](s32 index, s32 canonical) { marked = true; });
	REQUIRE(!marked);
}
	
TEST_CASE("mark_duplicates integer list", "[algorithm]")
{
	std::vector<s32> list = {1, 2, 3, 2, 4, 4};
	std::vector<s32> mark(list.size());
	mark_duplicates(list,
		[](s32 lhs, s32 rhs) { return memcmp(&lhs, &rhs, 4); },
		[&](s32 index, s32 canonical) { mark[index] = canonical; });
	std::vector<s32> correct_mark = {0, 1, 2, 1, 4, 4};
	REQUIRE(mark == correct_mark);
}
