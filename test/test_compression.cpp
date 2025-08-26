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

#include <core/util.h>
#include <engine/compression.h>

TEST_CASE("Compression and decompression yields same result", "[compression]")
{
	srand(time(NULL));
	
	s32 data_size = GENERATE(10, 100, 1000, 10000, 100000);
	
	std::vector<u8> uncompressed(data_size);
	uncompressed[0] = (u8) rand();
	for(s32 i = 1; i < data_size; i++) {
		// Make it more likely we'll get some match packets.
		uncompressed[i] = (rand() % 4 == 0) ? rand() : uncompressed[i - 1];
	}
	
	std::vector<u8> compressed;
	compress_wad(compressed, uncompressed, nullptr, 8);
	
	std::vector<u8> decompressed;
	REQUIRE(decompress_wad(decompressed, compressed));
	
	REQUIRE(decompressed == uncompressed);
}
