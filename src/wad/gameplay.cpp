/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2021 chaoticgd

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

#include "gameplay.h"

const s32 NONE = -1;

void read_gameplay(Gameplay& gameplay, Buffer src) {
	for(const GameplayBlockDescription& block_desc : gameplay_blocks) {
		auto& pos = block_desc.rac4;
		s32 block_offset = src.read<s32>(pos.offset, "gameplay header");
		block_desc.funcs.read(gameplay, src.subbuf(block_offset));
	}
}

std::vector<u8> write_gameplay(const Gameplay& gameplay) {
	std::vector<GameplayBlockDescription> blocks = gameplay_blocks;
	std::sort(blocks.begin(), blocks.end(), [](auto& lhs, auto& rhs)
		{ return lhs.rac4.order < rhs.rac4.order; });
	
	s64 block_count = 0x20;
	
	std::vector<u8> dest_vec(block_count * 4, 0);
	OutBuffer dest(dest_vec);
	for(const GameplayBlockDescription& block : blocks) {
		if(block.rac4.offset != NONE) {
			if(strcmp(block.name, "us english strings") != 0) {
			dest.pad(0x10, 0);
			}
			s32 pos = (s32) dest_vec.size();
			block.funcs.write(dest, gameplay);
			assert(block.rac4.offset + 4 <= (s32) dest_vec.size());
			*(s32*) &dest_vec[block.rac4.offset] = pos;
		}
	}
	return dest_vec;
}

packed_struct(TableHeader,
	s32 count_1;
	s32 pad[3];
)

template <typename T>
struct TableBlock {
	static void read(std::vector<T>& dest, Buffer src) {
		auto header = src.read<TableHeader>(0, "table header");
		verify(header.pad[0] == 0, "TableBlock contains more than one table.");
		dest = src.read_multiple<T>(0x10, header.count_1, "table body").copy();
	}
	
	static void write(OutBuffer dest, const std::vector<T>& src) {
		TableHeader header = {(s32) src.size()};
		dest.write(header);
		for(const T& elem : src) {
			dest.write(elem);
		}
	}
};

struct PropertiesBlock {
	static void read(GpProperties& dest, Buffer src) {
		s32 ofs = 0;
		dest.first_part = src.read<GpPropertiesFirstPart>(ofs, "gameplay properties");
		ofs += sizeof(GpPropertiesFirstPart);
		s32 second_part_count = src.read<s32>(ofs + 0xc, "second part count");
		if(second_part_count > 0) {
			dest.second_part = src.read_multiple<GpPropertiesSecondPart>(ofs, second_part_count, "second part").copy();
			ofs += second_part_count * sizeof(GpPropertiesSecondPart);
		} else {
			ofs += sizeof(GpPropertiesSecondPart);
		}
		dest.core_sounds_count = src.read<s32>(ofs, "core sounds count");
		ofs += 4;
		s64 third_part_count = src.read<s32>(ofs, "third part count");
		ofs += 4;
		if(third_part_count >= 0) {
			dest.third_part = src.read_multiple<GpPropertiesThirdPart>(ofs, third_part_count, "third part").copy();
			ofs += third_part_count * sizeof(GpPropertiesThirdPart);
			dest.fourth_part = src.read<GpPropertiesFourthPart>(ofs, "fourth part");
			ofs += sizeof(GpPropertiesFourthPart);
		} else {
			ofs += sizeof(GpPropertiesThirdPart);
		}
		dest.fifth_part = src.read<GpPropertiesFifthPart>(ofs, "fifth part");
		ofs += sizeof(GpPropertiesFifthPart);
		dest.sixth_part = src.read_multiple<s8>(ofs, dest.fifth_part.sixth_part_count, "sixth part").copy();
	}
	
	static void write(OutBuffer dest, const GpProperties& src) {
		dest.write(src.first_part);
		if(src.second_part.size() > 0) {
			dest.write_multiple(src.second_part);
		} else {
			GpPropertiesSecondPart terminator = {0};
			dest.write(terminator);
		}
		dest.write(src.core_sounds_count);
		dest.write((s32) src.third_part.size());
		if(src.third_part.size() > 0) {
			dest.write_multiple(src.third_part);
			dest.write(src.fourth_part);
		} else {
			dest.vec.resize(dest.tell() + 0x18, 0);
		}
		dest.write(src.fifth_part);
		dest.write_multiple(src.sixth_part);
	}
};

