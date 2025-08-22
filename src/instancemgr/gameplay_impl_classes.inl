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

#ifndef INSTANCEMGR_GAMEPLAY_IMPL_CLASSES_H
#define INSTANCEMGR_GAMEPLAY_IMPL_CLASSES_H

#include <instancemgr/gameplay_impl_common.inl>

struct ClassBlock
{
	static void read(std::vector<s32>& dest, Buffer src, Game game)
	{
		s32 count = src.read<s32>(0, "class count");
		dest = src.read_multiple<s32>(4, count, "class data").copy();
	}
	
	static void write(OutBuffer dest, const std::vector<s32>& src, Game game)
	{
		dest.write((s32) src.size());
		dest.write_multiple(src);
	}
};

packed_struct(MobyBlockHeader,
	s32 static_count;
	s32 spawnable_moby_count;
	s32 pad[2];
)

static std::map<s32, s32> moby_index_to_group(const std::vector<MobyGroupInstance>& groups)
{
	std::map<s32, s32> index_to_group;
	for(s32 i = 0; i < (s32) groups.size(); i++) {
		const MobyGroupInstance& group = groups[i];
		for(mobylink link : group.members) {
			verify(index_to_group.find(link.id) == index_to_group.end(), "A moby instance is in two or more different groups!");
			index_to_group[link.id] = i;
		}
	}
	return index_to_group;
}

packed_struct(RacMobyInstance,
	/* 0x00 */ s32 size; // sizeof(RacMobyInstance)
	/* 0x04 */ s32 unknown_4;
	/* 0x08 */ s32 unknown_8;
	/* 0x0c */ s32 unknown_c;
	/* 0x10 */ s32 unknown_10;
	/* 0x14 */ s32 unknown_14;
	/* 0x18 */ s32 o_class;
	/* 0x1c */ f32 scale;
	/* 0x20 */ f32 draw_distance;
	/* 0x24 */ s32 update_distance;
	/* 0x28 */ s32 unused_28;
	/* 0x2c */ s32 unused_2c;
	/* 0x30 */ Vec3f position;
	/* 0x3c */ Vec3f rotation;
	/* 0x48 */ s32 group;
	/* 0x4c */ s32 is_rooted;
	/* 0x50 */ f32 rooted_distance;
	/* 0x54 */ s32 unknown_54;
	/* 0x58 */ s32 pvar_index;
	/* 0x5c */ s32 occlusion; // 0 = precompute occlusion
	/* 0x60 */ s32 mode_bits;
	/* 0x64 */ Rgb96 colour;
	/* 0x70 */ s32 light;
	/* 0x74 */ s32 unknown_74;
)
static_assert(sizeof(RacMobyInstance) == 0x78);

struct RacMobyBlock
{
	static void read(Gameplay& gameplay, Buffer src, Game game)
	{
		auto& header = src.read<MobyBlockHeader>(0, "moby block header");
		gameplay.spawnable_moby_count = header.spawnable_moby_count;
		s32 index = 0;
		gameplay.moby_instances = std::vector<MobyInstance>();
		gameplay.moby_instances->reserve(header.static_count);
		for(RacMobyInstance entry : src.read_multiple<RacMobyInstance>(0x10, header.static_count, "moby instances")) {
			verify(entry.size == 0x78, "Moby size field has invalid value.");
			MobyInstance instance;
			instance.set_id_value(index++);
			swap_moby(instance, entry);
			gameplay.moby_instances->push_back(instance);
		}
	}
	
