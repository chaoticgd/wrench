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

#include "tfrag.h"
#include "../util.h"

tfrag::tfrag(stream *backing, std::size_t base_offset, uint16_t vertex_offset, uint16_t vertex_count)
		: _backing(backing, base_offset, 0), _vertex_offset(vertex_offset),
		  _vertex_count(vertex_count) {}

std::vector<float> tfrag::triangles() const {
	std::vector<float> result;

	for (std::size_t i = 0; i < _vertex_count; i++) {
		auto vertex = _backing.peek<fmt::vertex>(_vertex_offset + i * 0x10);
		result.emplace_back(vertex.x / (float) 1024);
		result.emplace_back(vertex.y / (float) 1024);
		result.emplace_back(vertex.z / (float) 1024);
	}

	return result;
}