packed_struct(StringBlockHeader,
	s32 string_count;
	s32 size;
)

packed_struct(StringTableEntry,
	s32 offset;
	s16 id;
	s16 unknown_6;
	s32 unknown_8;
	s16 unknown_c;
	s16 unknown_e;
)

struct StringBlock {
	static void read(std::vector<GpString>& dest, Buffer src) {
		auto& header = src.read<StringBlockHeader>(0, "string block header");
		auto table = src.read_multiple<StringTableEntry>(8, header.string_count, "string table");
		
		// TODO: For R&C3 and Deadlocked only.
		if(true) {
			src = src.subbuf(8);
		}
		
		for(StringTableEntry entry : table) {
			GpString string;
			if(entry.offset != 0) {
				string.string = src.read_string(entry.offset);
			}
			string.id = entry.id;
			string.unknown_6 = entry.unknown_6;
			string.unknown_8 = entry.unknown_8;
			string.unknown_c = entry.unknown_c;
			string.unknown_e = entry.unknown_e;
			dest.emplace_back(std::move(string));
		}
	}
	
	static void write(OutBuffer dest, const std::vector<GpString>& src) {
		s64 header_pos = dest.alloc<StringBlockHeader>();
		s64 table_pos = dest.alloc_multiple<StringTableEntry>(src.size());
		s64 entry_pos = table_pos;
		for(const GpString& string : src) {
			StringTableEntry entry {0};
			if(string.string) {
				entry.offset = dest.tell() - table_pos;
			}
			entry.id = string.id;
			entry.unknown_6 = string.unknown_6;
			entry.unknown_8 = string.unknown_8;
			entry.unknown_c = string.unknown_c;
			entry.unknown_e = string.unknown_e;
			dest.write(entry_pos, entry);
			entry_pos += sizeof(StringTableEntry);
			if(string.string) {
				for(char c : *string.string) {
					dest.write(c);
				}
				dest.write('\0');
			}
		}
		StringBlockHeader header;
		header.string_count = src.size();
		header.size = dest.tell() - table_pos;
		dest.write(header_pos, header);
	}
};

packed_struct(ImportCameraPacked,
	s32 unknown_0;
	s32 unknown_4;
	s32 unknown_8;
	s32 unknown_c;
	s32 unknown_10;
	s32 unknown_14;
	s32 unknown_18;
	s32 pvar_index;
)

template <typename Instance, typename Packed>
struct InstanceBlock {
	static void read(std::vector<Instance>& dest, Buffer src) {
		TableHeader header = src.read<TableHeader>(0, "instance block header");
		auto entries = src.read_multiple<Packed>(0x10, header.count_1, "instances");
		for(Packed packed : entries) {
			Instance inst;
			swap_instance(inst, packed);
			dest.push_back(inst);
		}
	}
	
	static void write(OutBuffer dest, const std::vector<Instance>& src) {
		TableHeader header = {(s32) src.size()};
		dest.write(header);
		for(Instance camera : src) {
			Packed packed;
			swap_instance(camera, packed);
			dest.write(packed);
		}
	}
};

static void swap_instance(ImportCamera& l, ImportCameraPacked& r) {
	SWAP_PACKED(l.unknown_0, r.unknown_0);
	SWAP_PACKED(l.unknown_4, r.unknown_4);
	SWAP_PACKED(l.unknown_8, r.unknown_8);
	SWAP_PACKED(l.unknown_c, r.unknown_c);
	SWAP_PACKED(l.unknown_10, r.unknown_10);
	SWAP_PACKED(l.unknown_14, r.unknown_14);
	SWAP_PACKED(l.unknown_18, r.unknown_18);
	SWAP_PACKED(l.pvar_index, r.pvar_index);
}

packed_struct(SoundInstancePacked,
	s16 o_class;
	s16 m_class;
	u32 update_fun_ptr;
	s32 pvar_index;
	f32 range;
	GpShape cuboid;
)

static void swap_instance(SoundInstance& l, SoundInstancePacked& r) {
	SWAP_PACKED(l.o_class, r.o_class);
	SWAP_PACKED(l.m_class, r.m_class);
	r.update_fun_ptr = 0;
	SWAP_PACKED(l.pvar_index, r.pvar_index);
	SWAP_PACKED(l.range, r.range);
	SWAP_PACKED(l.cuboid, r.cuboid);
}