	static bool write(OutBuffer dest, const Gameplay& gameplay, Game game)
	{
		verify(gameplay.spawnable_moby_count.has_value(), "Missing dynamic moby count field.");
		verify(gameplay.moby_instances.has_value(), "Missing moby instances array.");
		verify(gameplay.moby_groups.has_value(), "Missing moby groups array.");
		
		std::map<s32, s32> index_to_group = moby_index_to_group(*gameplay.moby_groups);
		
		MobyBlockHeader header = {};
		header.static_count = gameplay.moby_instances->size();
		header.spawnable_moby_count = *gameplay.spawnable_moby_count;
		dest.write(header);
		for(size_t i = 0; i < gameplay.moby_instances->size(); i++) {
			MobyInstance instance = (*gameplay.moby_instances)[i];
			RacMobyInstance entry;
			swap_moby(instance, entry);
			auto group = index_to_group.find(i);
			if(group != index_to_group.end()) {
				entry.group = group->second;
			} else {
				entry.group = -1;
			}
			dest.write(entry);
		}
		return true;
	}
	
	static void swap_moby(MobyInstance& l, RacMobyInstance& r)
	{
		r.size = sizeof(RacMobyInstance);
		swap_position_rotation_scale(l, r);
		SWAP_PACKED(l.pvars().temp_pvar_index, r.pvar_index);
		SWAP_PACKED(l.draw_distance(), r.draw_distance);
		SWAP_PACKED(l.rac1_unknown_4, r.unknown_4);
		SWAP_PACKED(l.rac1_unknown_8, r.unknown_8);
		SWAP_PACKED(l.rac1_unknown_c, r.unknown_c);
		SWAP_PACKED(l.rac1_unknown_10, r.unknown_10);
		SWAP_PACKED(l.rac1_unknown_14, r.unknown_14);
		SWAP_PACKED(l.o_class(), r.o_class);
		SWAP_PACKED(l.update_distance, r.update_distance);
		r.unused_28 = 32;
		r.unused_2c = 64;
		SWAP_PACKED(l.is_rooted, r.is_rooted);
		SWAP_PACKED(l.rooted_distance, r.rooted_distance);
		SWAP_PACKED(l.rac1_unknown_54, r.unknown_54);
		SWAP_PACKED(l.occlusion, r.occlusion);
		SWAP_PACKED(l.mode_bits, r.mode_bits);
		SWAP_COLOUR(l.colour(), r.colour);
		SWAP_PACKED(l.light, r.light);
		SWAP_PACKED(l.rac1_unknown_74, r.unknown_74);
	}
};

packed_struct(GcUyaMobyInstance,
	/* 0x00 */ s32 size; // Always 0x88.
	/* 0x04 */ s32 mission;
	/* 0x08 */ s32 unknown_8;
	/* 0x0c */ s32 unknown_c;
	/* 0x10 */ s32 uid;
	/* 0x14 */ s32 bolts;
	/* 0x18 */ s32 unknown_18;
	/* 0x1c */ s32 unknown_1c;
	/* 0x20 */ s32 unknown_20;
	/* 0x24 */ s32 unknown_24;
	/* 0x28 */ s32 o_class;
	/* 0x2c */ f32 scale;
	/* 0x30 */ s32 draw_distance;
	/* 0x34 */ s32 update_distance;
	/* 0x38 */ s32 unused_38;
	/* 0x3c */ s32 unused_3c;
	/* 0x40 */ Vec3f position;
	/* 0x4c */ Vec3f rotation;
	/* 0x58 */ s32 group;
	/* 0x5c */ s32 is_rooted;
	/* 0x60 */ f32 rooted_distance;
	/* 0x64 */ s32 unknown_4c;
	/* 0x68 */ s32 pvar_index;
	/* 0x6c */ s32 occlusion; // 0 = precompute occlusion
	/* 0x70 */ s32 mode_bits;
	/* 0x74 */ Rgb96 light_colour;
	/* 0x80 */ s32 light;
	/* 0x84 */ s32 unknown_84;
)
static_assert(sizeof(GcUyaMobyInstance) == 0x88);

struct GcUyaMobyBlock
{
	static void read(Gameplay& gameplay, Buffer src, Game game)
	{
		auto& header = src.read<MobyBlockHeader>(0, "moby block header");
		gameplay.spawnable_moby_count = header.spawnable_moby_count;
		s32 index = 0;
		gameplay.moby_instances = std::vector<MobyInstance>();
		gameplay.moby_instances->reserve(header.static_count);
		for(GcUyaMobyInstance entry : src.read_multiple<GcUyaMobyInstance>(0x10, header.static_count, "moby instances")) {
			verify(entry.size == 0x88, "Moby size field has invalid value.");
			MobyInstance instance;
			instance.set_id_value(index++);
			swap_moby(instance, entry);
			gameplay.moby_instances->push_back(instance);
		}
	}
	
