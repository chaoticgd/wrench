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
#include <core/gltf.h>
#include <core/filesystem.h>

static bool test_model_file(const char* path, bool print_diff);

TEST_CASE("read/write sample models", "[gltf]")
{
	CHECK(test_model_file("test/data/suzanne.glb", true));
	test_model_file("test/data/rigged_figure.glb", false);
}

static bool test_model_file(const char* path, bool print_diff)
{
	std::vector<u8> in = read_file(fs::path(std::string(path)));
	auto model = GLTF::read_glb(in);
	std::vector<u8> out = GLTF::write_glb(model);
	return diff_buffers(in, out, 0, DIFF_REST_OF_BUFFER, print_diff, nullptr);
}