template <typename T>
struct ClassBlock {
	static void read(T& dest, Buffer src) {
		s32 count = src.read<s32>(0, "class count");
		dest.classes = src.read_multiple<s32>(4, count, "class data").copy();
	}
	
	static void write(OutBuffer dest, const T& src) {
		dest.write((s32) src.classes.size());
		dest.write_multiple(src.classes);
	}
};

packed_struct(MobyBlockHeader,
	s32 static_count;
	s32 dynamic_count;
	s32 pad[2];
)

packed_struct(MobyInstanceDL,
	s32 size;        // 0x0 Always 0x70.
	s32 mission;     // 0x4
	s32 uid;         // 0x8
	s32 bolts;       // 0xc
	s32 o_class;     // 0x10
	f32 scale;       // 0x14
	s32 draw_dist;   // 0x18
	s32 update_dist; // 0x1c
	s32 unknown_20;  // 0x20
	s32 unknown_24;  // 0x24
	Vec3f position;  // 0x28
	Vec3f rotation;  // 0x34
	s32 group;       // 0x40
	s32 is_rooted;   // 0x44
	f32 rooted_dist; // 0x48
	s32 unknown_4c;  // 0x4c
	s32 pvar_index;  // 0x50
	s32 unknown_54;  // 0x54
	s32 unknown_58;  // 0x58
	s32 lights_1;    // 0x5c
	s32 lights_2;    // 0x60
	s32 lights_3;    // 0x64
	s32 unknown_68;  // 0x68
	s32 unknown_6c;  // 0x6c
)
static_assert(sizeof(MobyInstanceDL) == 0x70);

struct MobyBlock {
	static void read(MobyStore& dest, Buffer src) {
		auto& header = src.read<MobyBlockHeader>(0, "moby block header");
		dest.dynamic_count = header.dynamic_count;
		for(MobyInstanceDL entry : src.read_multiple<MobyInstanceDL>(0x10, header.static_count, "moby instances")) {
			MobyInstance instance;
			swap_moby(instance, entry);
			dest.instances.push_back(instance);
		}
	}
	
	static void write(OutBuffer dest, const MobyStore& src) {
		MobyBlockHeader header = {0};
		header.static_count = src.instances.size();
		header.dynamic_count = src.dynamic_count;
		dest.write(header);
		for(MobyInstance instance : src.instances) {
			MobyInstanceDL entry;
			swap_moby(instance, entry);
			dest.write(entry);
		}
	}
	
	static void swap_moby(MobyInstance& l, MobyInstanceDL& r) {
		SWAP_PACKED(l.size, r.size);
		SWAP_PACKED(l.mission, r.mission);
		SWAP_PACKED(l.uid, r.uid);
		SWAP_PACKED(l.bolts, r.bolts);
		SWAP_PACKED(l.o_class, r.o_class);
		SWAP_PACKED(l.scale, r.scale);
		SWAP_PACKED(l.draw_dist, r.draw_dist);
		SWAP_PACKED(l.update_dist, r.update_dist);
		SWAP_PACKED(l.dl.unknown_20, r.unknown_20);
		SWAP_PACKED(l.dl.unknown_24, r.unknown_24);
		SWAP_PACKED(l.position, r.position);
		SWAP_PACKED(l.rotation, r.rotation);
		SWAP_PACKED(l.group, r.group);
		SWAP_PACKED(l.is_rooted, r.is_rooted);
		SWAP_PACKED(l.rooted_dist, r.rooted_dist);
		SWAP_PACKED(l.dl.unknown_4c, r.unknown_4c);
		SWAP_PACKED(l.pvar_index, r.pvar_index);
		SWAP_PACKED(l.dl.unknown_54, r.unknown_54);
		SWAP_PACKED(l.dl.unknown_58, r.unknown_58);
		SWAP_PACKED(l.lights_1, r.lights_1);
		SWAP_PACKED(l.lights_2, r.lights_2);
		SWAP_PACKED(l.lights_3, r.lights_3);
		SWAP_PACKED(l.dl.unknown_68, r.unknown_68);
		SWAP_PACKED(l.dl.unknown_6c, r.unknown_6c);
	}
};

