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
#include <engine/occlusion.h>
#include <core/filesystem.h>

TEST_CASE("Occlusion grid empty", "[engine]")
{
	std::vector<OcclusionOctant> input;
	std::vector<u8> buffer;
	write_occlusion_grid(buffer, input);
	std::vector<OcclusionOctant> output = read_occlusion_grid(buffer);
	REQUIRE(input.empty());
}

TEST_CASE("Occlusion grid round trip", "[engine]")
{
	std::vector<OcclusionOctant> input = {
		{1, 2, 3, {1, 2, 3}},
		{2, 3, 4, {1, 2, 3}},
		{2, 3, 5, {2, 4, 5}}
	};
	std::vector<u8> buffer;
	write_occlusion_grid(buffer, input);
	std::vector<OcclusionOctant> output = read_occlusion_grid(buffer);
	REQUIRE(input == output);
}

TEST_CASE("Occlusion octants round trip", "[engine]")
{
	std::vector<OcclusionVector> input = {
		{1, 2, 3},
		{4, 5, 6}
	};
	std::vector<u8> buffer;
	write_occlusion_octants(buffer, input);
	std::vector<OcclusionVector> output = read_occlusion_octants((char*) buffer.data());
	REQUIRE(input == output);
}
