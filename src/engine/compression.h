/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2022 chaoticgd

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

#ifndef ENGINE_COMPRESSION_H
#define ENGINE_COMPRESSION_H

#include <map>
#include <vector>
#include <cstring>
#include <utility>
#include <cstdint>

// Decompress and recompress WAD segments used by the games to store various
// assets. Not to be confused with WAD archives.

// Check the magic bytes.
bool validate_wad(const uint8_t* magic);

struct WadBuffer
{
	WadBuffer(const uint8_t* p, const uint8_t* e) : ptr(p), end(e) {}
	WadBuffer(const std::vector<uint8_t>& vec) : ptr(vec.data()), end(vec.data() + vec.size()) {}
	
	const uint8_t* ptr;
	const uint8_t* end;
};

bool decompress_wad(std::vector<uint8_t>& dest, WadBuffer src);
void compress_wad(
	std::vector<uint8_t>& dest, const std::vector<uint8_t>& src, const char* muffin, int thread_count);

#endif
