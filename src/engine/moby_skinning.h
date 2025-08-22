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

#ifndef ENGINE_MOBY_SKINNING_H
#define ENGINE_MOBY_SKINNING_H

#include <map>
#include <core/gltf.h>
#include <engine/moby_vertex.h>

#define VERBOSE_SKINNING(...) //__VA_ARGS__
#define VERBOSE_MATRIX_ALLOCATION(...) //__VA_ARGS__

namespace MOBY {

void prepare_skin_matrices(const std::vector<MobyMatrixTransfer>& preloop_matrix_transfers, Opt<SkinAttributes> blend_cache[64], bool animated);
SkinAttributes read_skin_attributes(Opt<SkinAttributes> blend_buffer[64], const MobyVertex& mv, s32 ind, s32 two_way_count, s32 three_way_count);

struct MatrixAllocation
{
	u8 address = 0;
	bool first_use = true;
	bool first_use_pre = true;
	s32 generation = -1;
};

struct MatrixSlot
{
	s32 generation = 0;
	s32 liveness = -1;
	SkinAttributes current_contents;
};

struct VertexLocation
{
	size_t packet;
	size_t vertex;
	
	const Vertex& find_vertex_in(const std::vector<GLTF::Mesh>& packets) const;
};

struct MatrixLivenessInfo
{
	s32 population_count = 0;
	s32 last_packet = -1;
	VertexLocation first_vertex;
};

class VU0MatrixAllocator
{
	std::map<SkinAttributes, MatrixAllocation> allocations;
	std::array<MatrixSlot, 0x40> slots;
	u8 first_transfer_store_addr = 0x0;
	u8 next_transfer_store_addr = 0x0;
	u8 first_blend_store_addr;
	u8 next_blend_store_addr;
	s32 transfer_allocations_this_packet = 0;
	s32 blend_allocations_this_packet = 0;
public:
	VU0MatrixAllocator(s32 max_joints_per_packet);
	void new_packet();
	Opt<u8> allocate_transferred(u8 joint, const char* context);
	void allocate_blended(SkinAttributes attribs, s32 current_packet, s32 last_packet, const std::vector<Vertex>& vertices);
	Opt<MatrixAllocation> get_allocation(SkinAttributes attribs, s32 current_packet);
	Opt<MatrixAllocation> get_allocation_pre(SkinAttributes attribs);
};

s32 max_num_joints_referenced_per_packet(const std::vector<GLTF::Mesh>& packets);
std::vector<std::vector<MatrixLivenessInfo>> compute_matrix_liveness(const std::vector<GLTF::Mesh>& packets);
struct MatrixTransferSchedule
{
	std::vector<MobyMatrixTransfer> last_packet_transfers;
	std::vector<MobyMatrixTransfer> preloop_transfers;
	std::vector<MobyMatrixTransfer> two_way_transfers;
};
MatrixTransferSchedule schedule_matrix_transfers(
	s32 smi,
	const GLTF::Mesh& packet,
	MobyPacketLowLevel* last_packet,
	VU0MatrixAllocator& mat_alloc,
	const std::vector<MatrixLivenessInfo>& liveness);
s32 max_num_joints_referenced_per_packet(const std::vector<GLTF::Mesh>& packets);
std::vector<std::vector<MatrixLivenessInfo>> compute_matrix_liveness(const std::vector<GLTF::Mesh>& packets);

}

#endif