struct PvarTableBlock {
	static void read(Gameplay& dest, Buffer src) {
		s32 pvar_count = 0;
		for(const ImportCamera& camera : dest.import_cameras) {
			pvar_count = std::max(pvar_count, camera.pvar_index + 1);
		}
		for(const SoundInstance& inst : dest.sound_instances) {
			pvar_count = std::max(pvar_count, inst.pvar_index + 1);
		}
		for(const MobyInstance& inst : dest.moby.instances) {
			pvar_count = std::max(pvar_count, inst.pvar_index + 1);
		}
		
		// Store the table for use in PvarDataBlock::read.
		dest.pvars_temp = src.read_multiple<PvarTableEntry>(0, pvar_count, "pvar table").copy();
	}
	
	static void write(OutBuffer dest, const Gameplay& src) {
		s32 data_offset = 0;
		for(const ImportCamera& camera : src.import_cameras) {
			if(camera.pvars.size() > 0) {
				dest.write(data_offset);
				dest.write((s32) camera.pvars.size());
				data_offset += camera.pvars.size();
			}
		}
		for(const SoundInstance& inst : src.sound_instances) {
			if(inst.pvars.size() > 0) {
				dest.write(data_offset);
				dest.write((s32) inst.pvars.size());
				data_offset += inst.pvars.size();
			}
		}
		for(const MobyInstance& inst : src.moby.instances) {
			if(inst.pvars.size() > 0) {
				dest.write(data_offset);
				dest.write((s32) inst.pvars.size());
				data_offset += inst.pvars.size();
			}
		}
	}
};

struct PvarDataBlock {
	static void read(Gameplay& dest, Buffer src) {
		for(ImportCamera& camera : dest.import_cameras) {
			if(camera.pvar_index >= 0) {
				PvarTableEntry& entry = dest.pvars_temp->at(camera.pvar_index);
				camera.pvars = src.read_multiple<u8>(entry.offset, entry.size, "camera pvar data").copy();
			}
		}
		for(SoundInstance& inst : dest.sound_instances) {
			if(inst.pvar_index >= 0) {
				PvarTableEntry& entry = dest.pvars_temp->at(inst.pvar_index);
				inst.pvars = src.read_multiple<u8>(entry.offset, entry.size, "sound instance pvar data").copy();
			}
		}
		for(MobyInstance& inst : dest.moby.instances) {
			if(inst.pvar_index >= 0) {
				PvarTableEntry& entry = dest.pvars_temp->at(inst.pvar_index);
				inst.pvars = src.read_multiple<u8>(entry.offset, entry.size, "moby pvar data").copy();
			}
		}
		
		dest.pvars_temp = {};
	}
	
	static void write(OutBuffer dest, const Gameplay& src) {
		for(const ImportCamera& camera : src.import_cameras) {
			dest.write_multiple(camera.pvars);
		}
		for(const SoundInstance& inst : src.sound_instances) {
			dest.write_multiple(inst.pvars);
		}
		for(const MobyInstance& inst : src.moby.instances) {
			dest.write_multiple(inst.pvars);
		}
	}
};

template <typename T>
struct TerminatedArrayBlock {
	static void read(std::vector<T>& dest, Buffer src) {
		for(s64 offset = 0; src.read<s32>(offset, "array element") > -1; offset += sizeof(T)) {
			dest.emplace_back(src.read<T>(offset, "array element"));
		}
	}
	
	static void write(OutBuffer dest, const std::vector<T>& src) {
		dest.write_multiple(src);
		for(size_t i = 0; i < sizeof(T); i++) {
			dest.write<u8>(0xff);
		}
	}
};

packed_struct(DualTableHeader,
	s32 count_1;
	s32 count_2;
	s32 pad[2];
)

template <typename T>
struct DualTableBlock {
	using FirstType = decltype(T::first_part)::value_type;
	using SecondType = decltype(T::second_part)::value_type;
	
	static void read(T& dest, Buffer src) {
		auto header = src.read<DualTableHeader>(0, "dual table header");
		verify(header.pad[0] == 0, "DualTableBlock contains more than two tables.");
		dest.first_part = src.read_multiple<FirstType>(0x10, header.count_1, "table body").copy();
		s64 first_part_size = header.count_1 * sizeof(FirstType);
		dest.second_part = src.read_multiple<SecondType>(0x10 + first_part_size, header.count_2, "table body").copy();
	}
	
	static void write(OutBuffer dest, const T& src) {
		DualTableHeader header = {0};
		header.count_1 = src.first_part.size();
		header.count_2 = src.second_part.size();
		dest.write(header);
		for(const FirstType& elem : src.first_part) {
			dest.write(elem);
		}
		for(const SecondType& elem : src.second_part) {
			dest.write(elem);
		}
	}
};

