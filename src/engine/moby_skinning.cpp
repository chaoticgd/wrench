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

#include "moby_skinning.h"

#include <set>
#include <engine/moby_packet.h>

namespace MOBY {

void prepare_skin_matrices(
	const std::vector<MobyMatrixTransfer>& preloop_matrix_transfers,
	Opt<SkinAttributes> blend_cache[64],
	bool animated)
{
	for (const MobyMatrixTransfer& transfer : preloop_matrix_transfers) {
		verify(transfer.vu0_dest_addr % 4 == 0, "Unaligned pre-loop joint address 0x%llx.", transfer.vu0_dest_addr);
		if (!animated && transfer.spr_joint_index == 0) {
			// If the mesh isn't animated, use the blend shape matrix (identity matrix).
			blend_cache[transfer.vu0_dest_addr / 0x4] = SkinAttributes{0, {0, 0, 0}, {0, 0, 0}};
		} else {
			blend_cache[transfer.vu0_dest_addr / 0x4] = SkinAttributes{1, {(s8) transfer.spr_joint_index, 0, 0}, {255, 0, 0}};
		}
		VERBOSE_SKINNING(printf("preloop upload spr[%02hhx] -> %02hhx\n", transfer.spr_joint_index, transfer.vu0_dest_addr));
	}
}

SkinAttributes read_skin_attributes(
	Opt<SkinAttributes> blend_buffer[64],
	const MobyVertex& mv,
	s32 ind,
	s32 two_way_count,
	s32 three_way_count)
{
	SkinAttributes attribs;
	
	auto load_skin_attribs = [&](u8 addr) {
		verify(addr % 4 == 0, "Unaligned VU0 matrix load address 0x%x.", addr);
		verify(blend_buffer[addr / 0x4].has_value(), "Matrix load from uninitialised VU0 address 0x%llx.", addr);
		return *blend_buffer[addr / 0x4];
	};
	
	auto store_skin_attribs = [&](u8 addr, SkinAttributes attribs) {
		verify(addr % 4 == 0, "Unaligned VU0 matrix store address 0x%x.", addr);
		blend_buffer[addr / 0x4] = attribs;
	};
	
	s8 bits_9_15 = (mv.v.i.low_halfword & 0b1111111000000000) >> 9;
	
	if (ind < two_way_count) {
		u8 transfer_addr = mv.v.two_way_blend.vu0_transferred_matrix_store_addr;
		s8 spr_joint_index = bits_9_15;
		store_skin_attribs(transfer_addr, SkinAttributes{1, {spr_joint_index, 0, 0}, {255, 0, 0}});
		
		verify(mv.v.two_way_blend.vu0_matrix_load_addr_1 != transfer_addr &&
			mv.v.two_way_blend.vu0_matrix_load_addr_2 != transfer_addr,
			"Loading from and storing to the same VU0 address (%02hhx) in the same loop iteration. "
			"Insomniac's exporter never does this.", transfer_addr);
		
		SkinAttributes src_1 = load_skin_attribs(mv.v.two_way_blend.vu0_matrix_load_addr_1);
		SkinAttributes src_2 = load_skin_attribs(mv.v.two_way_blend.vu0_matrix_load_addr_2);
		verify(src_1.count < 2 && src_2.count < 2, "Input to two-way matrix blend operation has already been blended.");
		
		u8 weight_1 = mv.v.two_way_blend.weight_1;
		u8 weight_2 = mv.v.two_way_blend.weight_2;
		
		u8 blend_addr = mv.v.two_way_blend.vu0_blended_matrix_store_addr;
		attribs = SkinAttributes{2, {src_1.joints[0], src_2.joints[0], 0}, {weight_1, weight_2, 0}};
		store_skin_attribs(blend_addr, attribs);
		
		if (transfer_addr != 0xf4) {
			VERBOSE_SKINNING(printf("upload spr[%02hhx] -> %02hhx, ", spr_joint_index, transfer_addr));
		} else {
			VERBOSE_SKINNING(printf("                      "));
		}
		
		if (blend_addr != 0xf4) {
			VERBOSE_SKINNING(printf("use blend(%02hhx,%02hhx;%02hhx,%02hhx;%02hhx,%02hhx) -> %02hhx\n",
				mv.v.two_way_blend.vu0_matrix_load_addr_1,
				mv.v.two_way_blend.vu0_matrix_load_addr_2,
				src_1.joints[0],
				src_2.joints[0],
				mv.v.two_way_blend.weight_1,
				mv.v.two_way_blend.weight_2,
				blend_addr));
		} else {
			VERBOSE_SKINNING(printf("use blend(%02hhx,%02hhx;%02hhx,%02hhx;%02hhx,%02hhx)\n",
				mv.v.two_way_blend.vu0_matrix_load_addr_1,
				mv.v.two_way_blend.vu0_matrix_load_addr_2,
				src_1.joints[0],
				src_2.joints[0],
				mv.v.two_way_blend.weight_1,
				mv.v.two_way_blend.weight_2));
		}
	} else if (ind < two_way_count + three_way_count) {
		s8 vu0_matrix_load_addr_3 = bits_9_15 * 2;
		SkinAttributes src_1 = load_skin_attribs(mv.v.three_way_blend.vu0_matrix_load_addr_1);
		SkinAttributes src_2 = load_skin_attribs(mv.v.three_way_blend.vu0_matrix_load_addr_2);
		SkinAttributes src_3 = load_skin_attribs(vu0_matrix_load_addr_3);
		verify(src_1.count < 2 && src_2.count < 2 && src_3.count < 2,
			"Input to three-way matrix blend operation has already been blended.");
		
		u8 weight_1 = mv.v.three_way_blend.weight_1;
		u8 weight_2 = mv.v.three_way_blend.weight_2;
		u8 weight_3 = mv.v.three_way_blend.weight_3;
		
		u8 blend_addr = mv.v.three_way_blend.vu0_blended_matrix_store_addr;
		attribs = SkinAttributes{3, {src_1.joints[0], src_2.joints[0], src_3.joints[0]}, {weight_1, weight_2, weight_3}};
		store_skin_attribs(blend_addr, attribs);
		
		if (blend_addr != 0xf4) {
			VERBOSE_SKINNING(printf("                      use blend(%02hhx,%02hhx,%02hhx;%02hhx,%02hhx,%02hhx;%02hhx,%02hhx,%02hhx) -> %02x\n",
				mv.v.three_way_blend.vu0_matrix_load_addr_1,
				mv.v.three_way_blend.vu0_matrix_load_addr_2,
				vu0_matrix_load_addr_3,
				src_1.joints[0],
				src_2.joints[0],
				src_3.joints[0],
				mv.v.three_way_blend.weight_1,
				mv.v.three_way_blend.weight_2,
				mv.v.three_way_blend.weight_3,
				blend_addr));
		} else {
			VERBOSE_SKINNING(printf("                      use blend(%02hhx,%02hhx,%02hhx;%02hhx,%02hhx,%02hhx;%02hhx,%02hhx,%02hhx)\n",
				mv.v.three_way_blend.vu0_matrix_load_addr_1,
				mv.v.three_way_blend.vu0_matrix_load_addr_2,
				vu0_matrix_load_addr_3,
				src_1.joints[0],
				src_2.joints[0],
				src_3.joints[0],
				mv.v.three_way_blend.weight_1,
				mv.v.three_way_blend.weight_2,
				mv.v.three_way_blend.weight_3));
		}
	} else {
		u8 transfer_addr = mv.v.regular.vu0_transferred_matrix_store_addr;
		s8 spr_joint_index = bits_9_15;
		store_skin_attribs(transfer_addr, SkinAttributes{1, {spr_joint_index, 0, 0}, {255, 0, 0}});
		
		verify(mv.v.regular.vu0_matrix_load_addr != transfer_addr,
			"Loading from and storing to the same VU0 address (%02hhx) in the same loop iteration. "
			"Insomniac's exporter never does this.", transfer_addr);
		
		attribs = load_skin_attribs(mv.v.regular.vu0_matrix_load_addr);
		
		if (transfer_addr != 0xf4) {
			VERBOSE_SKINNING(printf("upload spr[%02hhx] -> %02hhx, use %s %02hhx\n",
				spr_joint_index, transfer_addr,
				attribs.count > 1 ? "blended" : "joint",
				mv.v.regular.vu0_matrix_load_addr));
		} else {
			VERBOSE_SKINNING(printf("                      use %s %02hhx\n",
				attribs.count > 1 ? "blended" : "joint",
				mv.v.regular.vu0_matrix_load_addr));
		}
	}
	
	return attribs;
}

const Vertex& VertexLocation::find_vertex_in(const std::vector<GLTF::Mesh>& packets) const
{
	return packets[packet].vertices[vertex];
}

s32 max_num_joints_referenced_per_packet(const std::vector<GLTF::Mesh>& packets)
{
	// This seems suboptimal but it's what Insomniac did.
	s32 max_joints_per_packet = 0;
	for (size_t i = 0; i < packets.size(); i++) {
		const GLTF::Mesh& packet = packets[i];
		
		std::set<u8> joints;
		for (size_t j = 0; j < packet.vertices.size(); j++) {
			const Vertex& vertex = packet.vertices[j];
			for (s32 k = 0; k < vertex.skin.count; k++) {
				joints.emplace(vertex.skin.joints[k]);
			}
		}
		
		max_joints_per_packet = std::max(max_joints_per_packet, (s32) joints.size());
	}
	return max_joints_per_packet;
}

std::vector<std::vector<MatrixLivenessInfo>> compute_matrix_liveness(
	const std::vector<GLTF::Mesh>& packets)
{
	std::vector<VertexLocation> mapping;
	for (size_t i = 0; i < packets.size(); i++) {
		for (size_t j = 0; j < packets[i].vertices.size(); j++) {
			mapping.push_back(VertexLocation{i, j});
		}
	}
	
	std::sort(BEGIN_END(mapping), [&](const VertexLocation& l, const VertexLocation& r) {
		return l.find_vertex_in(packets).skin < r.find_vertex_in(packets).skin;
	});
	
	std::vector<std::vector<MatrixLivenessInfo>> liveness;
	for (const GLTF::Mesh& packet : packets) {
		liveness.emplace_back(packet.vertices.size(), MatrixLivenessInfo{});
	}
	
	auto process_run = [&](size_t begin, size_t end) {
		VertexLocation first_vertex = {SIZE_MAX, SIZE_MAX};
		s32 last_packet = -1;
		for (size_t j = begin; j < end; j++) {
			if (mapping[j].packet < first_vertex.packet) {
				first_vertex = mapping[j];
			}
			if (mapping[j].packet == first_vertex.packet && mapping[j].vertex < first_vertex.vertex) {
				first_vertex = mapping[j];
			}
			if ((s32) mapping[j].packet > last_packet) {
				last_packet = (s32) mapping[j].packet;
			}
		}
		verify_fatal(first_vertex.packet != SIZE_MAX);
		verify_fatal(first_vertex.vertex != SIZE_MAX);
		verify_fatal(last_packet != -1);
		liveness[first_vertex.packet][first_vertex.vertex].population_count = end - begin;
		for (size_t j = begin; j < end; j++) {
			liveness[mapping[j].packet][mapping[j].vertex].last_packet = last_packet;
			liveness[mapping[j].packet][mapping[j].vertex].first_vertex = first_vertex;
		}
	};
	
	size_t start_of_run = 0;
	for (size_t i = 1; i < mapping.size(); i++) {
		const Vertex& last = mapping[i - 1].find_vertex_in(packets);
		const Vertex& current = mapping[i].find_vertex_in(packets);
		if (!(current.skin == last.skin)) {
			process_run(start_of_run, i);
			start_of_run = i;
		}
	}
	if (start_of_run != mapping.size()) {
		process_run(start_of_run, mapping.size());
	}
	
	return liveness;
}


MatrixTransferSchedule schedule_matrix_transfers(
	s32 smi,
	const GLTF::Mesh& packet,
	VertexTable* last_packet,
	VU0MatrixAllocator& mat_alloc,
	const std::vector<MatrixLivenessInfo>& liveness)
{
	// Determine which slots in VU0 memory are in use by the previous packet
	// while we are trying to do transfers for the current packet.
	std::vector<bool> slots_in_use(0x40, false);
	if (last_packet != nullptr) {
		size_t i = 0;
		size_t regular_begin = last_packet->two_way_blend_vertex_count + last_packet->three_way_blend_vertex_count;
		for (size_t i = regular_begin; i < last_packet->vertices.size(); i++) {
			const MobyVertex& mv = last_packet->vertices[i];
			slots_in_use[mv.v.regular.vu0_matrix_load_addr / 0x4] = true;
		}
	}
	
	// Find all the joints that are used by this packet.
	std::set<u8> used_joints;
	std::vector<bool> joint_used_by_two_way_blends(256, false);
	for (size_t i = 0; i < packet.vertices.size(); i++) {
		const Vertex& vertex = packet.vertices[i];
		//if (vertex.skin.count == 1 || liveness[i].population_count > 0) {
		for (s32 j = 0; j < vertex.skin.count; j++) {
			u8 joint = (u8) vertex.skin.joints[j];
			if (vertex.skin.count == 2) {
				joint_used_by_two_way_blends[joint] = true;
			}
			used_joints.emplace(joint);
		}
	}
	
	//
	std::vector<u8> two_way_joints;
	std::set<u8> other_joints;
	for (auto iter = used_joints.rbegin(); iter != used_joints.rend(); iter++) {
		u8 joint = *iter;
		if (!joint_used_by_two_way_blends[joint]) {
			two_way_joints.push_back(joint);
		} else {
			other_joints.emplace(joint);
		}
	}
	std::reverse(BEGIN_END(two_way_joints));
	
	// Allocate space for most of the newly transferred matrices.
	std::vector<MobyMatrixTransfer> maybe_conflicting_matrix_transfers;
	std::vector<MobyMatrixTransfer> independent_matrix_transfers;
	for (u8 joint : other_joints) {
		Opt<u8> addr = mat_alloc.allocate_transferred(joint, "not two-way");
		if (addr.has_value()) {
			if (slots_in_use[*addr / 0x4]) {
				maybe_conflicting_matrix_transfers.push_back(MobyMatrixTransfer{joint, *addr});
			} else {
				independent_matrix_transfers.push_back(MobyMatrixTransfer{joint, *addr});
			}
		}
	}
	std::reverse(BEGIN_END(maybe_conflicting_matrix_transfers));
	
	// Put the maybe conflicting transfers first so there's less chance of
	// having conflicts.
	std::vector<MobyMatrixTransfer> matrix_transfers = std::move(maybe_conflicting_matrix_transfers);
	matrix_transfers.insert(matrix_transfers.end(), BEGIN_END(independent_matrix_transfers));
	
	MatrixTransferSchedule schedule;
	
	// Allocate space for the remaining transferred matrices.
	std::vector<MobyMatrixTransfer> allocated_two_way_transfers;
	for (u8 joint : two_way_joints) {
		Opt<u8> addr = mat_alloc.allocate_transferred(joint, "maybe two-way");
		if (addr.has_value()) {
			allocated_two_way_transfers.push_back(MobyMatrixTransfer{joint, *addr});
		}
	}
	
	// Allocate space for newly blended matrices.
	for (size_t i = 0; i < packet.vertices.size(); i++) {
		const Vertex& vertex = packet.vertices[i];
		if (vertex.skin.count > 1) {
			mat_alloc.allocate_blended(packet.vertices[i].skin, smi, liveness[i].last_packet, packet.vertices);
		}
	}
	
	auto xferlist = {
		allocated_two_way_transfers,
		maybe_conflicting_matrix_transfers,
		independent_matrix_transfers
	};
	
	// Count the number of two-way blends that will be issued for this packet.
	s32 two_way_count = 0;
	for (size_t i = 0; i < packet.vertices.size(); i++) {
		const Vertex& vertex = packet.vertices[i];
		if (vertex.skin.count == 2) {
			MatrixAllocation allocation;
			if (liveness[i].population_count != 1) {
				auto alloc_opt = mat_alloc.get_allocation_pre(vertex.skin);
				if (alloc_opt.has_value()) {
					allocation = *alloc_opt;
				}
			}
			if (allocation.first_use_pre) {
				two_way_count++;
			}
		}
	}
	
	if (last_packet != nullptr) {
		// Try to schedule as many matrix transfers as is possible given this
		// heuristic on the last packet.
		verify_fatal(last_packet->vertices.size() >= 1);
		s64 insert_index = (s64) last_packet->vertices.size() - 1 - schedule.last_packet_transfers.size();
		s64 last_three_way_end = last_packet->two_way_blend_vertex_count + last_packet->three_way_blend_vertex_count;
		for (MobyMatrixTransfer& transfer : matrix_transfers) {
			if (insert_index >= last_three_way_end) {
				bool conflict = false;
				for (size_t i = insert_index; i < last_packet->vertices.size(); i++) {
					const MobyVertex& mv = last_packet->vertices[i];
					if (mv.v.regular.vu0_matrix_load_addr == transfer.vu0_dest_addr) {
						conflict = true;
						break;
					}
				}
				
				if (!conflict) {
					schedule.last_packet_transfers.push_back(transfer);
					insert_index--;
				} else {
					schedule.preloop_transfers.push_back(transfer);
				}
			} else {
				schedule.preloop_transfers.emplace_back(transfer);
			}
		}
	} else {
		schedule.preloop_transfers.insert(schedule.preloop_transfers.end(), BEGIN_END(matrix_transfers));
	}
	
	// Schedule the two-way transfers and overflow to pre-loop transfers.
	s32 two_way_insert_index = 0;
	for (MobyMatrixTransfer& transfer : allocated_two_way_transfers) {
		if (two_way_insert_index < two_way_count) {
			schedule.two_way_transfers.push_back(transfer);
			two_way_insert_index++;
		} else {
			bool last_packet_has_space = last_packet != nullptr &&
				schedule.last_packet_transfers.size() < last_packet->main_vertex_count;
			if (last_packet_has_space && !slots_in_use[transfer.vu0_dest_addr / 0x4]) {
				schedule.last_packet_transfers.push_back(transfer);
			} else {
				schedule.preloop_transfers.push_back(transfer);
			}
		}
	}
	
	return schedule;
}

VU0MatrixAllocator::VU0MatrixAllocator(s32 max_joints_per_packet)
{
	first_blend_store_addr = max_joints_per_packet * 0x4;
	next_blend_store_addr = first_blend_store_addr;
	verify(first_blend_store_addr < 0xf4, "Failed to allocate transfer matrices in VU0 memory. Try simplifying your joint weights.");
}

void VU0MatrixAllocator::new_packet()
{
	next_blend_store_addr = first_blend_store_addr;
	transfer_allocations_this_packet = 0;
	blend_allocations_this_packet = 0;
	for (s32 i = 0; i < first_blend_store_addr; i += 0x4) {
		slots[i / 0x4].generation++;
	}
}

Opt<u8> VU0MatrixAllocator::allocate_transferred(u8 joint, const char* context)
{
	SkinAttributes attribs{1, {(s8) joint, 0, 0}, {255, 0, 0}};
	MatrixAllocation& allocation = allocations[attribs];
	if (allocation.generation != slots[allocation.address / 0x4].generation) {
		VERBOSE_MATRIX_ALLOCATION(printf("Alloc %s transferred matrix {%hhu,{%hhd,%hhd,%hhd},{%hhu,%hhu,%hhu}} -> %hhx\n",
			context, attribs.count, attribs.joints[0], attribs.joints[1], attribs.joints[2],
			attribs.weights[0], attribs.weights[1], attribs.weights[2], next_transfer_store_addr));
		
		MatrixSlot& slot = slots[next_transfer_store_addr / 0x4];
		slot.generation++;
		allocation = MatrixAllocation{next_transfer_store_addr, true, true, slot.generation};
		transfer_allocations_this_packet++;
		next_transfer_store_addr += 0x4;
		if (next_transfer_store_addr >= first_blend_store_addr) {
			next_transfer_store_addr = 0;
		}
		return allocation.address;
	}
	return std::nullopt;
}

void VU0MatrixAllocator::allocate_blended(
	SkinAttributes attribs, s32 current_packet, s32 last_packet, const std::vector<Vertex>& vertices)
{
	MatrixAllocation& allocation = allocations[attribs];
	if (allocation.generation != slots[allocation.address / 0x4].generation) {
		// Try to find a slot that isn't live.
		u8 first_addr = next_blend_store_addr;
		while (slots[next_blend_store_addr / 0x4].liveness >= current_packet) {
			next_blend_store_addr += 0x4;
			if (next_blend_store_addr >= 0xf4) {
				next_blend_store_addr = first_blend_store_addr;
			}
			if (next_blend_store_addr == first_addr) {
				// All the slots are live, try to pick one anyway.
				s32 liveness = -1;
				for (s32 i = first_blend_store_addr; i < 0xf4; i += 0x4) {
					MatrixSlot& slot = slots[i / 0x4];
					
					bool used_by_this_packet = false;
					for (const Vertex& vertex : vertices) {
						if (vertex.skin == slot.current_contents) {
							used_by_this_packet = true;
						}
					}
					
					if (slot.liveness > liveness && !used_by_this_packet) {
						// Make sure we're not writing over data that's going to
						// be needed for this packet.
						next_blend_store_addr = i;
						liveness = slot.liveness;
					}
				}
				if (liveness == -1) {
					allocations.erase(attribs);
					return;
				}
				break;
			}
		}
		
		VERBOSE_MATRIX_ALLOCATION(printf("Alloc blended matrix {%hhu,{%hhd,%hhd,%hhd},{%hhu,%hhu,%hhu}} -> %hhx (%d)\n",
			attribs.count, attribs.joints[0], attribs.joints[1], attribs.joints[2],
			attribs.weights[0], attribs.weights[1], attribs.weights[2], next_blend_store_addr, last_packet));
		
		MatrixSlot& slot = slots[next_blend_store_addr / 0x4];
		slot.generation++;
		slot.liveness = last_packet;
		slot.current_contents = attribs;
		allocation = MatrixAllocation{next_blend_store_addr, true, true, slot.generation};
		blend_allocations_this_packet++;
		next_blend_store_addr += 0x4;
		if (next_blend_store_addr >= 0xf4) {
			next_blend_store_addr = first_blend_store_addr;
		}
	}
}

Opt<MatrixAllocation> VU0MatrixAllocator::get_allocation(SkinAttributes attribs, s32 current_packet)
{
	auto iter = allocations.find(attribs);
	if (iter == allocations.end()) {
		return std::nullopt;
	}
	MatrixAllocation& allocation = iter->second;
	MatrixSlot& slot = slots[allocation.address / 0x4];
	verify(allocation.generation == slot.generation,
		"Failed to get address for matrix with joint weights {%hhu,{%hhd,%hhd,%hhd},{%hhu,%hhu,%hhu}}. Generations are %d and %d.",
			attribs.count, attribs.joints[0], attribs.joints[1], attribs.joints[2],
			attribs.weights[0], attribs.weights[1], attribs.weights[2],
			allocation.generation, slot.generation);
	verify(attribs.count == 1 || slot.liveness >= current_packet,
		"Bad liveness analysis (current packet is %d, max is %d).",
		current_packet, slot.liveness);
	MatrixAllocation copy = allocation;
	allocation.first_use = false;
	return copy;
}

Opt<MatrixAllocation> VU0MatrixAllocator::get_allocation_pre(SkinAttributes attribs)
{
	auto iter = allocations.find(attribs);
	if (iter == allocations.end()) {
		return std::nullopt;
	}
	MatrixAllocation copy = iter->second;
	iter->second.first_use_pre = false;
	return copy;
}

}
