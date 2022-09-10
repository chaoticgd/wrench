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

#include "moby_mesh.h"

#include <set>
#include <core/vif.h>

#define VERBOSE_MATRIX_ALLOCATION(...) //__VA_ARGS__

struct MatrixAllocation {
	u8 address = 0;
	bool first_use = true;
	bool first_use_pre = true;
	s32 generation = -1;
};

struct MatrixSlot {
	s32 generation = 0;
	s32 liveness = -1;
	SkinAttributes current_contents;
};

struct VertexLocation {
	size_t submesh;
	size_t vertex;
	
	const Vertex& find_vertex_in(const std::vector<MobySubMesh>& submeshes) const {
		return submeshes[submesh].vertices[vertex];
	}
};

struct MatrixLivenessInfo {
	s32 population_count = 0;
	s32 last_submesh = -1;
	VertexLocation first_vertex;
};

class VU0MatrixAllocator {
	std::map<SkinAttributes, MatrixAllocation> allocations;
	std::array<MatrixSlot, 0x40> slots;
	u8 first_transfer_store_addr = 0x0;
	u8 next_transfer_store_addr = 0x0;
	u8 first_blend_store_addr;
	u8 next_blend_store_addr;
	s32 transfer_allocations_this_submesh = 0;
	s32 blend_allocations_this_submesh = 0;
public:
	VU0MatrixAllocator(s32 max_joints_per_submesh);
	void new_submesh();
	Opt<u8> allocate_transferred(u8 joint, const char* context);
	void allocate_blended(SkinAttributes attribs, s32 current_submesh, s32 last_submesh, const std::vector<Vertex>& vertices);
	Opt<MatrixAllocation> get_allocation(SkinAttributes attribs, s32 current_submesh);
	Opt<MatrixAllocation> get_allocation_pre(SkinAttributes attribs);
};

// write_moby_submeshes
// write_moby_metal_submeshes
static s64 write_shared_moby_vif_packets(OutBuffer dest, GifUsageTable* gif_usage, const MobySubMeshBase& submesh, s64 class_header_ofs);
struct MatrixTransferSchedule {
	std::vector<MobyMatrixTransfer> last_submesh_transfers;
	std::vector<MobyMatrixTransfer> preloop_transfers;
	std::vector<MobyMatrixTransfer> two_way_transfers;
};
static MatrixTransferSchedule schedule_matrix_transfers(s32 smi, const MobySubMesh& submesh, MobySubMeshLowLevel* last_submesh, VU0MatrixAllocator& mat_alloc, const std::vector<MatrixLivenessInfo>& liveness);
static MobySubMeshLowLevel pack_vertices(s32 smi, const MobySubMesh& submesh, VU0MatrixAllocator& mat_alloc, const std::vector<MatrixLivenessInfo>& liveness, f32 scale);
static void pack_common_attributes(MobyVertex& dest, const Vertex& src, f32 inverse_scale);
static s32 max_num_joints_referenced_per_submesh(const std::vector<MobySubMesh>& submeshes);
static std::vector<std::vector<MatrixLivenessInfo>> compute_matrix_liveness(const std::vector<MobySubMesh>& submeshes);
static MobyVertexTableHeaderRac1 write_vertices(OutBuffer& dest, const MobySubMesh& submesh, const MobySubMeshLowLevel& low, MobyFormat format);
// build_moby_submeshes
struct IndexMappingRecord {
	s32 submesh = -1;
	s32 index = -1; // The index of the vertex in the vertex table.
	s32 id = -1; // The index of the vertex in the intermediate buffer.
	s32 dedup_out_edge = -1; // If this vertex is a duplicate, this points to the canonical vertex.
};
static void find_duplicate_vertices(std::vector<IndexMappingRecord>& index_mapping, const std::vector<Vertex>& vertices);