static std::vector<std::vector<Vec4f>> read_splines(Buffer src, s32 count, s32 data_offset) {
	std::vector<std::vector<Vec4f>> splines;
	auto relative_offsets = src.read_multiple<s32>(0, count, "spline offsets");
	for(s32 relative_offset : relative_offsets) {
		s32 spline_offset = data_offset + relative_offset;
		auto header = src.read<TableHeader>(spline_offset, "spline vertex count");
		splines.emplace_back(src.read_multiple<Vec4f>(spline_offset + 0x10, header.count_1, "spline vertices").copy());
	}
	return splines;
}

static s32 write_splines(OutBuffer dest, const std::vector<std::vector<Vec4f>>& src) {
	s64 offsets_pos = dest.alloc_multiple<s32>(src.size());
	dest.pad(0x10, 0);
	s32 data_offset = (s32) dest.tell();
	for(std::vector<Vec4f> spline : src) {
		dest.pad(0x10, 0);
		s32 offset = dest.tell() - data_offset;
		dest.write(offsets_pos, offset);
		offsets_pos += 4;
		
		TableHeader header {(s32) spline.size()};
		dest.write(header);
		dest.write_multiple(spline);
	}
	return data_offset;
}

packed_struct(SplineBlockHeader,
	s32 spline_count;
	s32 data_offset;
	s32 data_size;
	s32 pad;
)

struct SplineBlock {
	static void read(std::vector<std::vector<Vec4f>>& dest, Buffer src) {
		auto& header = src.read<SplineBlockHeader>(0, "spline block header");
		dest = read_splines(src.subbuf(0x10), header.spline_count, header.data_offset - 0x10);
	}
	
	static void write(OutBuffer dest, const std::vector<std::vector<Vec4f>>& src) {
		s64 header_pos = dest.alloc<SplineBlockHeader>();
		SplineBlockHeader header = {0};
		header.spline_count = src.size();
		header.data_offset = write_splines(dest, src);
		header.data_size = dest.tell() - header.data_offset;
		header.data_offset -= header_pos;
		dest.write(header_pos, header);
	}
};

struct GC_88_DL_6c_Block {
	static void read(std::vector<u8>& dest, Buffer src) {
		s32 size = src.read<s32>(0, "block size");
		dest = src.read_multiple<u8>(4, size, "block data").copy();
	}
	
	static void write(OutBuffer dest, const std::vector<u8>& src) {
		dest.write((s32) src.size());
		dest.write_multiple(src);
	}
};

struct GC_80_DL_64_Block {
	static void read(Gp_GC_80_DL_64& dest, Buffer src) {
		auto header = src.read<TableHeader>(0, "block header");
		dest.first_part = src.read_multiple<u8>(0x10, 0x800, "first part of block").copy();
		dest.second_part = src.read_multiple<u8>(0x810, header.count_1 * 0x10, "second part of block").copy();
	}
	
	static void write(OutBuffer dest, const Gp_GC_80_DL_64& src) {
		TableHeader header {(s32) src.second_part.size() / 0x10};
		dest.write(header);
		dest.write_multiple(src.first_part);
		dest.write_multiple(src.second_part);
	}
};

struct GrindRailBlock {
	static void read(GrindRails& dest, Buffer src) {
		auto& header = src.read<SplineBlockHeader>(0, "spline block header");
		dest.grindrails = src.read_multiple<GrindRailData>(0x10, header.spline_count, "grindrail data").copy();
		s64 offsets_pos = 0x10 + header.spline_count * sizeof(GrindRailData);
		dest.splines = read_splines(src.subbuf(offsets_pos), header.spline_count, header.data_offset - offsets_pos);
	}
	
	static void write(OutBuffer dest, const GrindRails& src) {
		assert(src.grindrails.size() == src.splines.size());
		s64 header_pos = dest.alloc<SplineBlockHeader>();
		dest.write_multiple(src.grindrails);
		SplineBlockHeader header = {0};
		header.spline_count = src.grindrails.size();
		header.data_offset = write_splines(dest, src.splines);
		header.data_size = dest.tell() - header.data_offset;
		header.data_offset -= header_pos;
		dest.write(header_pos, header);
	}
};

packed_struct(GameplayAreaListHeader,
	s32 area_count;
	s32 part_offsets[5];
	s32 unknown_1c;
	s32 unknown_20;
)