	static bool write(OutBuffer dest, const Gameplay& gameplay, Game game)
	{
		verify(gameplay.spawnable_moby_count.has_value(), "Missing dynamic moby count field.");
		verify(gameplay.moby_instances.has_value(), "Missing moby instances array.");
		verify(gameplay.moby_groups.has_value(), "Missing moby groups array.");
		
		std::map<s32, s32> index_to_group = moby_index_to_group(*gameplay.moby_groups);
		
		MobyBlockHeader header = {};
		header.static_count = gameplay.moby_instances->size();
		header.spawnable_moby_count = *gameplay.spawnable_moby_count;
		dest.write(header);
		for(size_t i = 0; i < gameplay.moby_instances->size(); i++) {
			MobyInstance instance = (*gameplay.moby_instances)[i];
			GcUyaMobyInstance entry;
			swap_moby(instance, entry);
			auto group = index_to_group.find(i);
			if(group != index_to_group.end()) {
				entry.group = group->second;
			} else {
				entry.group = -1;
			}
			dest.write(entry);
		}
		return true;
	}
	
	static void swap_moby(MobyInstance& l, GcUyaMobyInstance& r)
	{
		r.size = 0x88;
		swap_position_rotation_scale(l, r);
		SWAP_PACKED(l.pvars().temp_pvar_index, r.pvar_index);
		SWAP_PACKED(l.draw_distance(), r.draw_distance);
		SWAP_COLOUR(l.colour(), r.light_colour);
		SWAP_PACKED(l.mission, r.mission);
		SWAP_PACKED(l.rac23_unknown_8, r.unknown_8);
		SWAP_PACKED(l.rac23_unknown_c, r.unknown_c);
		SWAP_PACKED(l.uid, r.uid);
		SWAP_PACKED(l.bolts, r.bolts);
		SWAP_PACKED(l.rac23_unknown_18, r.unknown_18);
		SWAP_PACKED(l.rac23_unknown_1c, r.unknown_1c);
		SWAP_PACKED(l.rac23_unknown_20, r.unknown_20);
		SWAP_PACKED(l.rac23_unknown_24, r.unknown_24);
		SWAP_PACKED(l.o_class(), r.o_class);
		SWAP_PACKED(l.update_distance, r.update_distance);
		r.unused_38 = 32;
		r.unused_3c = 64;
		SWAP_PACKED(l.is_rooted, r.is_rooted);
		SWAP_PACKED(l.rooted_distance, r.rooted_distance);
		SWAP_PACKED(l.rac23_unknown_4c, r.unknown_4c);
		SWAP_PACKED(l.occlusion, r.occlusion);
		SWAP_PACKED(l.mode_bits, r.mode_bits);
		SWAP_PACKED(l.light, r.light);
		SWAP_PACKED(l.rac23_unknown_84, r.unknown_84);
	}
};

packed_struct(DlMobyInstance,
	/* 0x00 */ s32 size; // sizeof(DlMobyInstance)
	/* 0x04 */ s32 mission;
	/* 0x08 */ s32 uid;
	/* 0x0c */ s32 bolts;
	/* 0x10 */ s32 o_class;
	/* 0x14 */ f32 scale;
	/* 0x18 */ s32 draw_distance;
	/* 0x1c */ s32 update_distance;
	/* 0x20 */ s32 unused_20;
	/* 0x24 */ s32 unused_24;
	/* 0x28 */ Vec3f position;
	/* 0x34 */ Vec3f rotation;
	/* 0x40 */ s32 group;
	/* 0x44 */ s32 is_rooted;
	/* 0x48 */ f32 rooted_distance;
	/* 0x4c */ s32 unused_4c;
	/* 0x50 */ s32 pvar_index;
	/* 0x54 */ s32 occlusion; // 0 = precompute occlusion
	/* 0x58 */ s32 mode_bits;
	/* 0x5c */ Rgb96 colour;
	/* 0x68 */ s32 light;
	/* 0x6c */ s32 unused_6c;
)
static_assert(sizeof(DlMobyInstance) == 0x70);