void write_moby_submeshes(OutBuffer dest, GifUsageTable& gif_usage, s64 table_ofs, const MobySubMesh* submeshes_in, size_t submesh_count, f32 scale, MobyFormat format, s64 class_header_ofs) {
	static const s32 ST_UNPACK_ADDR_QUADWORDS = 0xc2;
	
	// TODO: Make it so we don't have to copy the submeshes here.
	std::vector<MobySubMesh> submeshes;
	for(size_t i = 0; i < submesh_count; i++) {
		submeshes.push_back(submeshes_in[i]);
	}
	
	// Fixup joint indices.
	for(MobySubMesh& submesh : submeshes) {
		for(Vertex& vertex : submesh.vertices) {
			for(s32 i = 0; i < vertex.skin.count; i++) {
				if(vertex.skin.joints[i] == -1) {
					vertex.skin.joints[i] = 0;
				}
			}
		}
	}
	
	s32 max_joints_per_submesh = max_num_joints_referenced_per_submesh(submeshes);
	
	std::vector<std::vector<MatrixLivenessInfo>> liveness = compute_matrix_liveness(submeshes);
	assert(liveness.size() == submeshes.size());
	
	std::vector<MobySubMeshLowLevel> low_submeshes;
	MobySubMeshLowLevel* last = nullptr;
	VU0MatrixAllocator matrix_allocator(max_joints_per_submesh);
	for(size_t i = 0; i < submeshes.size(); i++) {
		VERBOSE_MATRIX_ALLOCATION(printf("**** submesh %d ****\n", i));
		matrix_allocator.new_submesh();
		MatrixTransferSchedule schedule = schedule_matrix_transfers((s32) i, submeshes[i], last, matrix_allocator, liveness[i]);
		MobySubMeshLowLevel low = pack_vertices((s32) i, submeshes[i], matrix_allocator, liveness[i], scale);
		
		// Write the scheduled transfers.
		assert((last == nullptr && schedule.last_submesh_transfers.size() == 0) ||
			(last != nullptr && schedule.last_submesh_transfers.size() <= last->main_vertex_count));
		for(size_t i = 0; i < schedule.last_submesh_transfers.size(); i++) {
			assert(last != nullptr);
			MobyVertex& mv = last->vertices.at(last->vertices.size() - i - 1);
			MobyMatrixTransfer& transfer = schedule.last_submesh_transfers[i];
			mv.v.regular.low_halfword |= transfer.spr_joint_index << 9;
			mv.v.regular.vu0_transferred_matrix_store_addr = transfer.vu0_dest_addr;
		}
		low.preloop_matrix_transfers = std::move(schedule.preloop_transfers);
		assert(schedule.two_way_transfers.size() <= low.two_way_blend_vertex_count);
		for(size_t i = 0; i < schedule.two_way_transfers.size(); i++) {
			MobyVertex& mv = low.vertices.at(i);
			MobyMatrixTransfer& transfer = schedule.two_way_transfers[i];
			mv.v.two_way_blend.low_halfword |= transfer.spr_joint_index << 9;
			mv.v.two_way_blend.vu0_transferred_matrix_store_addr = transfer.vu0_dest_addr;
		}
		
		// The vertices are reordered while being packed.
		map_indices(submeshes[i], low.index_mapping);
		
		low_submeshes.emplace_back(std::move(low));
		last = &low_submeshes.back();
	}
	
	for(const MobySubMeshLowLevel& low : low_submeshes) {
		const MobySubMesh& submesh = low.high_level;
		MobySubMeshEntry entry = {0};
		
		// Write VIF command list.
		dest.pad(0x10);
		s64 vif_list_ofs = dest.tell();
		entry.vif_list_offset = vif_list_ofs - class_header_ofs;
		
		VifPacket st_unpack;
		st_unpack.code.interrupt = 0;
		st_unpack.code.cmd = (VifCmd) 0b1110000; // UNPACK
		st_unpack.code.num = submesh.sts.size();
		st_unpack.code.unpack.vnvl = VifVnVl::V2_16;
		st_unpack.code.unpack.flg = VifFlg::USE_VIF1_TOPS;
		st_unpack.code.unpack.usn = VifUsn::SIGNED;
		st_unpack.code.unpack.addr = ST_UNPACK_ADDR_QUADWORDS;
		st_unpack.data.resize(submesh.sts.size() * 4);
		memcpy(st_unpack.data.data(), submesh.sts.data(), submesh.sts.size() * 4);
		write_vif_packet(dest, st_unpack);
		
		s64 tex_unpack = write_shared_moby_vif_packets(dest, &gif_usage, submesh, class_header_ofs);
		
		entry.vif_list_texture_unpack_offset = tex_unpack;
		dest.pad(0x10);
		entry.vif_list_size = (dest.tell() - vif_list_ofs) / 0x10;
		
		s64 vertex_header_ofs = dest.tell();
		
		auto vertex_header = write_vertices(dest, submesh, low, format);
		
		entry.vertex_offset = vertex_header_ofs - class_header_ofs;
		dest.pad(0x10);
		entry.vertex_data_size = (dest.tell() - vertex_header_ofs) / 0x10;
		entry.unknown_d = (0xf + vertex_header.transfer_vertex_count * 6) / 0x10;
		entry.unknown_e = (3 + vertex_header.transfer_vertex_count) / 4;
		entry.transfer_vertex_count = vertex_header.transfer_vertex_count;
		
		dest.pad(0x10);
		dest.write(table_ofs, entry);
		table_ofs += 0x10;
	}
}

void write_moby_metal_submeshes(OutBuffer dest, s64 table_ofs, const std::vector<MobyMetalSubMesh>& submeshes, s64 class_header_ofs) {
	for(const MobyMetalSubMesh& submesh : submeshes) {
		MobySubMeshEntry entry = {0};
		
		// Write VIF command list.
		dest.pad(0x10);
		s64 vif_list_ofs = dest.tell();
		entry.vif_list_offset = vif_list_ofs - class_header_ofs;
		s64 tex_unpack = write_shared_moby_vif_packets(dest, nullptr, submesh, class_header_ofs);
		entry.vif_list_texture_unpack_offset = tex_unpack;
		dest.pad(0x10);
		entry.vif_list_size = (dest.tell() - vif_list_ofs) / 0x10;
		
		// Write vertex table.
		MobyMetalVertexTableHeader vertex_header;
		vertex_header.vertex_count = submesh.vertices.size();
		vertex_header.unknown_4 = submesh.unknown_4;
		vertex_header.unknown_8 = submesh.unknown_8;
		vertex_header.unknown_c = submesh.unknown_c;
		s64 vertex_header_ofs = dest.write(vertex_header);
		dest.write_multiple(submesh.vertices);
		entry.vertex_offset = vertex_header_ofs - class_header_ofs;
		dest.pad(0x10);
		entry.vertex_data_size = (dest.tell() - vertex_header_ofs) / 0x10;
		entry.unknown_d = (0xf + vertex_header.vertex_count * 6) / 0x10;
		entry.unknown_e = (3 + vertex_header.vertex_count) / 4;
		entry.transfer_vertex_count = vertex_header.vertex_count;
		
		dest.write(table_ofs, entry);
		table_ofs += 0x10;
	}
}