packed_struct(GameplayAreaPacked,
	GpBSphere bsphere;
	s16 part_counts[5];
	s16 last_update_time;
	s32 relative_part_offsets[5];
)

struct GameplayAreaListBlock {
	static void read(std::vector<GpArea>& dest, Buffer src) {
		src = src.subbuf(4); // Skip past size field.
		s64 header_size = sizeof(GameplayAreaListHeader);
		auto header = src.read<GameplayAreaListHeader>(0, "area list block header");
		auto entries = src.read_multiple<GameplayAreaPacked>(header_size, header.area_count, "area list table");
		for(const GameplayAreaPacked& entry : entries) {
			GpArea area;
			area.bsphere = entry.bsphere;
			area.last_update_time = entry.last_update_time;
			for(s32 part = 0; part < 5; part++) {
				s32 part_ofs = header.part_offsets[part] + entry.relative_part_offsets[part];
				area.parts[part] = src.read_multiple<s32>(part_ofs, entry.part_counts[part], "area list data").copy();
			}
			dest.emplace_back(std::move(area));
		}
	}
	
	static void write(OutBuffer dest, const std::vector<GpArea>& src) {
		s64 size_ofs = dest.alloc<s32>();
		s64 header_ofs = dest.alloc<GameplayAreaListHeader>();
		s64 table_ofs = dest.alloc_multiple<GameplayAreaPacked>(src.size());
		
		s64 total_part_counts[5] = {0, 0, 0, 0, 0};
		
		GameplayAreaListHeader header;
		std::vector<GameplayAreaPacked> table;
		for(const GpArea& area : src) {
			GameplayAreaPacked packed;
			packed.bsphere = area.bsphere;
			for(s32 part = 0; part < 5; part++) {
				packed.part_counts[part] = area.parts[part].size();
				total_part_counts[part] += area.parts[part].size();
			}
			packed.last_update_time = area.last_update_time;
			table.emplace_back(std::move(packed));
		}
		
		for(s32 part = 0; part < 5; part++) {
			s64 paths_ofs = dest.tell();
			if(total_part_counts[part] > 0) {
				header.part_offsets[part] = paths_ofs - header_ofs;
			} else {
				header.part_offsets[part] = 0;
			}
			for(size_t area = 0; area < src.size(); area++) {
				if(table[area].part_counts[part] > 0) {
					table[area].relative_part_offsets[part] = dest.tell() - paths_ofs;
					dest.write_multiple(src[area].parts[part]);
				} else {
					table[area].relative_part_offsets[part] = 0;
				}
			}
		}
		header.area_count = src.size();
		header.unknown_1c = 0;
		header.unknown_20 = 0;
		
		dest.write(size_ofs, dest.tell() - header_ofs);
		dest.write(header_ofs, header);
		dest.write_multiple(table_ofs, table);
	}
};

template <typename Block, typename Field>
static GameplayBlockFuncs bf(Field field) {
	GameplayBlockFuncs funcs;
	funcs.read = [field](Gameplay& gameplay, Buffer src) {
		Block::read(gameplay.*field, src);
	};
	funcs.write = [field](OutBuffer dest, const Gameplay& gameplay) {
		Block::write(dest, gameplay.*field);
	};
	return funcs;
}