struct DlMobyBlock
{
	static void read(Gameplay& gameplay, Buffer src, Game game)
	{
		auto& header = src.read<MobyBlockHeader>(0, "moby block header");
		gameplay.spawnable_moby_count = header.spawnable_moby_count;
		gameplay.moby_instances = std::vector<MobyInstance>();
		gameplay.moby_instances->reserve(header.static_count);
		s32 index = 0;
		for(DlMobyInstance entry : src.read_multiple<DlMobyInstance>(0x10, header.static_count, "moby instances")) {
			verify(entry.size == sizeof(DlMobyInstance), "Moby size field has invalid value.");
			verify(entry.unused_20 == 32, "Moby field has weird value.");
			verify(entry.unused_24 == 64, "Moby field has weird value.");
			verify(entry.unused_4c == 1, "Moby field has weird value.");
			verify(entry.unused_6c == -1, "Moby field has weird value.");
			
			MobyInstance instance;
			instance.set_id_value(gameplay.core_moby_count + index++);
			swap_moby(instance, entry);
			gameplay.moby_instances->push_back(instance);
		}
	}
	
	static bool write(OutBuffer dest, const Gameplay& gameplay, Game game)
	{
		verify(gameplay.spawnable_moby_count.has_value(), "Missing dynamic moby count field.");
		verify(gameplay.moby_instances.has_value(), "Missing moby instances array.");
		verify(gameplay.moby_groups.has_value(), "Missing moby groups array.");
		
		std::map<s32, s32> index_to_group = moby_index_to_group(*gameplay.moby_groups);
		
		MobyBlockHeader header = {};
		header.static_count = gameplay.moby_instances->size();
		header.spawnable_moby_count = *gameplay.spawnable_moby_count;
		dest.write(header);
		for(size_t i = 0; i < gameplay.moby_instances->size(); i++) {
			MobyInstance instance = (*gameplay.moby_instances)[i];
			DlMobyInstance entry;
			swap_moby(instance, entry);
			auto group = index_to_group.find(gameplay.core_moby_count + i);
			if(group != index_to_group.end()) {
				entry.group = group->second;
			} else {
				entry.group = -1;
			}
			dest.write(entry);
		}
		return true;
	}
	
	static void swap_moby(MobyInstance& l, DlMobyInstance& r)
	{
		r.size = 0x70;
		swap_position_rotation_scale(l, r);
		SWAP_PACKED(l.pvars().temp_pvar_index, r.pvar_index);
		SWAP_PACKED(l.draw_distance(), r.draw_distance);
		SWAP_COLOUR(l.colour(), r.colour);
		SWAP_PACKED(l.mission, r.mission);
		SWAP_PACKED(l.uid, r.uid);
		SWAP_PACKED(l.bolts, r.bolts);
		SWAP_PACKED(l.o_class(), r.o_class);
		SWAP_PACKED(l.update_distance, r.update_distance);
		r.unused_20 = 32;
		r.unused_24 = 64;
		SWAP_PACKED(l.is_rooted, r.is_rooted);
		SWAP_PACKED(l.rooted_distance, r.rooted_distance);
		r.unused_4c = 1;
		SWAP_PACKED(l.occlusion, r.occlusion);
		SWAP_PACKED(l.mode_bits, r.mode_bits);
		SWAP_PACKED(l.light, r.light);
		r.unused_6c = -1;
	}
};