static s64 write_shared_moby_vif_packets(OutBuffer dest, GifUsageTable* gif_usage, const MobySubMeshBase& submesh, s64 class_header_ofs) {
	static const s32 INDEX_UNPACK_ADDR_QUADWORDS = 0x12d;
	
	std::vector<u8> indices;
	OutBuffer index_buffer(indices);
	s64 index_header_ofs = index_buffer.alloc<MobyIndexHeader>();
	index_buffer.write_multiple(submesh.indices);
	
	MobyIndexHeader index_header = {0};
	index_header.unknown_0 = submesh.index_header_first_byte;
	if(submesh.textures.size() > 0) {
		index_header.texture_unpack_offset_quadwords = indices.size() / 4;
	}
	if(submesh.secret_indices.size() >= 1) {
		index_header.secret_index = submesh.secret_indices[0];
	}
	index_buffer.write(index_header_ofs, index_header);
	
	VifPacket index_unpack;
	index_unpack.code.interrupt = 0;
	index_unpack.code.cmd = (VifCmd) 0b1100000; // UNPACK
	index_unpack.code.num = indices.size() / 4;
	index_unpack.code.unpack.vnvl = VifVnVl::V4_8;
	index_unpack.code.unpack.flg = VifFlg::USE_VIF1_TOPS;
	index_unpack.code.unpack.usn = VifUsn::SIGNED;
	index_unpack.code.unpack.addr = INDEX_UNPACK_ADDR_QUADWORDS;
	index_unpack.data = std::move(indices);
	write_vif_packet(dest, index_unpack);
	
	s64 rel_texture_unpack_ofs = 0;
	if(submesh.textures.size() > 0) {
		while(dest.tell() % 0x10 != 0xc) {
			dest.write<u8>(0);
		}
		
		VifPacket texture_unpack;
		texture_unpack.code.interrupt = 0;
		texture_unpack.code.cmd = (VifCmd) 0b1100000; // UNPACK
		texture_unpack.code.num = submesh.textures.size() * 4;
		texture_unpack.code.unpack.vnvl = VifVnVl::V4_32;
		texture_unpack.code.unpack.flg = VifFlg::USE_VIF1_TOPS;
		texture_unpack.code.unpack.usn = VifUsn::SIGNED;
		texture_unpack.code.unpack.addr = INDEX_UNPACK_ADDR_QUADWORDS + index_unpack.code.num;
		
		assert(submesh.secret_indices.size() >= submesh.textures.size());
		for(size_t i = 0; i < submesh.textures.size(); i++) {
			MobyTexturePrimitive primitive = submesh.textures[i];
			OutBuffer(texture_unpack.data).write(primitive);
		}
		for(size_t i = 1; i < submesh.secret_indices.size(); i++) {
			OutBuffer(texture_unpack.data).write((i - 1) * 0x10 + 0xc, submesh.secret_indices[i]);
		}
		s32 abs_texture_unpack_ofs = dest.tell();
		write_vif_packet(dest, texture_unpack);
		
		if(gif_usage != nullptr) {
			MobyGifUsage gif_entry;
			gif_entry.offset_and_terminator = abs_texture_unpack_ofs - 0xc - class_header_ofs;
			s32 gif_index = 0;
			for(const MobyTexturePrimitive& prim : submesh.textures) {
				assert(gif_index < 12);
				gif_entry.texture_indices[gif_index++] = prim.d3_tex0.data_lo;
			}
			for(s32 i = gif_index; i < 12; i++) {
				gif_entry.texture_indices[i] = 0xff;
			}
			gif_usage->push_back(gif_entry);
		}
		
		dest.pad(0x10);
		rel_texture_unpack_ofs = (dest.tell() - abs_texture_unpack_ofs + 0x4) / 0x10;
	}
	
	return rel_texture_unpack_ofs;
}

