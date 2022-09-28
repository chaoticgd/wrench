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

// This models the limited maximum size of a given packet. For each constraint,
// the number of the given objects in a packet will be multiplied by their
// respective costs and these results will be summed. If the sum is greater than
// the max cost, the packet is too big so this can be used to reject changes to
// a packet e.g. by limiting the length of a strip.
struct TriStripConstraints {
	u8 num_constraints = 0;
	s32 constant_cost[4];
	s32 strip_cost[4];
	s32 vertex_cost[4];
	s32 index_cost[4];
	s32 material_cost[4];
	s32 max_cost[4];
};

// *****************************************************************************

// A unit of data that can be sent and processed on VU1 at a time.
struct TriStripPacket {
	s32 strip_begin;
	s32 strip_count;
};

// A single tristrip.
struct TriStrip {
	s32 index_begin;
	s32 index_count;
	s32 material; // -1 for no change
};

struct TriStripPackets {
	std::vector<TriStripPacket> packets;
	std::vector<TriStrip> strips;
	std::vector<s32> indices;
};

// Generates a set of tristrips that cover a given mesh.
TriStripPackets weave_tristrips(const Mesh& mesh, const TriStripConstraints& constraints, bool support_instancing);

#endif