struct PvarTableBlock
{
	static void read(Gameplay& dest, Buffer src, Game game)
	{
		s32 pvar_count = 0;
		for(const MobyInstance& inst : opt_iterator(dest.moby_instances)) {
			pvar_count = std::max(pvar_count, inst.pvars().temp_pvar_index + 1);
		}
		for(const CameraInstance& inst : opt_iterator(dest.cameras)) {
			pvar_count = std::max(pvar_count, inst.pvars().temp_pvar_index + 1);
		}
		for(const SoundInstance& inst : opt_iterator(dest.sound_instances)) {
			pvar_count = std::max(pvar_count, inst.pvars().temp_pvar_index + 1);
		}
		
		dest.pvar_table = src.read_multiple<PvarTableEntry>(0, pvar_count, "pvar table").copy();
	}
	
	static bool write(OutBuffer dest, const Gameplay& src, Game game)
	{
		verify_fatal(src.pvar_table.has_value());
		dest.write_multiple(*src.pvar_table);
		return true;
	}
};

struct PvarDataBlock
{
	static void read(Gameplay& dest, Buffer src, Game game)
	{
		verify_fatal(dest.pvar_table.has_value());
		s32 size = 0;
		for(PvarTableEntry& entry : *dest.pvar_table) {
			size = std::max(entry.offset + entry.size, size);
		}
		dest.pvar_data = src.read_multiple<u8>(0, size, "pvar data").copy();
	}
	
	static bool write(OutBuffer dest, const Gameplay& src, Game game)
	{
		verify_fatal(src.pvar_data.has_value());
		dest.write_multiple(*src.pvar_data);
		return true;
	}
};

struct PvarFixupBlock
{
	static void read(std::vector<PvarFixupEntry>& dest, Buffer src, Game game)
	{
		for(s64 offset = 0;; offset += sizeof(PvarFixupEntry)) {
			auto& entry = src.read<PvarFixupEntry>(offset, "pvar scratchpad block");
			if(entry.pvar_index < 0) {
				break;
			}
			
			dest.emplace_back(entry);
		}
	}
	
	static bool write(OutBuffer dest, const std::vector<PvarFixupEntry>& src, Game game)
	{
		dest.write_multiple(src);
		dest.write<s32>(-1);
		dest.write<s32>(-1);
		return true;
	}
};

packed_struct(GroupHeader,
	/* 0x0 */ s32 group_count;
	/* 0x4 */ s32 data_size;
	/* 0x8 */ s32 pad[2];
)

template <typename GroupInstance>
struct GroupBlock
{
	static void read(std::vector<GroupInstance>& dest, Buffer src, Game game)
	{
		auto& header = src.read<GroupHeader>(0, "group block header");
		auto pointers = src.read_multiple<s32>(0x10, header.group_count, "group pointers");
		s64 data_ofs = 0x10 + header.group_count * 4;
		if(data_ofs % 0x10 != 0) {
			data_ofs += 0x10 - (data_ofs % 0x10);
		}
		auto members = src.read_multiple<u16>(data_ofs, header.data_size / 2, "groups");
		s32 index = 0;
		for(s32 pointer : pointers) {
			s32 member_index = pointer / 2;
			GroupInstance& group = dest.emplace_back();
			group.set_id_value(index++);
			s32 member;
			if(pointer >= 0) {
				do {
					member = members[member_index++];
					group.members.emplace_back(member & 0x7fff);
				} while(!(member & 0x8000));
			}
		}
	}
	
	static void write(OutBuffer dest, const std::vector<GroupInstance>& src, Game game)
	{
		s64 header_ofs = dest.alloc<GroupHeader>();
		s64 pointer_ofs = dest.alloc_multiple<s32>(src.size());
		dest.pad(0x10, 0);
		s64 data_ofs = dest.tell();
		
		std::vector<s32> pointers;
		pointers.reserve(src.size());
		for(const GroupInstance& group : src) {
			if(group.members.size() > 0) {
				pointers.push_back(dest.tell() - data_ofs);
				for(size_t i = 0; i < group.members.size(); i++) {
					if(i == group.members.size() - 1) {
						dest.write<u16>(group.members[i].id | 0x8000);
					} else {
						dest.write<u16>(group.members[i].id);
					}
				}
			} else {
				pointers.push_back(-1);
			}
		}
		
		dest.pad(0x10, 0);
		
		GroupHeader header = {};
		header.group_count = src.size();
		header.data_size = dest.tell() - data_ofs;
		dest.write(header_ofs, header);
		dest.write_multiple(pointer_ofs, pointers);
	}
};