static MatrixTransferSchedule schedule_matrix_transfers(s32 smi, const MobySubMesh& submesh, MobySubMeshLowLevel* last_submesh, VU0MatrixAllocator& mat_alloc, const std::vector<MatrixLivenessInfo>& liveness) {
	// Determine which slots in VU0 memory are in use by the previous submesh
	// while we are trying to do transfers for the current submesh.
	std::vector<bool> slots_in_use(0x40, false);
	if(last_submesh != nullptr) {
		size_t i = 0;
		size_t regular_begin = last_submesh->two_way_blend_vertex_count + last_submesh->three_way_blend_vertex_count;
		for(size_t i = regular_begin; i < last_submesh->vertices.size(); i++) {
			const MobyVertex& mv = last_submesh->vertices[i];
			slots_in_use[mv.v.regular.vu0_matrix_load_addr / 0x4] = true;
		}
	}
	
	// Find all the joints that are used by this submesh.
	std::set<u8> used_joints;
	std::vector<bool> joint_used_by_two_way_blends(256, false);
	for(size_t i = 0; i < submesh.vertices.size(); i++) {
		const Vertex& vertex = submesh.vertices[i];
		//if(vertex.skin.count == 1 || liveness[i].population_count > 0) {
		for(s32 j = 0; j < vertex.skin.count; j++) {
			u8 joint = (u8) vertex.skin.joints[j];
			if(vertex.skin.count == 2) {
				joint_used_by_two_way_blends[joint] = true;
			}
			used_joints.emplace(joint);
		}
	}
	
	//
	std::vector<u8> two_way_joints;
	std::set<u8> other_joints;
	for(auto iter = used_joints.rbegin(); iter != used_joints.rend(); iter++) {
		u8 joint = *iter;
		if(!joint_used_by_two_way_blends[joint]) {
			two_way_joints.push_back(joint);
		} else {
			other_joints.emplace(joint);
		}
	}
	std::reverse(BEGIN_END(two_way_joints));
	
	// Allocate space for most of the newly transferred matrices.
	std::vector<MobyMatrixTransfer> maybe_conflicting_matrix_transfers;
	std::vector<MobyMatrixTransfer> independent_matrix_transfers;
	for(u8 joint : other_joints) {
		Opt<u8> addr = mat_alloc.allocate_transferred(joint, "not two-way");
		if(addr.has_value()) {
			if(slots_in_use[*addr / 0x4]) {
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
	for(u8 joint : two_way_joints) {
		Opt<u8> addr = mat_alloc.allocate_transferred(joint, "maybe two-way");
		if(addr.has_value()) {
			allocated_two_way_transfers.push_back(MobyMatrixTransfer{joint, *addr});
		}
	}
	
	// Allocate space for newly blended matrices.
	for(size_t i = 0; i < submesh.vertices.size(); i++) {
		const Vertex& vertex = submesh.vertices[i];
		if(vertex.skin.count > 1) {
			mat_alloc.allocate_blended(submesh.vertices[i].skin, smi, liveness[i].last_submesh, submesh.vertices);
		}
	}
	
	auto xferlist = {
		allocated_two_way_transfers,
		maybe_conflicting_matrix_transfers,
		independent_matrix_transfers
	};
	
	// Count the number of two-way blends that will be issued for this submesh.
	s32 two_way_count = 0;
	for(size_t i = 0; i < submesh.vertices.size(); i++) {
		const Vertex& vertex = submesh.vertices[i];
		if(vertex.skin.count == 2) {
			MatrixAllocation allocation;
			if(liveness[i].population_count != 1) {
				auto alloc_opt = mat_alloc.get_allocation_pre(vertex.skin);
				if(alloc_opt.has_value()) {
					allocation = *alloc_opt;
				}
			}
			if(allocation.first_use_pre) {
				two_way_count++;
			}
		}
	}
	
	if(last_submesh != nullptr) {
		// Try to schedule as many matrix transfers as is possible given this
		// heuristic on the last submesh.
		assert(last_submesh->vertices.size() >= 1);
		s64 insert_index = (s64) last_submesh->vertices.size() - 1 - schedule.last_submesh_transfers.size();
		s64 last_three_way_end = last_submesh->two_way_blend_vertex_count + last_submesh->three_way_blend_vertex_count;
		for(MobyMatrixTransfer& transfer : matrix_transfers) {
			if(insert_index >= last_three_way_end) {
				bool conflict = false;
				for(size_t i = insert_index; i < last_submesh->vertices.size(); i++) {
					const MobyVertex& mv = last_submesh->vertices[i];
					if(mv.v.regular.vu0_matrix_load_addr == transfer.vu0_dest_addr) {
						conflict = true;
						break;
					}
				}
				
				if(!conflict) {
					schedule.last_submesh_transfers.push_back(transfer);
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
	for(MobyMatrixTransfer& transfer : allocated_two_way_transfers) {
		if(two_way_insert_index < two_way_count) {
			schedule.two_way_transfers.push_back(transfer);
			two_way_insert_index++;
		} else {
			bool last_submesh_has_space = last_submesh != nullptr &&
				schedule.last_submesh_transfers.size() < last_submesh->main_vertex_count;
			if(last_submesh_has_space && !slots_in_use[transfer.vu0_dest_addr / 0x4]) {
				schedule.last_submesh_transfers.push_back(transfer);
			} else {
				schedule.preloop_transfers.push_back(transfer);
			}
		}
	}
	
	return schedule;
}

static MobySubMeshLowLevel pack_vertices(s32 smi, const MobySubMesh& submesh, VU0MatrixAllocator& mat_alloc, const std::vector<MatrixLivenessInfo>& liveness, f32 scale) {
	MobySubMeshLowLevel dest{submesh};
	dest.index_mapping.resize(submesh.vertices.size());
	
	f32 inverse_scale = 1024.f / scale;
	
	std::vector<bool> first_uses(submesh.vertices.size(), false);
	
	// Pack vertices that should issue a 2-way matrix blend operation on VU0.
	for(size_t i = 0; i < submesh.vertices.size(); i++) {
		const Vertex& vertex = submesh.vertices[i];
		if(vertex.skin.count == 2) {
			MatrixAllocation allocation;
			if(liveness[i].population_count != 1) {
				auto alloc_opt = mat_alloc.get_allocation(vertex.skin, smi);
				if(alloc_opt.has_value()) {
					allocation = *alloc_opt;
				}
			}
			if(allocation.first_use) {
				first_uses[i] = true;
				dest.two_way_blend_vertex_count++;
				dest.index_mapping[i] = dest.vertices.size();
				
				MobyVertex mv = {0};
				mv.v.i.low_halfword = vertex.vertex_index & 0x1ff;
				pack_common_attributes(mv, vertex, inverse_scale);
				
				SkinAttributes load_1 = {1, {vertex.skin.joints[0], 0, 0}, {255, 0, 0}};
				SkinAttributes load_2 = {1, {vertex.skin.joints[1], 0, 0}, {255, 0, 0}};
				
				auto alloc_1 = mat_alloc.get_allocation(load_1, smi);
				auto alloc_2 = mat_alloc.get_allocation(load_2, smi);
				assert(alloc_1 && alloc_2);
				
				mv.v.two_way_blend.vu0_matrix_load_addr_1 = alloc_1->address;
				mv.v.two_way_blend.vu0_matrix_load_addr_2 = alloc_2->address;
				mv.v.two_way_blend.weight_1 = vertex.skin.weights[0];
				mv.v.two_way_blend.weight_2 = vertex.skin.weights[1];
				mv.v.two_way_blend.vu0_transferred_matrix_store_addr = 0xf4;
				if(liveness[i].population_count > 1) {
					mv.v.two_way_blend.vu0_blended_matrix_store_addr = allocation.address;
				} else {
					mv.v.two_way_blend.vu0_blended_matrix_store_addr = 0xf4;
				}
				
				dest.vertices.emplace_back(mv);
			}
		}
	}
	
	// Pack vertices that should issue a 3-way matrix blend operation on VU0.
	for(size_t i = 0; i < submesh.vertices.size(); i++) {
		const Vertex& vertex = submesh.vertices[i];
		if(vertex.skin.count == 3) {
			MatrixAllocation allocation;
			if(liveness[i].population_count != 1) {
				auto alloc_opt = mat_alloc.get_allocation(vertex.skin, smi);
				if(alloc_opt.has_value()) {
					allocation = *alloc_opt;
				}
			}
			if(allocation.first_use) {
				first_uses[i] = true;
				dest.three_way_blend_vertex_count++;
				dest.index_mapping[i] = dest.vertices.size();
				
				MobyVertex mv = {0};
				mv.v.i.low_halfword = vertex.vertex_index & 0x1ff;
				pack_common_attributes(mv, vertex, inverse_scale);
				
				SkinAttributes load_1 = {1, {vertex.skin.joints[0], 0, 0}, {255, 0, 0}};
				SkinAttributes load_2 = {1, {vertex.skin.joints[1], 0, 0}, {255, 0, 0}};
				SkinAttributes load_3 = {1, {vertex.skin.joints[2], 0, 0}, {255, 0, 0}};
				
				auto alloc_1 = mat_alloc.get_allocation(load_1, smi);
				auto alloc_2 = mat_alloc.get_allocation(load_2, smi);
				auto alloc_3 = mat_alloc.get_allocation(load_3, smi);
				assert(alloc_1 && alloc_2 && alloc_3);
				
				mv.v.three_way_blend.vu0_matrix_load_addr_1 = alloc_1->address;
				mv.v.three_way_blend.vu0_matrix_load_addr_2 = alloc_2->address;
				mv.v.three_way_blend.low_halfword |= (alloc_3->address / 2) << 9;
				mv.v.three_way_blend.weight_1 = vertex.skin.weights[0];
				mv.v.three_way_blend.weight_2 = vertex.skin.weights[1];
				mv.v.three_way_blend.weight_3 = vertex.skin.weights[2];
				if(liveness[i].population_count > 1) {
					mv.v.three_way_blend.vu0_blended_matrix_store_addr = allocation.address;
				} else {
					mv.v.three_way_blend.vu0_blended_matrix_store_addr = 0xf4;
				}
				
				dest.vertices.emplace_back(mv);
			}
		}
	}
	
	// Pack vertices that use unblended matrices.
	for(size_t i = 0; i < submesh.vertices.size(); i++) {
		const Vertex& vertex = submesh.vertices[i];
		if(vertex.skin.count == 1) {
			dest.main_vertex_count++;
			dest.index_mapping[i] = dest.vertices.size();
			
			auto alloc = mat_alloc.get_allocation(vertex.skin, smi);
			assert(alloc);
			
			MobyVertex mv = {0};
			mv.v.i.low_halfword = vertex.vertex_index & 0x1ff;
			pack_common_attributes(mv, vertex, inverse_scale);
			mv.v.regular.vu0_matrix_load_addr = alloc->address;
			mv.v.regular.vu0_transferred_matrix_store_addr = 0xf4;
			//mv.v.regular.unused_5 = smi;
			
			dest.vertices.emplace_back(mv);
		}
	}
	
	// Pack vertices that use previously blended matrices.
	for(size_t i = 0; i < submesh.vertices.size(); i++) {
		const Vertex& vertex = submesh.vertices[i];
		if(vertex.skin.count > 1 && !first_uses[i]) {
			dest.main_vertex_count++;
			dest.index_mapping[i] = dest.vertices.size();
			
			auto alloc = mat_alloc.get_allocation(vertex.skin, smi);
			assert(alloc);
			
			MobyVertex mv = {0};
			mv.v.i.low_halfword = vertex.vertex_index & 0x1ff;
			pack_common_attributes(mv, vertex, inverse_scale);
			mv.v.regular.vu0_matrix_load_addr = alloc->address;
			mv.v.regular.vu0_transferred_matrix_store_addr = 0xf4;
			//mv.v.regular.unused_5 = smi;
			
			dest.vertices.emplace_back(mv);
		}
	}
	
	return dest;
}

VU0MatrixAllocator::VU0MatrixAllocator(s32 max_joints_per_submesh) {
	first_blend_store_addr = max_joints_per_submesh * 0x4;
	next_blend_store_addr = first_blend_store_addr;
	verify(first_blend_store_addr < 0xf4, "Failed to allocate transfer matrices in VU0 memory. Try simplifying your joint weights.");
}

void VU0MatrixAllocator::new_submesh() {
	next_blend_store_addr = first_blend_store_addr;
	transfer_allocations_this_submesh = 0;
	blend_allocations_this_submesh = 0;
	for(s32 i = 0; i < first_blend_store_addr; i += 0x4) {
		slots[i / 0x4].generation++;
	}
}

Opt<u8> VU0MatrixAllocator::allocate_transferred(u8 joint, const char* context) {
	SkinAttributes attribs{1, {joint, 0, 0}, {255, 0, 0}};
	MatrixAllocation& allocation = allocations[attribs];
	if(allocation.generation != slots[allocation.address / 0x4].generation) {
		VERBOSE_MATRIX_ALLOCATION(printf("Alloc %s transferred matrix {%hhu,{%hhd,%hhd,%hhd},{%hhu,%hhu,%hhu}} -> %hhx\n",
			context, attribs.count, attribs.joints[0], attribs.joints[1], attribs.joints[2],
			attribs.weights[0], attribs.weights[1], attribs.weights[2], next_transfer_store_addr));
		
		MatrixSlot& slot = slots[next_transfer_store_addr / 0x4];
		slot.generation++;
		allocation = MatrixAllocation{next_transfer_store_addr, true, true, slot.generation};
		transfer_allocations_this_submesh++;
		next_transfer_store_addr += 0x4;
		if(next_transfer_store_addr >= first_blend_store_addr) {
			next_transfer_store_addr = 0;
		}
		return allocation.address;
	}
	return std::nullopt;
}

void VU0MatrixAllocator::allocate_blended(SkinAttributes attribs, s32 current_submesh, s32 last_submesh, const std::vector<Vertex>& vertices) {
	MatrixAllocation& allocation = allocations[attribs];
	if(allocation.generation != slots[allocation.address / 0x4].generation) {
		// Try to find a slot that isn't live.
		u8 first_addr = next_blend_store_addr;
		while(slots[next_blend_store_addr / 0x4].liveness >= current_submesh) {
			next_blend_store_addr += 0x4;
			if(next_blend_store_addr >= 0xf4) {
				next_blend_store_addr = first_blend_store_addr;
			}
			if(next_blend_store_addr == first_addr) {
				// All the slots are live, try to pick one anyway.
				s32 liveness = -1;
				for(s32 i = first_blend_store_addr; i < 0xf4; i += 0x4) {
					MatrixSlot& slot = slots[i / 0x4];
					
					bool used_by_this_submesh = false;
					for(const Vertex& vertex : vertices) {
						if(vertex.skin == slot.current_contents) {
							used_by_this_submesh = true;
						}
					}
					
					if(slot.liveness > liveness && !used_by_this_submesh) {
						// Make sure we're not writing over data that's going to
						// be needed for this submesh.
						next_blend_store_addr = i;
						liveness = slot.liveness;
					}
				}
				if(liveness == -1) {
					allocations.erase(attribs);
					return;
				}
				break;
			}
		}
		
		VERBOSE_MATRIX_ALLOCATION(printf("Alloc blended matrix {%hhu,{%hhd,%hhd,%hhd},{%hhu,%hhu,%hhu}} -> %hhx (%d)\n",
			attribs.count, attribs.joints[0], attribs.joints[1], attribs.joints[2],
			attribs.weights[0], attribs.weights[1], attribs.weights[2], next_blend_store_addr, last_submesh));
		
		MatrixSlot& slot = slots[next_blend_store_addr / 0x4];
		slot.generation++;
		slot.liveness = last_submesh;
		slot.current_contents = attribs;
		allocation = MatrixAllocation{next_blend_store_addr, true, true, slot.generation};
		blend_allocations_this_submesh++;
		next_blend_store_addr += 0x4;
		if(next_blend_store_addr >= 0xf4) {
			next_blend_store_addr = first_blend_store_addr;
		}
	}
}

Opt<MatrixAllocation> VU0MatrixAllocator::get_allocation(SkinAttributes attribs, s32 current_submesh) {
	auto iter = allocations.find(attribs);
	if(iter == allocations.end()) {
		return std::nullopt;
	}
	MatrixAllocation& allocation = iter->second;
	MatrixSlot& slot = slots[allocation.address / 0x4];
	verify(allocation.generation == slot.generation,
		"Failed to get address for matrix with joint weights {%hhu,{%hhd,%hhd,%hhd},{%hhu,%hhu,%hhu}}. Generations are %d and %d.",
			attribs.count, attribs.joints[0], attribs.joints[1], attribs.joints[2],
			attribs.weights[0], attribs.weights[1], attribs.weights[2],
			allocation.generation, slot.generation);
	verify(attribs.count == 1 || slot.liveness >= current_submesh,
		"Bad liveness analysis (current submesh is %d, max is %d).",
		current_submesh, slot.liveness);
	MatrixAllocation copy = allocation;
	allocation.first_use = false;
	return copy;
}

Opt<MatrixAllocation> VU0MatrixAllocator::get_allocation_pre(SkinAttributes attribs) {
	auto iter = allocations.find(attribs);
	if(iter == allocations.end()) {
		return std::nullopt;
	}
	MatrixAllocation copy = iter->second;
	iter->second.first_use_pre = false;
	return copy;
}

static void pack_common_attributes(MobyVertex& dest, const Vertex& src, f32 inverse_scale) {
	dest.v.x = roundf(src.pos.x * inverse_scale);
	dest.v.y = roundf(src.pos.y * inverse_scale);
	dest.v.z = roundf(src.pos.z * inverse_scale);
	glm::vec3 normal = glm::normalize(src.normal);
	f32 normal_angle_azimuth_radians = atan2f(normal.x, normal.y);
	f32 normal_angle_elevation_radians = asinf(normal.z);
	dest.v.normal_angle_azimuth = roundf(normal_angle_azimuth_radians * (128.f / WRENCH_PI));
	dest.v.normal_angle_elevation = roundf(normal_angle_elevation_radians * (128.f / WRENCH_PI));
	// If the normal vector is pointing vertically upwards, the azimuth doesn't
	// matter so we set it to match the behaviour of Insomniac's exporter.
	if(dest.v.normal_angle_elevation == 0x40) {
		dest.v.normal_angle_azimuth += 0x80;
	}
}

static s32 max_num_joints_referenced_per_submesh(const std::vector<MobySubMesh>& submeshes) {
	// This seems suboptimal but it's what Insomniac did.
	s32 max_joints_per_submesh = 0;
	for(size_t i = 0; i < submeshes.size(); i++) {
		const MobySubMesh& submesh = submeshes[i];
		
		std::set<u8> joints;
		for(size_t j = 0; j < submesh.vertices.size(); j++) {
			const Vertex& vertex = submesh.vertices[j];
			for(s32 k = 0; k < vertex.skin.count; k++) {
				joints.emplace(vertex.skin.joints[k]);
			}
		}
		
		max_joints_per_submesh = std::max(max_joints_per_submesh, (s32) joints.size());
	}
	return max_joints_per_submesh;
}

static std::vector<std::vector<MatrixLivenessInfo>> compute_matrix_liveness(const std::vector<MobySubMesh>& submeshes) {
	std::vector<VertexLocation> mapping;
	for(size_t i = 0; i < submeshes.size(); i++) {
		for(size_t j = 0; j < submeshes[i].vertices.size(); j++) {
			mapping.push_back(VertexLocation{i, j});
		}
	}
	
	std::sort(BEGIN_END(mapping), [&](const VertexLocation& l, const VertexLocation& r) {
		return l.find_vertex_in(submeshes).skin < r.find_vertex_in(submeshes).skin;
	});
	
	std::vector<std::vector<MatrixLivenessInfo>> liveness;
	for(const MobySubMesh& submesh : submeshes) {
		liveness.emplace_back(submesh.vertices.size(), MatrixLivenessInfo{});
	}
	
	auto process_run = [&](size_t begin, size_t end) {
		VertexLocation first_vertex = {SIZE_MAX, SIZE_MAX};
		s32 last_submesh = -1;
		for(size_t j = begin; j < end; j++) {
			if(mapping[j].submesh < first_vertex.submesh) {
				first_vertex = mapping[j];
			}
			if(mapping[j].submesh == first_vertex.submesh && mapping[j].vertex < first_vertex.vertex) {
				first_vertex = mapping[j];
			}
			if((s32) mapping[j].submesh > last_submesh) {
				last_submesh = (s32) mapping[j].submesh;
			}
		}
		assert(first_vertex.submesh != SIZE_MAX);
		assert(first_vertex.vertex != SIZE_MAX);
		assert(last_submesh != -1);
		liveness[first_vertex.submesh][first_vertex.vertex].population_count = end - begin;
		for(size_t j = begin; j < end; j++) {
			liveness[mapping[j].submesh][mapping[j].vertex].last_submesh = last_submesh;
			liveness[mapping[j].submesh][mapping[j].vertex].first_vertex = first_vertex;
		}
	};
	
	size_t start_of_run = 0;
	for(size_t i = 1; i < mapping.size(); i++) {
		const Vertex& last = mapping[i - 1].find_vertex_in(submeshes);
		const Vertex& current = mapping[i].find_vertex_in(submeshes);
		if(!(current.skin == last.skin)) {
			process_run(start_of_run, i);
			start_of_run = i;
		}
	}
	if(start_of_run != mapping.size()) {
		process_run(start_of_run, mapping.size());
	}
	
	return liveness;
}

static MobyVertexTableHeaderRac1 write_vertices(OutBuffer& dest, const MobySubMesh& submesh, const MobySubMeshLowLevel& low, MobyFormat format) {
	s64 vertex_header_ofs;
	if(format == MobyFormat::RAC1) {
		vertex_header_ofs = dest.alloc<MobyVertexTableHeaderRac1>();
	} else {
		vertex_header_ofs = dest.alloc<MobyVertexTableHeaderRac23DL>();
	}
	
	MobyVertexTableHeaderRac1 vertex_header;
	vertex_header.matrix_transfer_count = low.preloop_matrix_transfers.size();
	vertex_header.two_way_blend_vertex_count = low.two_way_blend_vertex_count;
	vertex_header.three_way_blend_vertex_count = low.three_way_blend_vertex_count;
	vertex_header.main_vertex_count = low.main_vertex_count;
	
	std::vector<MobyVertex> vertices = low.vertices;
	dest.write_multiple(low.preloop_matrix_transfers);
	dest.pad(0x8);
	for(u16 dupe : submesh.duplicate_vertices) {
		dest.write<u16>(dupe << 7);
	}
	vertex_header.duplicate_vertex_count = submesh.duplicate_vertices.size();
	dest.pad(0x10);
	vertex_header.vertex_table_offset = dest.tell() - vertex_header_ofs;
	
	// Write out the remaining vertex indices after the rest of the proper
	// vertices (since the vertex index stored in each vertex corresponds to
	// the vertex 7 vertices prior for some reason). The remaining indices
	// are written out into the padding vertices and then when that space
	// runs out they're written into the second part of the last padding
	// vertex (hence there is at least one padding vertex). Now I see why
	// they call it Insomniac Games.
	std::vector<u16> trailing_vertex_indices(std::max(7 - (s32) vertices.size(), 0), 0);
	for(s32 i = std::max((s32) vertices.size() - 7, 0); i < vertices.size(); i++) {
		trailing_vertex_indices.push_back(vertices[i].v.i.low_halfword & 0x1ff);
	}
	for(s32 i = vertices.size() - 1; i >= 7; i--) {
		vertices[i].v.i.low_halfword = (vertices[i].v.i.low_halfword & ~0x1ff) | (vertices[i - 7].v.i.low_halfword & 0xff);
	}
	for(s32 i = 0; i < std::min(7, (s32) vertices.size()); i++) {
		vertices[i].v.i.low_halfword = vertices[i].v.i.low_halfword & ~0x1ff;
	}
	
	s32 trailing = 0;
	for(; vertices.size() % 4 != 2 && trailing < trailing_vertex_indices.size(); trailing++) {
		MobyVertex vertex = {0};
		if(submesh.vertices.size() + trailing >= 7) {
			vertex.v.i.low_halfword = trailing_vertex_indices[trailing];
		}
		vertices.push_back(vertex);
	}
	assert(trailing < trailing_vertex_indices.size());
	MobyVertex last_vertex = {0};
	if(submesh.vertices.size() + trailing >= 7) {
		last_vertex.v.i.low_halfword = trailing_vertex_indices[trailing];
	}
	for(s32 i = trailing + 1; i < trailing_vertex_indices.size(); i++) {
		if(submesh.vertices.size() + i >= 7) {
			last_vertex.trailing.vertex_indices[i - trailing - 1] = trailing_vertex_indices[i];
		}
	}
	vertices.push_back(last_vertex);
	
	// Write all the vertices.
	dest.write_multiple(vertices);
	
	// Fill in rest of the vertex header.
	vertex_header.transfer_vertex_count =
		vertex_header.two_way_blend_vertex_count +
		vertex_header.three_way_blend_vertex_count +
		vertex_header.main_vertex_count +
		vertex_header.duplicate_vertex_count;
	vertex_header.unknown_e = submesh.unknown_e;
	
	if(format == MobyFormat::RAC1) {
		vertex_header.unknown_e = dest.tell() - vertex_header_ofs;
		dest.write_multiple(submesh.unknown_e_data);
		dest.write(vertex_header_ofs, vertex_header);
	} else {
		MobyVertexTableHeaderRac23DL compact_vertex_header;
		compact_vertex_header.matrix_transfer_count = vertex_header.matrix_transfer_count;
		compact_vertex_header.two_way_blend_vertex_count = vertex_header.two_way_blend_vertex_count;
		compact_vertex_header.three_way_blend_vertex_count = vertex_header.three_way_blend_vertex_count;
		compact_vertex_header.main_vertex_count = vertex_header.main_vertex_count;
		compact_vertex_header.duplicate_vertex_count = vertex_header.duplicate_vertex_count;
		compact_vertex_header.transfer_vertex_count = vertex_header.transfer_vertex_count;
		compact_vertex_header.vertex_table_offset = vertex_header.vertex_table_offset;
		compact_vertex_header.unknown_e = vertex_header.unknown_e;
		dest.write(vertex_header_ofs, compact_vertex_header);
	}
	
	return vertex_header;
}

struct RichIndex {
	u32 index;
	bool restart;
	bool is_dupe = 0;
};

static std::vector<RichIndex> fake_tristripper(const std::vector<Face>& faces) {
	std::vector<RichIndex> indices;
	for(const Face& face : faces) {
		indices.push_back({(u32) face.v0, 1u});
		indices.push_back({(u32) face.v1, 1u});
		indices.push_back({(u32) face.v2, 0u});
	}
	return indices;
}


struct MidLevelTexture {
	s32 texture;
	s32 starting_index;
};

struct MidLevelVertex {
	s32 canonical;
	s32 tex_coord;
	s32 id = 0xff;
};

struct MidLevelDuplicateVertex {
	s32 index;
	s32 tex_coord;
};

// Intermediate data structure used so the submeshes can be built in two
// seperate passes.
struct MidLevelSubMesh {
	std::vector<MidLevelVertex> vertices;
	std::vector<RichIndex> indices;
	std::vector<MidLevelTexture> textures;
	std::vector<MidLevelDuplicateVertex> duplicate_vertices;
};

std::vector<MobySubMesh> build_moby_submeshes(const Mesh& mesh, const std::vector<Material>& materials) {
	static const s32 MAX_SUBMESH_TEXTURE_COUNT = 4;
	static const s32 MAX_SUBMESH_STORED_VERTEX_COUNT = 97;
	static const s32 MAX_SUBMESH_TOTAL_VERTEX_COUNT = 0x7f;
	static const s32 MAX_SUBMESH_INDEX_COUNT = 196;
	
	std::vector<IndexMappingRecord> index_mappings(mesh.vertices.size());
	find_duplicate_vertices(index_mappings, mesh.vertices);
	
	// *************************************************************************
	// First pass
	// *************************************************************************
	
	std::vector<MidLevelSubMesh> mid_submeshes;
	MidLevelSubMesh mid;
	s32 next_id = 0;
	for(s32 i = 0; i < (s32) mesh.submeshes.size(); i++) {
		const SubMesh& high = mesh.submeshes[i];
		
		auto indices = fake_tristripper(high.faces);
		if(indices.size() < 1) {
			continue;
		}
		
		const Material& material = materials.at(high.material);
		s32 texture = -1;
		if(material.name.size() > 4 && memcmp(material.name.data(), "mat_", 4) == 0) {
			texture = strtol(material.name.c_str() + 4, nullptr, 10);
		} else {
			fprintf(stderr, "Invalid material '%s'.", material.name.c_str());
			continue;
		}
		
		if(mid.textures.size() >= MAX_SUBMESH_TEXTURE_COUNT || mid.indices.size() >= MAX_SUBMESH_INDEX_COUNT) {
			mid_submeshes.emplace_back(std::move(mid));
			mid = MidLevelSubMesh{};
		}
		
		mid.textures.push_back({texture, (s32) mid.indices.size()});
		
		for(size_t j = 0; j < indices.size(); j++) {
			auto new_submesh = [&]() {
				mid_submeshes.emplace_back(std::move(mid));
				mid = MidLevelSubMesh{};
				// Handle splitting the strip up between moby submeshes.
				if(j - 2 >= 0) {
					if(!indices[j].restart) {
						j -= 3;
						indices[j + 1].restart = 1;
						indices[j + 2].restart = 1;
					} else if(!indices[j + 1].restart) {
						j -= 2;
						indices[j + 1].restart = 1;
						indices[j + 2].restart = 1;
					} else {
						j -= 1;
					}
				} else {
					// If we tried to start a tristrip at the end of the last
					// submesh but didn't push any non-restarting indices, go
					// back to the beginning of the strip.
					j = -1;
				}
			};
			
			RichIndex& r = indices[j];
			IndexMappingRecord& mapping = index_mappings[r.index];
			size_t canonical_index = r.index;
			//if(mapping.dedup_out_edge != -1) {
			//	canonical_index = mapping.dedup_out_edge;
			//}
			IndexMappingRecord& canonical = index_mappings[canonical_index];
			
			if(canonical.submesh != mid_submeshes.size()) {
				if(mid.vertices.size() >= MAX_SUBMESH_STORED_VERTEX_COUNT) {
					new_submesh();
					continue;
				}
				
				canonical.submesh = mid_submeshes.size();
				canonical.index = mid.vertices.size();
				
				mid.vertices.push_back({(s32) r.index, (s32) r.index});
			} else if(mapping.submesh != mid_submeshes.size()) {
				if(canonical.id == -1) {
					canonical.id = next_id++;
					mid.vertices.at(canonical.index).id = canonical.id;
				}
				mid.duplicate_vertices.push_back({canonical.id, (s32) r.index});
			}
			
			if(mid.indices.size() >= MAX_SUBMESH_INDEX_COUNT - 4) {
				new_submesh();
				continue;
			}
			
			mid.indices.push_back({(u32) canonical.index, r.restart, r.is_dupe});
		}
	}
	if(mid.indices.size() > 0) {
		mid_submeshes.emplace_back(std::move(mid));
	}
	
	// *************************************************************************
	// Second pass
	// *************************************************************************
	
	std::vector<MobySubMesh> low_submeshes;
	for(const MidLevelSubMesh& mid : mid_submeshes) {
		MobySubMesh low;
		
		for(const MidLevelVertex& vertex : mid.vertices) {
			const Vertex& high_vert = mesh.vertices[vertex.canonical];
			low.vertices.emplace_back(high_vert);
			
			const glm::vec2& tex_coord = mesh.vertices[vertex.tex_coord].tex_coord;
			s16 s = tex_coord.x * (INT16_MAX / 8.f);
			s16 t = tex_coord.y * (INT16_MAX / 8.f);
			low.sts.push_back({s, t});
		}
		
		s32 texture_index = 0;
		for(size_t i = 0; i < mid.indices.size(); i++) {
			RichIndex cur = mid.indices[i];
			u8 out;
			if(cur.is_dupe) {
				out = mid.vertices.size() + cur.index;
			} else {
				out = cur.index;
			}
			if(texture_index < mid.textures.size() && mid.textures.at(texture_index).starting_index >= i) {
				assert(cur.restart);
				low.indices.push_back(0);
				low.secret_indices.push_back(out + 1);
				texture_index++;
			} else {
				low.indices.push_back(cur.restart ? (out + 0x81) : (out + 1));
			}
		}
		
		// These fake indices are required to signal to the microprogram that it
		// should terminate.
		low.indices.push_back(1);
		low.indices.push_back(1);
		low.indices.push_back(1);
		low.indices.push_back(0);
		
		for(const MidLevelTexture& tex : mid.textures) {
			MobyTexturePrimitive primitive = {0};
			primitive.d1_xyzf2.data_lo = 0xff92; // Not sure.
			primitive.d1_xyzf2.data_hi = 0x4;
			primitive.d1_xyzf2.address = 0x4;
			primitive.d1_xyzf2.pad_a = 0x41a0;
			primitive.d2_clamp.address = 0x08;
			primitive.d3_tex0.address = 0x06;
			primitive.d3_tex0.data_lo = tex.texture;
			primitive.d4_xyzf2.address = 0x34;
			low.textures.push_back(primitive);
		}
		
		for(const MidLevelDuplicateVertex& dupe : mid.duplicate_vertices) {
			low.duplicate_vertices.push_back(dupe.index);
			
			const glm::vec2& tex_coord = mesh.vertices[dupe.tex_coord].tex_coord;
			s16 s = tex_coord.x * (INT16_MAX / 8.f);
			s16 t = tex_coord.y * (INT16_MAX / 8.f);
			low.sts.push_back({s, t});
		}
		
		low_submeshes.emplace_back(std::move(low));
	}
	
	return low_submeshes;
}

static void find_duplicate_vertices(std::vector<IndexMappingRecord>& index_mapping, const std::vector<Vertex>& vertices) {
	std::vector<size_t> indices(vertices.size());
	for(size_t i = 0; i < vertices.size(); i++) {
		indices[i] = i;
	}
	std::sort(BEGIN_END(indices), [&](size_t l, size_t r) {
		return vertices[l] < vertices[r];
	});
	
	for(size_t i = 1; i < indices.size(); i++) {
		const Vertex& prev = vertices[indices[i - 1]];
		const Vertex& cur = vertices[indices[i]];
		if(vec3_equal_eps(prev.pos, cur.pos) && vec3_equal_eps(prev.normal, cur.normal)) {
			size_t vert = indices[i - 1];
			if(index_mapping[vert].dedup_out_edge != -1) {
				vert = index_mapping[vert].dedup_out_edge;
			}
			index_mapping[indices[i]].dedup_out_edge = vert;
		}
	}
}
