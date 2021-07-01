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

void read_gameplay(Gameplay& gameplay, Buffer src, Game game, const std::vector<GameplayBlockDescription>& blocks) {
	for(const GameplayBlockDescription& block : blocks) {
		s32 block_offset = src.read<s32>(block.header_pointer_offset, "gameplay header");
		if(block_offset != 0 && block.funcs.read != nullptr) {
			block.funcs.read(gameplay, src.subbuf(block_offset), game);
		}
	}
}

std::vector<u8> write_gameplay(const Gameplay& gameplay, Game game, const std::vector<GameplayBlockDescription>& blocks) {
	s32 header_size = 0;
	s32 block_count = 0;
	for(const GameplayBlockDescription& block : blocks) {
		header_size = std::max(header_size, block.header_pointer_offset + 4);
		if(block.header_pointer_offset != NONE) {
			block_count++;
		}
	}
	assert(header_size == block_count * 4);
	
	std::vector<u8> dest_vec(header_size, 0);
	OutBuffer dest(dest_vec);
	for(const GameplayBlockDescription& block : blocks) {
		if(block.header_pointer_offset != NONE && block.funcs.write != nullptr) {
			if(strcmp(block.name, "us english strings") != 0) {
				dest.pad(0x10, 0);
			}
			if(strcmp(block.name, "occlusion clusters") == 0) {
				dest.pad(0x40, 0);
			}
			s32 ofs = (s32) dest_vec.size();
			if(block.funcs.write(dest, gameplay, game)) {
				assert(block.header_pointer_offset + 4 <= (s32) dest_vec.size());
				*(s32*) &dest_vec[block.header_pointer_offset] = ofs;
				if(strcmp(block.name, "art instance shrub groups") == 0
					&& gameplay.shrub_groups.has_value()
					&& gameplay.shrub_groups->second_part.size() > 0) {
					dest.pad(0x40, 0);
				}
			}
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
	static void read(std::vector<T>& dest, Buffer src, Game game) {
		auto header = src.read<TableHeader>(0, "table header");
		verify(header.pad[0] == 0, "TableBlock contains more than one table.");
		dest = src.read_multiple<T>(0x10, header.count_1, "table body").copy();
	}
	
	static void write(OutBuffer dest, const std::vector<T>& src, Game game) {
		TableHeader header = {(s32) src.size()};
		dest.write(header);
		for(const T& elem : src) {
			dest.write(elem);
		}
	}
};

struct PropertiesBlock {
	static void read(GpProperties& dest, Buffer src, Game game) {
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
		if(game == Game::DL) {
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
	}
	
	static void write(OutBuffer dest, const GpProperties& src, Game game) {
		dest.write(src.first_part);
		if(src.second_part.size() > 0) {
			dest.write_multiple(src.second_part);
		} else {
			GpPropertiesSecondPart terminator = {0};
			dest.write(terminator);
		}
		dest.write(src.core_sounds_count);
		if(game == Game::DL) {
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
	static void read(std::vector<GpString>& dest, Buffer src, Game game) {
		auto& header = src.read<StringBlockHeader>(0, "string block header");
		auto table = src.read_multiple<StringTableEntry>(8, header.string_count, "string table");
		
		if(game == Game::RAC3 || game == Game::DL) {
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
	
	static void write(OutBuffer dest, const std::vector<GpString>& src, Game game) {
		s64 header_ofs = dest.alloc<StringBlockHeader>();
		s64 table_ofs = dest.alloc_multiple<StringTableEntry>(src.size());
		
		s64 base_ofs;
		if(game == Game::RAC3 || game == Game::DL) {
			base_ofs = table_ofs;
		} else {
			base_ofs = header_ofs;
		}
		
		s64 entry_ofs = table_ofs;
		for(const GpString& string : src) {
			StringTableEntry entry {0};
			if(string.string) {
				entry.offset = dest.tell() - base_ofs;
			}
			entry.id = string.id;
			entry.unknown_6 = string.unknown_6;
			entry.unknown_8 = string.unknown_8;
			entry.unknown_c = string.unknown_c;
			entry.unknown_e = string.unknown_e;
			dest.write(entry_ofs, entry);
			entry_ofs += sizeof(StringTableEntry);
			if(string.string) {
				for(char c : *string.string) {
					dest.write(c);
				}
				dest.write('\0');
				if(game == Game::RAC1 || game == Game::RAC2) {
					dest.pad(0x4, 0);
				}
			}
		}
		StringBlockHeader header;
		header.string_count = src.size();
		header.size = dest.tell() - base_ofs;
		dest.write(header_ofs, header);
	}
};

template <typename Instance, typename Packed>
struct InstanceBlock {
	static void read(std::vector<Instance>& dest, Buffer src, Game game) {
		TableHeader header = src.read<TableHeader>(0, "instance block header");
		auto entries = src.read_multiple<Packed>(0x10, header.count_1, "instances");
		for(Packed packed : entries) {
			Instance inst;
			swap_instance(inst, packed);
			dest.push_back(inst);
		}
	}
	
	static void write(OutBuffer dest, const std::vector<Instance>& src, Game game) {
		TableHeader header = {(s32) src.size()};
		dest.write(header);
		for(Instance camera : src) {
			Packed packed;
			swap_instance(camera, packed);
			dest.write(packed);
		}
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

struct ClassBlock {
	static void read(std::vector<s32>& dest, Buffer src, Game game) {
		s32 count = src.read<s32>(0, "class count");
		dest = src.read_multiple<s32>(4, count, "class data").copy();
	}
	
	static void write(OutBuffer dest, const std::vector<s32>& src, Game game) {
		dest.write((s32) src.size());
		dest.write_multiple(src);
	}
};

packed_struct(MobyBlockHeader,
	s32 static_count;
	s32 dynamic_count;
	s32 pad[2];
)

packed_struct(MobyInstanceRAC23,
	s32 size;        // 0x0 Always 0x88.
	s32 mission;     // 0x4
	s32 unknown_8;   // 0x8
	s32 unknown_c;   // 0xc
	s32 uid;         // 0x10
	s32 bolts;       // 0x14
	s32 unknown_18;  // 0x18
	s32 unknown_1c;  // 0x1c
	s32 unknown_20;  // 0x20
	s32 unknown_24;  // 0x24
	s32 o_class;     // 0x28
	f32 scale;       // 0x2c
	s32 draw_dist;   // 0x30
	s32 update_dist; // 0x34
	s32 unknown_38;  // 0x38
	s32 unknown_3c;  // 0x3c
	Vec3f position;  // 0x40
	Vec3f rotation;  // 0x4c
	s32 group;       // 0x58
	s32 is_rooted;   // 0x5c
	f32 rooted_dist; // 0x60
	s32 unknown_4c;  // 0x64
	s32 pvar_index;  // 0x68
	s32 occlusion;   // 0x6c
	s32 mode_bits;   // 0x70
	Rgb96 light_col; // 0x74
	s32 light;       // 0x80
	s32 unknown_84;  // 0x84
)

struct RAC23MobyBlock {
	static void read(Gameplay& dest, Buffer src, Game game) {
		dest.moby_instances = std::vector<MobyInstance>();
		auto& header = src.read<MobyBlockHeader>(0, "moby block header");
		for(MobyInstanceRAC23 entry : src.read_multiple<MobyInstanceRAC23>(0x10, header.static_count, "moby instances")) {
			verify(entry.size == 0x88, "Moby size field has invalid value.");
			
			MobyInstance instance;
			swap_moby(instance, entry);
			dest.moby_instances->push_back(instance);
		}
		dest.dynamic_moby_count = header.dynamic_count;
	}
	
	static bool write(OutBuffer dest, const Gameplay& src, Game game) {
		assert(src.moby_instances.has_value());
		assert(src.dynamic_moby_count.has_value());
		MobyBlockHeader header = {0};
		header.static_count = src.moby_instances->size();
		header.dynamic_count = *src.dynamic_moby_count;
		dest.write(header);
		for(MobyInstance instance : *src.moby_instances) {
			MobyInstanceRAC23 entry;
			swap_moby(instance, entry);
			dest.write(entry);
		}
		return true;
	}
	
	static void swap_moby(MobyInstance& l, MobyInstanceRAC23& r) {
		SWAP_PACKED(l.size, r.size);
		SWAP_PACKED(l.mission, r.mission);
		SWAP_PACKED(l.rac23.unknown_8, r.unknown_8);
		SWAP_PACKED(l.rac23.unknown_c, r.unknown_c);
		SWAP_PACKED(l.uid, r.uid);
		SWAP_PACKED(l.bolts, r.bolts);
		SWAP_PACKED(l.rac23.unknown_18, r.unknown_18);
		SWAP_PACKED(l.rac23.unknown_1c, r.unknown_1c);
		SWAP_PACKED(l.rac23.unknown_20, r.unknown_20);
		SWAP_PACKED(l.rac23.unknown_24, r.unknown_24);
		SWAP_PACKED(l.o_class, r.o_class);
		SWAP_PACKED(l.scale, r.scale);
		SWAP_PACKED(l.draw_dist, r.draw_dist);
		SWAP_PACKED(l.update_dist, r.update_dist);
		SWAP_PACKED(l.rac23.unknown_38, r.unknown_38);
		SWAP_PACKED(l.rac23.unknown_3c, r.unknown_3c);
		SWAP_PACKED(l.position, r.position);
		SWAP_PACKED(l.rotation, r.rotation);
		SWAP_PACKED(l.group, r.group);
		SWAP_PACKED(l.is_rooted, r.is_rooted);
		SWAP_PACKED(l.rooted_dist, r.rooted_dist);
		SWAP_PACKED(l.rac23.unknown_4c, r.unknown_4c);
		SWAP_PACKED(l.pvar_index, r.pvar_index);
		SWAP_PACKED(l.occlusion, r.occlusion);
		SWAP_PACKED(l.mode_bits, r.mode_bits);
		SWAP_PACKED(l.light_col, r.light_col);
		SWAP_PACKED(l.light, r.light);
		SWAP_PACKED(l.rac23.unknown_84, r.unknown_84);
	}
};

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
	s32 occlusion;   // 0x54
	s32 mode_bits;   // 0x58
	Rgb96 light_col; // 0x5c
	s32 light;       // 0x68
	s32 unknown_6c;  // 0x6c
)
static_assert(sizeof(MobyInstanceDL) == 0x70);

struct DeadlockedMobyBlock {
	static void read(Gameplay& dest, Buffer src, Game game) {
		dest.moby_instances = std::vector<MobyInstance>();
		auto& header = src.read<MobyBlockHeader>(0, "moby block header");
		for(MobyInstanceDL entry : src.read_multiple<MobyInstanceDL>(0x10, header.static_count, "moby instances")) {
			verify(entry.size == 0x70, "Moby size field has invalid value.");
			verify(entry.unknown_20 == 32, "Moby field has weird value.");
			verify(entry.unknown_24 == 64, "Moby field has weird value.");
			verify(entry.unknown_4c == 1, "Moby field has weird value.");
			verify(entry.unknown_6c == -1, "Moby field has weird value.");
			
			MobyInstance instance;
			swap_moby(instance, entry);
			dest.moby_instances->push_back(instance);
		}
		dest.dynamic_moby_count = header.dynamic_count;
	}
	
	static bool write(OutBuffer dest, const Gameplay& src, Game game) {
		assert(src.moby_instances.has_value());
		assert(src.dynamic_moby_count.has_value());
		MobyBlockHeader header = {0};
		header.static_count = src.moby_instances->size();
		header.dynamic_count = *src.dynamic_moby_count;
		dest.write(header);
		for(MobyInstance instance : *src.moby_instances) {
			MobyInstanceDL entry;
			swap_moby(instance, entry);
			dest.write(entry);
		}
		return true;
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
		r.unknown_20 = 32;
		r.unknown_24 = 64;
		SWAP_PACKED(l.position, r.position);
		SWAP_PACKED(l.rotation, r.rotation);
		SWAP_PACKED(l.group, r.group);
		SWAP_PACKED(l.is_rooted, r.is_rooted);
		SWAP_PACKED(l.rooted_dist, r.rooted_dist);
		r.unknown_4c = 1;
		SWAP_PACKED(l.pvar_index, r.pvar_index);
		SWAP_PACKED(l.occlusion, r.occlusion);
		SWAP_PACKED(l.mode_bits, r.mode_bits);
		SWAP_PACKED(l.light_col, r.light_col);
		SWAP_PACKED(l.light, r.light);
		r.unknown_6c = -1;
	}
};

struct PvarTableBlock {
	static void read(Gameplay& dest, Buffer src, Game game) {
		s32 pvar_count = 0;
		if(dest.cameras.has_value()) {
			for(const ImportCamera& camera : *dest.cameras) {
				pvar_count = std::max(pvar_count, camera.pvar_index + 1);
			}
		}
		if(dest.sound_instances.has_value()) {
			for(const SoundInstance& inst : *dest.sound_instances) {
				pvar_count = std::max(pvar_count, inst.pvar_index + 1);
			}
		}
		if(dest.moby_instances.has_value()) {
			for(const MobyInstance& inst : *dest.moby_instances) {
				pvar_count = std::max(pvar_count, inst.pvar_index + 1);
			}
		}
		
		// Store the table for use in PvarDataBlock::read.
		dest.pvars_temp = src.read_multiple<PvarTableEntry>(0, pvar_count, "pvar table").copy();
	}
	
	static bool write(OutBuffer dest, const Gameplay& src, Game game) {
		s32 data_offset = 0;
		if(src.cameras.has_value()) {
			for(const ImportCamera& camera : *src.cameras) {
				if(camera.pvars.size() > 0) {
					dest.write(data_offset);
					dest.write((s32) camera.pvars.size());
					data_offset += camera.pvars.size();
				}
			}
		}
		if(src.sound_instances.has_value()) {
			for(const SoundInstance& inst : *src.sound_instances) {
				if(inst.pvars.size() > 0) {
					dest.write(data_offset);
					dest.write((s32) inst.pvars.size());
					data_offset += inst.pvars.size();
				}
			}
		}
		if(src.moby_instances.has_value()) {
			for(const MobyInstance& inst : *src.moby_instances) {
				if(inst.pvars.size() > 0) {
					dest.write(data_offset);
					dest.write((s32) inst.pvars.size());
					data_offset += inst.pvars.size();
				}
			}
		}
		return true;
	}
};

struct PvarDataBlock {
	static void read(Gameplay& dest, Buffer src, Game game) {
		assert(dest.pvars_temp.has_value());
		if(dest.cameras.has_value()) {
			for(ImportCamera& camera : *dest.cameras) {
				if(camera.pvar_index >= 0) {
					PvarTableEntry& entry = dest.pvars_temp->at(camera.pvar_index);
					camera.pvars = src.read_multiple<u8>(entry.offset, entry.size, "camera pvar data").copy();
				}
			}
		}
		if(dest.sound_instances.has_value()) {
			for(SoundInstance& inst : *dest.sound_instances) {
				if(inst.pvar_index >= 0) {
					PvarTableEntry& entry = dest.pvars_temp->at(inst.pvar_index);
					inst.pvars = src.read_multiple<u8>(entry.offset, entry.size, "sound instance pvar data").copy();
				}
			}
		}
		if(dest.moby_instances.has_value()) {
			for(MobyInstance& inst : *dest.moby_instances) {
				if(inst.pvar_index >= 0) {
					PvarTableEntry& entry = dest.pvars_temp->at(inst.pvar_index);
					inst.pvars = src.read_multiple<u8>(entry.offset, entry.size, "moby pvar data").copy();
				}
			}
		}
		
		dest.pvars_temp = {};
	}
	
	static bool write(OutBuffer dest, const Gameplay& src, Game game) {
		if(src.cameras.has_value()) {
			for(const ImportCamera& camera : *src.cameras) {
				dest.write_multiple(camera.pvars);
			}
		}
		if(src.sound_instances.has_value()) {
			for(const SoundInstance& inst : *src.sound_instances) {
				dest.write_multiple(inst.pvars);
			}
		}
		if(src.moby_instances.has_value()) {
			for(const MobyInstance& inst : *src.moby_instances) {
				dest.write_multiple(inst.pvars);
			}
		}
		return true;
	}
};

template <typename T>
struct TerminatedArrayBlock {
	static void read(std::vector<T>& dest, Buffer src, Game game) {
		for(s64 offset = 0; src.read<s32>(offset, "array element") > -1; offset += sizeof(T)) {
			dest.emplace_back(src.read<T>(offset, "array element"));
		}
	}
	
	static void write(OutBuffer dest, const std::vector<T>& src, Game game) {
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
	
	static void read(T& dest, Buffer src, Game game) {
		auto header = src.read<DualTableHeader>(0, "dual table header");
		verify(header.pad[0] == 0, "DualTableBlock contains more than two tables.");
		dest.first_part = src.read_multiple<FirstType>(0x10, header.count_1, "table body").copy();
		s64 first_part_size = header.count_1 * sizeof(FirstType);
		if(first_part_size % 0x10 != 0) {
			first_part_size += 0x10 - (first_part_size % 0x10);
		}
		dest.second_part = src.read_multiple<SecondType>(0x10 + first_part_size, header.count_2, "table body").copy();
	}
	
	static void write(OutBuffer dest, const T& src, Game game) {
		DualTableHeader header = {0};
		header.count_1 = src.first_part.size();
		header.count_2 = src.second_part.size();
		dest.write(header);
		for(const FirstType& elem : src.first_part) {
			dest.write(elem);
		}
		dest.pad(0x10, 0);
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

struct PathBlock {
	static void read(std::vector<std::vector<Vec4f>>& dest, Buffer src, Game game) {
		auto& header = src.read<SplineBlockHeader>(0, "spline block header");
		dest = read_splines(src.subbuf(0x10), header.spline_count, header.data_offset - 0x10);
	}
	
	static void write(OutBuffer dest, const std::vector<std::vector<Vec4f>>& src, Game game) {
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
	static void read(std::vector<u8>& dest, Buffer src, Game game) {
		s32 size = src.read<s32>(0, "block size");
		dest = src.read_multiple<u8>(4, size, "block data").copy();
	}
	
	static void write(OutBuffer dest, const std::vector<u8>& src, Game game) {
		dest.write((s32) src.size());
		dest.write_multiple(src);
	}
};

struct GC_80_DL_64_Block {
	static void read(Gp_GC_80_DL_64& dest, Buffer src, Game game) {
		auto header = src.read<TableHeader>(0, "block header");
		dest.first_part = src.read_multiple<u8>(0x10, 0x800, "first part of block").copy();
		dest.second_part = src.read_multiple<u8>(0x810, header.count_1 * 0x10, "second part of block").copy();
	}
	
	static void write(OutBuffer dest, const Gp_GC_80_DL_64& src, Game game) {
		TableHeader header {(s32) src.second_part.size() / 0x10};
		dest.write(header);
		dest.write_multiple(src.first_part);
		dest.write_multiple(src.second_part);
	}
};

packed_struct(GrindPathData,
	GpBoundingSphere bounding_sphere;
	s32 unknown_4;
	s32 wrap;
	s32 inactive;
	s32 pad = 0;
)

struct GrindPathBlock {
	static void read(std::vector<GrindPath>& dest, Buffer src, Game game) {
		auto& header = src.read<SplineBlockHeader>(0, "spline block header");
		auto grindpaths = src.read_multiple<GrindPathData>(0x10, header.spline_count, "grindrail data");
		s64 offsets_pos = 0x10 + header.spline_count * sizeof(GrindPathData);
		auto splines = read_splines(src.subbuf(offsets_pos), header.spline_count, header.data_offset - offsets_pos);
		for(s64 i = 0; i < header.spline_count; i++) {
			GrindPath path;
			path.bounding_sphere = grindpaths[i].bounding_sphere;
			path.unknown_4 = grindpaths[i].unknown_4;
			path.wrap = grindpaths[i].wrap;
			path.inactive = grindpaths[i].inactive;
			path.vertices = splines[i];
			dest.emplace_back(std::move(path));
		}
	}
	
	static void write(OutBuffer dest, const std::vector<GrindPath>& src, Game game) {
		s64 header_pos = dest.alloc<SplineBlockHeader>();
		std::vector<std::vector<Vec4f>> splines;
		for(const GrindPath& path : src) {
			GrindPathData packed;
			packed.bounding_sphere = path.bounding_sphere;
			packed.unknown_4 = path.unknown_4;
			packed.wrap = path.wrap;
			packed.inactive = path.inactive;
			dest.write(packed);
			splines.emplace_back(path.vertices);
		}
		SplineBlockHeader header = {0};
		header.spline_count = src.size();
		header.data_offset = write_splines(dest, splines);
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
	GpBoundingSphere bounding_sphere;
	s16 part_counts[5];
	s16 last_update_time;
	s32 relative_part_offsets[5];
)

struct GameplayAreaListBlock {
	static void read(std::vector<GpArea>& dest, Buffer src, Game game) {
		src = src.subbuf(4); // Skip past size field.
		s64 header_size = sizeof(GameplayAreaListHeader);
		auto header = src.read<GameplayAreaListHeader>(0, "area list block header");
		auto entries = src.read_multiple<GameplayAreaPacked>(header_size, header.area_count, "area list table");
		for(const GameplayAreaPacked& entry : entries) {
			GpArea area;
			area.bounding_sphere = entry.bounding_sphere;
			area.last_update_time = entry.last_update_time;
			for(s32 part = 0; part < 5; part++) {
				s32 part_ofs = header.part_offsets[part] + entry.relative_part_offsets[part];
				area.parts[part] = src.read_multiple<s32>(part_ofs, entry.part_counts[part], "area list data").copy();
			}
			dest.emplace_back(std::move(area));
		}
	}
	
	static void write(OutBuffer dest, const std::vector<GpArea>& src, Game game) {
		s64 size_ofs = dest.alloc<s32>();
		s64 header_ofs = dest.alloc<GameplayAreaListHeader>();
		s64 table_ofs = dest.alloc_multiple<GameplayAreaPacked>(src.size());
		
		s64 total_part_counts[5] = {0, 0, 0, 0, 0};
		
		GameplayAreaListHeader header;
		std::vector<GameplayAreaPacked> table;
		for(const GpArea& area : src) {
			GameplayAreaPacked packed;
			packed.bounding_sphere = area.bounding_sphere;
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

struct TieAmbientRgbaBlock {
	static void read(std::vector<GpTieAmbientRgbas>& dest, Buffer src, Game game) {
		s64 ofs = 0;
		for(;;) {
			s16 id = src.read<s16>(ofs, "index");
			ofs += 2;
			if(id == -1) {
				break;
			}
			s64 size = ((s64) src.read<s16>(ofs, "size")) * 2;
			ofs += 2;
			GpTieAmbientRgbas part;
			part.id = id;
			part.data = src.read_multiple<u8>(ofs, size, "tie rgba data").copy();
			dest.emplace_back(std::move(part));
			ofs += size;
		}
	}
	
	static void write(OutBuffer dest, const std::vector<GpTieAmbientRgbas>& src, Game game) {
		for(const GpTieAmbientRgbas& part : src) {
			dest.write(part.id);
			assert(part.data.size() % 2 == 0);
			dest.write<s16>(part.data.size() / 2);
			dest.write_multiple(part.data);
		}
		dest.write<s16>(-1);
	}
};

packed_struct(OcclusionHeader,
	s32 count_1;
	s32 count_2;
	s32 count_3;
	s32 pad = 0;
)

struct OcclusionBlock {
	static void read(OcclusionClusters& dest, Buffer src, Game game) {
		auto& header = src.read<OcclusionHeader>(0, "occlusion header");
		s64 ofs = 0x10;
		dest.first_part = src.read_multiple<OcclusionPair>(ofs, header.count_1, "first part of occlusion").copy();
		ofs += header.count_1 * sizeof(OcclusionPair);
		dest.second_part = src.read_multiple<OcclusionPair>(ofs, header.count_2, "second part of occlusion").copy();
		ofs += header.count_2 * sizeof(OcclusionPair);
		dest.third_part = src.read_multiple<OcclusionPair>(ofs, header.count_3, "third part of occlusion").copy();
	}
	
	static void write(OutBuffer dest, const OcclusionClusters& src, Game game) {
		OcclusionHeader header;
		header.count_1 = (s32) src.first_part.size();
		header.count_2 = (s32) src.second_part.size();
		header.count_3 = (s32) src.third_part.size();
		dest.write(header);
		dest.write_multiple(src.first_part);
		dest.write_multiple(src.second_part);
		dest.write_multiple(src.third_part);
		dest.pad(0x40, 0);
	}
};

template <typename Block, typename Field>
static GameplayBlockFuncs bf(Field field) {
	// e.g. if field = &Gameplay::moby_instances then FieldType = std::vector<MobyInstance>.
	using FieldType = std::remove_reference<decltype(Gameplay().*field)>::type::value_type;
	
	GameplayBlockFuncs funcs;
	funcs.read = [field](Gameplay& gameplay, Buffer src, Game game) {
		FieldType value;
		Block::read(value, src, game);
		gameplay.*field = std::move(value);
	};
	funcs.write = [field](OutBuffer dest, const Gameplay& gameplay, Game game) {
		if(!(gameplay.*field).has_value()) {
			return false;
		}
		Block::write(dest, *(gameplay.*field), game);
		return true;
	};
	return funcs;
}

const std::vector<GameplayBlockDescription> RAC23_GAMEPLAY_BLOCKS = {
	{0x8c, bf<TableBlock<Gp_GC_8c_DL_70>>(&Gameplay::gc_8c_dl_70), "GC 8c DL 70"},
	{0x00, bf<PropertiesBlock>(&Gameplay::properties), "properties"},
	{0x10, bf<StringBlock>(&Gameplay::us_english_strings), "us english strings"},
	{0x14, bf<StringBlock>(&Gameplay::uk_english_strings), "uk english strings"},
	{0x18, bf<StringBlock>(&Gameplay::french_strings), "french strings"},
	{0x1c, bf<StringBlock>(&Gameplay::german_strings), "german strings"},
	{0x20, bf<StringBlock>(&Gameplay::spanish_strings), "spanish strings"},
	{0x24, bf<StringBlock>(&Gameplay::italian_strings), "italian strings"},
	{0x28, bf<StringBlock>(&Gameplay::japanese_strings), "japanese strings"},
	{0x2c, bf<StringBlock>(&Gameplay::korean_strings), "korean strings"},
	{0x04, bf<TableBlock<GpDirectionalLight>>(&Gameplay::lights), "directional lights"},
	{0x84, bf<TableBlock<GC_84_Instance>>(&Gameplay::gc_84), "GC 84"},
	{0x08, bf<InstanceBlock<ImportCamera, ImportCameraPacked>>(&Gameplay::cameras), "import cameras"},
	{0x0c, bf<InstanceBlock<SoundInstance, SoundInstancePacked>>(&Gameplay::sound_instances), "sound instances"},
	{0x48, bf<ClassBlock>(&Gameplay::moby_classes), "moby classes"},
	{0x4c, {RAC23MobyBlock::read, RAC23MobyBlock::write}, "moby instances"},
	{0x5c, {PvarTableBlock::read, PvarTableBlock::write}, "pvar table"},
	{0x60, {PvarDataBlock::read, PvarDataBlock::write}, "pvar data"},
	{0x58, bf<TerminatedArrayBlock<Gp_DL_3c>>(&Gameplay::gc_58_dl_3c), "GC 58 DL 3c"},
	{0x64, bf<TerminatedArrayBlock<Gp_GC_64_DL_48>>(&Gameplay::gc_64_dl_48), "GC 64 DL 48"},
	{0x50, bf<DualTableBlock<GpMobyGroups>>(&Gameplay::moby_groups), "moby groups"},
	{0x54, bf<DualTableBlock<Gp_GC_54_DL_38>>(&Gameplay::gc_54_dl_38), "GC 54 DL 38"},
	{0x30, bf<ClassBlock>(&Gameplay::tie_classes), "tie classes"},
	{0x34, bf<TableBlock<GpTieInstance>>(&Gameplay::tie_instances), "tie instances"},
	{0x94, bf<TieAmbientRgbaBlock>(&Gameplay::tie_ambient_rgbas), "tie ambient rgbas"},
	{0x38, bf<DualTableBlock<GpTieGroups>>(&Gameplay::tie_groups), "tie groups"},
	{0x3c, bf<ClassBlock>(&Gameplay::shrub_classes), "shrub classes"},
	{0x40, bf<TableBlock<GpShrubInstance>>(&Gameplay::shrub_instances), "shrub instances"},
	{0x44, bf<DualTableBlock<GpShrubGroups>>(&Gameplay::shrub_groups), "shrub groups"},
	{0x78, bf<PathBlock>(&Gameplay::paths), "paths"},
	{0x68, bf<TableBlock<GpShape>>(&Gameplay::cuboids), "cuboids"},
	{0x6c, bf<TableBlock<GpShape>>(&Gameplay::spheres), "spheres"},
	{0x70, bf<TableBlock<GpShape>>(&Gameplay::cylinders), "cylinders"},
	{0x74, bf<TableBlock<s32>>(&Gameplay::gc_74_dl_58), "GC 74 DL 58"},
	{0x88, bf<GC_88_DL_6c_Block>(&Gameplay::gc_88_dl_6c), "GC 88 DL 6c"},
	{0x80, bf<GC_80_DL_64_Block>(&Gameplay::gc_80_dl_64), "GC 80 DL 64"},
	{0x7c, bf<GrindPathBlock>(&Gameplay::grindpaths), "grindpaths"},
	{0x98, bf<GameplayAreaListBlock>(&Gameplay::gameplay_area_list), "gameplay area list"},
	{0x90, bf<OcclusionBlock>(&Gameplay::occlusion_clusters), "occlusion clusters"}
};

const std::vector<GameplayBlockDescription> DL_GAMEPLAY_CORE_BLOCKS = {
	{0x70, bf<TableBlock<Gp_GC_8c_DL_70>>(&Gameplay::gc_8c_dl_70), "GC 8c DL 70"},
	{0x00, bf<PropertiesBlock>(&Gameplay::properties), "properties"},
	{0x0c, bf<StringBlock>(&Gameplay::us_english_strings), "us english strings"},
	{0x10, bf<StringBlock>(&Gameplay::uk_english_strings), "uk english strings"},
	{0x14, bf<StringBlock>(&Gameplay::french_strings), "french strings"},
	{0x18, bf<StringBlock>(&Gameplay::german_strings), "german strings"},
	{0x1c, bf<StringBlock>(&Gameplay::spanish_strings), "spanish strings"},
	{0x20, bf<StringBlock>(&Gameplay::italian_strings), "italian strings"},
	{0x24, bf<StringBlock>(&Gameplay::japanese_strings), "japanese strings"},
	{0x28, bf<StringBlock>(&Gameplay::korean_strings), "korean strings"},
	{0x04, bf<InstanceBlock<ImportCamera, ImportCameraPacked>>(&Gameplay::cameras), "import cameras"},
	{0x08, bf<InstanceBlock<SoundInstance, SoundInstancePacked>>(&Gameplay::sound_instances), "sound instances"},
	{0x2c, bf<ClassBlock>(&Gameplay::moby_classes), "moby classes"},
	{0x30, {DeadlockedMobyBlock::read, DeadlockedMobyBlock::write}, "moby instances"},
	{0x40, {PvarTableBlock::read, PvarTableBlock::write}, "pvar table"},
	{0x44, {PvarDataBlock::read, PvarDataBlock::write}, "pvar data"},
	{0x3c, bf<TerminatedArrayBlock<Gp_DL_3c>>(&Gameplay::gc_58_dl_3c), "GC 58 DL 3c"},
	{0x48, bf<TerminatedArrayBlock<Gp_GC_64_DL_48>>(&Gameplay::gc_64_dl_48), "GC 64 DL 48"},
	{0x34, bf<DualTableBlock<GpMobyGroups>>(&Gameplay::moby_groups), "moby groups"},
	{0x38, bf<DualTableBlock<Gp_GC_54_DL_38>>(&Gameplay::gc_54_dl_38), "GC 54 DL 38"},
	{0x5c, bf<PathBlock>(&Gameplay::paths), "paths"},
	{0x4c, bf<TableBlock<GpShape>>(&Gameplay::cuboids), "cuboids"},
	{0x50, bf<TableBlock<GpShape>>(&Gameplay::spheres), "spheres"},
	{0x54, bf<TableBlock<GpShape>>(&Gameplay::cylinders), "cylinders"},
	{0x58, bf<TableBlock<s32>>(&Gameplay::gc_74_dl_58), "GC 74 DL 58"},
	{0x6c, bf<GC_88_DL_6c_Block>(&Gameplay::gc_88_dl_6c), "GC 88 DL 6c"},
	{0x64, bf<GC_80_DL_64_Block>(&Gameplay::gc_80_dl_64), "GC 80 DL 64"},
	{0x60, bf<GrindPathBlock>(&Gameplay::grindpaths), "grindpaths"},
	{0x74, bf<GameplayAreaListBlock>(&Gameplay::gameplay_area_list), "gameplay area list"},
	{0x68, {nullptr, nullptr}, "pad"}
};

const std::vector<GameplayBlockDescription> DL_ART_INSTANCE_BLOCKS = {
	{0x00, bf<TableBlock<GpDirectionalLight>>(&Gameplay::lights), "directional lights"},
	{0x04, bf<ClassBlock>(&Gameplay::tie_classes), "tie classes"},
	{0x08, bf<TableBlock<GpTieInstance>>(&Gameplay::tie_instances), "tie instances"},
	{0x20, bf<TieAmbientRgbaBlock>(&Gameplay::tie_ambient_rgbas), "tie ambient rgbas"},
	{0x0c, bf<DualTableBlock<GpTieGroups>>(&Gameplay::tie_groups), "tie groups"},
	{0x10, bf<ClassBlock>(&Gameplay::shrub_classes), "shrub classes"},
	{0x14, bf<TableBlock<GpShrubInstance>>(&Gameplay::shrub_instances), "shrub instances"},
	{0x18, bf<DualTableBlock<GpShrubGroups>>(&Gameplay::shrub_groups), "art instance shrub groups"},
	{0x1c, bf<OcclusionBlock>(&Gameplay::occlusion_clusters), "occlusion clusters"},
	{0x24, {nullptr, nullptr}, "pad 1"},
	{0x28, {nullptr, nullptr}, "pad 2"},
	{0x2c, {nullptr, nullptr}, "pad 3"},
	{0x30, {nullptr, nullptr}, "pad 4"},
	{0x34, {nullptr, nullptr}, "pad 5"},
	{0x38, {nullptr, nullptr}, "pad 6"},
	{0x3c, {nullptr, nullptr}, "pad 7"}
};

const std::vector<GameplayBlockDescription> DL_GAMEPLAY_MISSION_INSTANCE_BLOCKS = {
	{0x00, bf<ClassBlock>(&Gameplay::moby_classes), "moby classes"},
	{0x04, {DeadlockedMobyBlock::read, DeadlockedMobyBlock::write}, "moby instances"},
	{0x14, {PvarTableBlock::read, PvarTableBlock::write}, "pvar table"},
	{0x18, {PvarDataBlock::read, PvarDataBlock::write}, "pvar data"},
	{0x10, bf<TerminatedArrayBlock<Gp_DL_3c>>(&Gameplay::gc_58_dl_3c), "GC 58 DL 3c"},
	{0x1c, bf<TerminatedArrayBlock<Gp_GC_64_DL_48>>(&Gameplay::gc_64_dl_48), "GC 64 DL 48"},
	{0x08, bf<DualTableBlock<GpMobyGroups>>(&Gameplay::moby_groups), "moby groups"},
	{0x0c, bf<DualTableBlock<Gp_GC_54_DL_38>>(&Gameplay::gc_54_dl_38), "GC 54 DL 38"}
};