packed_struct(SharedDataBlockHeader,
	/* 0x0 */ s32 data_size;
	/* 0x4 */ s32 pointer_count;
	/* 0x8 */ s32 unused_8[2];
)

struct SharedDataBlock
{
	static void read(Gameplay& dest, Buffer src, Game game)
	{
		auto& header = src.read<SharedDataBlockHeader>(0, "global pvar block header");
		dest.shared_data = src.read_multiple<u8>(0x10, header.data_size, "global pvar").copy();
		dest.shared_data_table = src.read_multiple<SharedDataEntry>(0x10 + header.data_size, header.pointer_count, "global pvar pointers").copy();
	}
	
	static bool write(OutBuffer dest, const Gameplay& src, Game game)
	{
		if(!src.shared_data.has_value() || !src.shared_data_table.has_value()) {
			SharedDataBlockHeader header = {};
			dest.write(header);
			return true;
		}
		
		SharedDataBlockHeader header = {};
		header.data_size = align32((s32) src.shared_data->size(), 0x10);
		header.pointer_count = (s32) src.shared_data_table->size();
		dest.write(header);
		
		dest.write_multiple(*src.shared_data);
		dest.pad(0x10);
		dest.write_multiple(*src.shared_data_table);
		
		return true;
	}
};

struct TieAmbientRgbaBlock
{
	static void read(Gameplay& dest, Buffer src, Game game)
	{
		if(!dest.tie_instances.has_value()) {
			return;
		}
		s64 ofs = 0;
		for(;;) {
			s16 index = src.read<s16>(ofs, "tie ambient RGBA index");
			ofs += 2;
			if(index == -1) {
				break;
			}
			TieInstance& inst = dest.tie_instances->at(index);
			s64 size = ((s64) src.read<s16>(ofs, "tie ambient RGBA size")) * 2;
			ofs += 2;
			inst.ambient_rgbas = src.read_multiple<u8>(ofs, size, "tie ambient RGBA data").copy();
			ofs += size;
		}
	}
	
	static bool write(OutBuffer dest, const Gameplay& src, Game game)
	{
		s16 index = 0;
		for(const TieInstance& inst : opt_iterator(src.tie_instances)) {
			if(inst.ambient_rgbas.size() > 0) {
				dest.write(index);
				verify_fatal(inst.ambient_rgbas.size() % 2 == 0);
				dest.write<s16>(inst.ambient_rgbas.size() / 2);
				dest.write_multiple(inst.ambient_rgbas);
			}
			index++;
		}
		dest.write<s16>(-1);
		return true;
	}
};

struct TieClassBlock
{
	static void read(Gameplay& dest, Buffer src, Game game) {}
	
	static bool write(OutBuffer dest, const Gameplay& src, Game game)
	{
		std::vector<s32> classes;
		for(const TieInstance& inst : opt_iterator(src.tie_instances)) {
			if(std::find(BEGIN_END(classes), inst.o_class()) == classes.end()) {
				classes.push_back(inst.o_class());
			}
		}
		dest.write<s32>(classes.size());
		dest.write_multiple(classes);
		return true;
	}
};

struct ShrubClassBlock
{
	static void read(Gameplay& dest, Buffer src, Game game) {}
	
	static bool write(OutBuffer dest, const Gameplay& src, Game game)
	{
		std::vector<s32> classes;
		for(const ShrubInstance& inst : opt_iterator(src.shrub_instances)) {
			if(std::find(BEGIN_END(classes), inst.o_class()) == classes.end()) {
				classes.push_back(inst.o_class());
			}
		}
		dest.write<s32>(classes.size());
		dest.write_multiple(classes);
		return true;
	}
};