const std::vector<GameplayBlockDescription> gameplay_blocks = {
	//   R&C1         GC/UYA      Deadlocked
	//   ----         ------      ----------
	{{NONE, NONE}, {0x8c, NONE}, {0x70, 0x01}, bf<TableBlock<Gp_GC_8c_DL_70>>(&Gameplay::gc_8c_dl_70), "GC 8c DL 70"},
	{{NONE, NONE}, {NONE, NONE}, {0x00, 0x02}, bf<PropertiesBlock>(&Gameplay::properties), "properties"},
	{{NONE, NONE}, {NONE, NONE}, {0x0c, 0x03}, bf<StringBlock>(&Gameplay::us_english_strings), "us english strings"},
	{{NONE, NONE}, {NONE, NONE}, {0x10, 0x04}, bf<StringBlock>(&Gameplay::uk_english_strings), "uk english strings"},
	{{NONE, NONE}, {NONE, NONE}, {0x14, 0x05}, bf<StringBlock>(&Gameplay::french_strings), "french strings"},
	{{NONE, NONE}, {NONE, NONE}, {0x18, 0x06}, bf<StringBlock>(&Gameplay::german_strings), "german strings"},
	{{NONE, NONE}, {NONE, NONE}, {0x1c, 0x07}, bf<StringBlock>(&Gameplay::spanish_strings), "spanish strings"},
	{{NONE, NONE}, {NONE, NONE}, {0x20, 0x08}, bf<StringBlock>(&Gameplay::italian_strings), "italian strings"},
	{{NONE, NONE}, {NONE, NONE}, {0x24, 0x09}, bf<StringBlock>(&Gameplay::japanese_strings), "japanese strings"},
	{{NONE, NONE}, {NONE, NONE}, {0x28, 0x0a}, bf<StringBlock>(&Gameplay::korean_strings), "korean strings"},
	{{NONE, NONE}, {0x08, NONE}, {0x04, 0x0b}, bf<InstanceBlock<ImportCamera, ImportCameraPacked>>(&Gameplay::import_cameras), "import cameras"},
	{{NONE, NONE}, {0x0c, NONE}, {0x08, 0x0c}, bf<InstanceBlock<SoundInstance, SoundInstancePacked>>(&Gameplay::sound_instances), "sound instances"},
	{{NONE, NONE}, {0x48, NONE}, {0x2c, 0x0d}, bf<ClassBlock<MobyStore>>(&Gameplay::moby), "moby classes"},
	{{NONE, NONE}, {0x4c, NONE}, {0x30, 0x0e}, bf<MobyBlock>(&Gameplay::moby), "moby instances"},
	{{NONE, NONE}, {NONE, NONE}, {0x40, 0x0f}, {PvarTableBlock::read, PvarTableBlock::write}, "pvar table"},
	{{NONE, NONE}, {NONE, NONE}, {0x44, 0x10}, {PvarDataBlock::read, PvarDataBlock::write}, "pvar data"},
	{{NONE, NONE}, {NONE, NONE}, {0x3c, 0x11}, bf<TerminatedArrayBlock<Gp_DL_3c>>(&Gameplay::dl_3c), "DL 3c"},
	{{NONE, NONE}, {0x64, NONE}, {0x48, 0x12}, bf<TerminatedArrayBlock<Gp_GC_64_DL_48>>(&Gameplay::gc_64_dl_48), "GC 64 DL 48"},
	{{NONE, NONE}, {0x50, NONE}, {0x34, 0x13}, bf<DualTableBlock<GpMobyGroups>>(&Gameplay::moby_groups), "moby groups"},
	{{NONE, NONE}, {0x54, NONE}, {0x38, 0x14}, bf<DualTableBlock<Gp_GC_54_DL_38>>(&Gameplay::gc_54_dl_38), "GC 54 DL 38"},
	{{NONE, NONE}, {0x78, NONE}, {0x5c, 0x15}, bf<SplineBlock>(&Gameplay::splines), "splines"},
	{{NONE, NONE}, {0x68, NONE}, {0x4c, 0x16}, bf<TableBlock<GpShape>>(&Gameplay::cuboids), "cuboids"},
	{{NONE, NONE}, {0x6c, NONE}, {0x50, 0x17}, bf<TableBlock<GpShape>>(&Gameplay::spheres), "spheres"},
	{{NONE, NONE}, {0x70, NONE}, {0x54, 0x18}, bf<TableBlock<GpShape>>(&Gameplay::cylinders), "cylinders"},
	{{NONE, NONE}, {0x74, NONE}, {0x58, 0x19}, bf<TableBlock<s32>>(&Gameplay::gc_74_dl_58), "GC 74 DL 58"},
	{{NONE, NONE}, {0x88, NONE}, {0x6c, 0x1a}, bf<GC_88_DL_6c_Block>(&Gameplay::gc_88_dl_6c), "GC 88 DL 6c"},
	{{NONE, NONE}, {0x80, NONE}, {0x64, 0x1b}, bf<GC_80_DL_64_Block>(&Gameplay::gc_80_dl_64), "GC 80 DL 64"},
	{{NONE, NONE}, {0x7c, NONE}, {0x60, 0x1c}, bf<GrindRailBlock>(&Gameplay::grindrails), "grindrails"},
	{{NONE, NONE}, {0x98, NONE}, {0x74, 0x1d}, bf<GameplayAreaListBlock>(&Gameplay::gameplay_area_list), "gameplay area list"}
};
