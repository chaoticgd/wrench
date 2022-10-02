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

#ifndef CORE_TRISTRIP_H
#define CORE_TRISTRIP_H

#include <core/mesh.h>
#include <core/material.h>
#include <core/tristrip_packet.h>

// A unit of data that can be sent and processed on VU1 at a time.
struct TriStripPacket {
	s32 strip_begin = 0;
	s32 strip_count = 0;
};

// A single tristrip.
struct TriStrip {
	s32 index_begin = 0;
	s32 index_count = 0;
	s32 material = 0; // -1 for no change
};

struct TriStripPackets {
	std::vector<TriStripPacket> packets;
	std::vector<TriStrip> strips;
	std::vector<s32> indices;
};

struct TriStripConfig {
	TriStripConstraints constraints;
	bool support_instancing;
};

// Generates a set of tristrips that cover a given mesh.
TriStripPackets weave_tristrips(const Mesh& mesh, const std::vector<Material>& materials, const TriStripConfig& config);

#endif