packed_struct(RacTieInstance,
	/* 0x00 */ s32 o_class;
	/* 0x04 */ s32 draw_distance;
	/* 0x08 */ s32 pad_8;
	/* 0x0c */ s32 occlusion_index;
	/* 0x10 */ Mat4 matrix;
	/* 0x50 */ u8 ambient_rgbas[0x80];
	/* 0x50 */ s32 directional_lights;
	/* 0x54 */ s32 uid;
	/* 0x58 */ s32 pad_58;
	/* 0x5c */ s32 pad_5c;
)
static_assert(sizeof(RacTieInstance) == 0xe0);

static void swap_instance(TieInstance& l, RacTieInstance& r)
{
	swap_matrix(l, r);
	SWAP_PACKED(l.draw_distance(), r.draw_distance);
	SWAP_PACKED(l.o_class(), r.o_class);
	r.pad_8 = 0;
	SWAP_PACKED(l.occlusion_index, r.occlusion_index);
	SWAP_PACKED(l.directional_lights, r.directional_lights);
	SWAP_PACKED(l.uid, r.uid);
	r.pad_58 = 0;
	r.pad_5c = 0;
	u8 temp_rgbas[0x80];
	if(l.ambient_rgbas.size() == 0x80) {
		memcpy(temp_rgbas, l.ambient_rgbas.data(), 0x80);
	} else {
		memset(temp_rgbas, 0, 0x80);
	}
	l.ambient_rgbas.resize(0x80);
	memcpy(l.ambient_rgbas.data(), r.ambient_rgbas, 0x80);
	memcpy(r.ambient_rgbas, temp_rgbas, 0x80);
}

packed_struct(GcUyaDlTieInstance,
	/* 0x00 */ s32 o_class;
	/* 0x04 */ s32 draw_distance;
	/* 0x08 */ s32 pad_8;
	/* 0x0c */ s32 occlusion_index;
	/* 0x10 */ Mat4 matrix;
	/* 0x50 */ s32 directional_lights;
	/* 0x54 */ s32 uid;
	/* 0x58 */ s32 pad_58;
	/* 0x5c */ s32 pad_5c;
)
static_assert(sizeof(GcUyaDlTieInstance) == 0x60);

static void swap_instance(TieInstance& l, GcUyaDlTieInstance& r)
{
	swap_matrix(l, r);
	SWAP_PACKED(l.draw_distance(), r.draw_distance);
	SWAP_PACKED(l.o_class(), r.o_class);
	r.pad_8 = 0;
	SWAP_PACKED(l.occlusion_index, r.occlusion_index);
	SWAP_PACKED(l.directional_lights, r.directional_lights);
	SWAP_PACKED(l.uid, r.uid);
	r.pad_58 = 0;
	r.pad_5c = 0;
}

packed_struct(ShrubInstancePacked,
	/* 0x00 */ s32 o_class;
	/* 0x04 */ f32 draw_distance;
	/* 0x08 */ s32 unused_8;
	/* 0x0c */ s32 unused_c;
	/* 0x10 */ Mat4 matrix;
	/* 0x50 */ Rgb96 colour;
	/* 0x5c */ s32 unused_5c;
	/* 0x60 */ s32 dir_lights;
	/* 0x64 */ s32 unused_64;
	/* 0x68 */ s32 unused_68;
	/* 0x6c */ s32 unused_6c;
)

static void swap_instance(ShrubInstance& l, ShrubInstancePacked& r)
{
	swap_matrix(l, r);
	SWAP_PACKED(l.draw_distance(), r.draw_distance);
	SWAP_COLOUR(l.colour(), r.colour);
	SWAP_PACKED(l.o_class(), r.o_class);
	r.unused_8 = 0;
	r.unused_c = 0;
	r.unused_5c = 0;
	SWAP_PACKED(l.dir_lights, r.dir_lights);
	r.unused_64 = 0;
	r.unused_68 = 0;
	r.unused_6c = 0;
}

#endif
