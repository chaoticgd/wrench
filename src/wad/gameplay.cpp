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

static void rewire_pvar_indices(Gameplay& gameplay);
static void rewire_global_pvar_pointers(const LevelWad& wad, Gameplay& gameplay);

void read_gameplay(LevelWad& wad, Gameplay& gameplay, Buffer src, Game game, const std::vector<GameplayBlockDescription>& blocks) {
	for(const GameplayBlockDescription& block : blocks) {
		s32 block_offset = src.read<s32>(block.header_pointer_offset, "gameplay header");
		if(block_offset != 0 && block.funcs.read != nullptr) {
			block.funcs.read(wad, gameplay, src.subbuf(block_offset), game);
		}
	}
}

std::vector<u8> write_gameplay(const LevelWad& wad, const Gameplay& gameplay_arg, Game game, const std::vector<GameplayBlockDescription>& blocks) {
	Gameplay gameplay = gameplay_arg;
	
	s32 header_size = 0;
	s32 block_count = 0;
	for(const GameplayBlockDescription& block : blocks) {
		header_size = std::max(header_size, block.header_pointer_offset + 4);
		if(block.header_pointer_offset != NONE) {
			block_count++;
		}
	}
	assert(header_size == block_count * 4);
	
	rewire_pvar_indices(gameplay); // Set pvar_index fields.
	rewire_global_pvar_pointers(wad, gameplay); // Extract global pvar pointers from pvars.
	
	std::vector<u8> dest_vec(header_size, 0);
	OutBuffer dest(dest_vec);
	for(const GameplayBlockDescription& block : blocks) {
		if(block.header_pointer_offset != NONE && block.funcs.write != nullptr) {
			if(strcmp(block.name, "us english help messages") != 0) {
				dest.pad(0x10, 0);
			}
			if(strcmp(block.name, "occlusion") == 0) {
				dest.pad(0x40, 0);
			}
			s32 ofs = (s32) dest_vec.size();
			if(block.funcs.write(dest, wad, gameplay, game)) {
				assert(block.header_pointer_offset + 4 <= (s32) dest_vec.size());
				*(s32*) &dest_vec[block.header_pointer_offset] = ofs;
				if(strcmp(block.name, "art instance shrub groups") == 0
					&& gameplay.shrub_groups.has_value()
					&& gameplay.shrub_groups->size() > 0) {
					dest.pad(0x40, 0);
				}
			}
		}
	}
	return dest_vec;
}

template <typename Packed>
static void swap_matrix(Instance& inst, Packed& packed) {
	glm::mat4 matrix = inst.matrix();
	inst.set_transform(packed.matrix.unpack());
	f32 m33_value = packed.matrix.m_3.w;
	packed.matrix = Mat4::pack(matrix);
	packed.matrix.m_3.w = inst.m33_value_do_not_use();
	inst.m33_value_do_not_use() = m33_value;
}

template <typename Packed>
static void swap_matrix_inverse_rotation(Instance& inst, Packed& packed) {
	glm::mat4 matrix = inst.matrix();
	glm::mat3 inverse_matrix = inst.inverse_matrix();
	glm::vec3 rotation = inst.rotation();
	inst.set_transform(packed.matrix.unpack(), packed.inverse_matrix.unpack(), packed.rotation.unpack());
	f32 m33_value = packed.matrix.m_3.w;
	packed.matrix = Mat4::pack(matrix);
	packed.matrix.m_3.w = inst.m33_value_do_not_use();
	inst.m33_value_do_not_use() = m33_value;
	packed.inverse_matrix = Mat3::pack(inverse_matrix);
	packed.rotation = Vec3f::pack(rotation);
}

template <typename Packed>
static void swap_position_rotation(Instance& instance, Packed& packed) {
	glm::vec3 pos = instance.position();
	glm::vec3 rot = instance.rotation();
	instance.set_transform(packed.position.unpack(), packed.rotation.unpack());
	packed.position = Vec3f::pack(pos);
	packed.rotation = Vec3f::pack(rot);
}

template <typename Packed>
static void swap_position_rotation_scale(Instance& instance, Packed& packed) {
	glm::vec3 pos = instance.position();
	glm::vec3 rot = instance.rotation();
	f32 scale = instance.scale();
	instance.set_transform(packed.position.unpack(), packed.rotation.unpack(), packed.scale);
	packed.position = Vec3f::pack(pos);
	packed.rotation = Vec3f::pack(rot);
	packed.scale = scale;
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
	static void read(Properties& dest, Buffer src, Game game) {
		s32 ofs = 0;
		dest.first_part = src.read<PropertiesFirstPart>(ofs, "gameplay properties");
		ofs += sizeof(PropertiesFirstPart);
		s32 second_part_count = src.read<s32>(ofs + 0xc, "second part count");
		if(second_part_count > 0) {
			dest.second_part = src.read_multiple<PropertiesSecondPart>(ofs, second_part_count, "second part").copy();
			ofs += second_part_count * sizeof(PropertiesSecondPart);
		} else {
			ofs += sizeof(PropertiesSecondPart);
		}
		dest.core_sounds_count = src.read<s32>(ofs, "core sounds count");
		ofs += 4;
		if(game == Game::RAC3) {
			dest.rac3_third_part = src.read<s32>(ofs, "R&C3 third part");
		} else if(game == Game::DL) {
			s64 third_part_count = src.read<s32>(ofs, "third part count");
			ofs += 4;
			if(third_part_count >= 0) {
				dest.third_part = src.read_multiple<PropertiesThirdPart>(ofs, third_part_count, "third part").copy();
				ofs += third_part_count * sizeof(PropertiesThirdPart);
				dest.fourth_part = src.read<PropertiesFourthPart>(ofs, "fourth part");
				ofs += sizeof(PropertiesFourthPart);
			} else {
				ofs += sizeof(PropertiesThirdPart);
			}
			dest.fifth_part = src.read<PropertiesFifthPart>(ofs, "fifth part");
			ofs += sizeof(PropertiesFifthPart);
			dest.sixth_part = src.read_multiple<s8>(ofs, dest.fifth_part->sixth_part_count, "sixth part").copy();
		}
	}
	
	static void write(OutBuffer dest, const Properties& src, Game game) {
		static const char* ERROR_MESSAGE = "Invalid properties block.";
		dest.write(src.first_part);
		if(src.second_part.size() > 0) {
			dest.write_multiple(src.second_part);
		} else {
			PropertiesSecondPart terminator = {0};
			dest.write(terminator);
		}
		dest.write(src.core_sounds_count);
		if(game == Game::RAC3) {
			verify(src.rac3_third_part.has_value(), ERROR_MESSAGE);
			dest.write(*src.rac3_third_part);
		} else if(game == Game::DL) {
			verify(src.third_part.has_value(), ERROR_MESSAGE);
			dest.write((s32) src.third_part->size());
			if(src.third_part->size() > 0) {
				dest.write_multiple(*src.third_part);
				verify(src.fourth_part.has_value(), ERROR_MESSAGE);
				dest.write(*src.fourth_part);
			} else {
				dest.vec.resize(dest.tell() + 0x18, 0);
			}
			verify(src.fifth_part.has_value(), ERROR_MESSAGE);
			dest.write(*src.fifth_part);
			verify(src.sixth_part.has_value(), ERROR_MESSAGE);
			dest.write_multiple(*src.sixth_part);
		}
	}
};

packed_struct(HelpMessageHeader,
	s32 count;
	s32 size;
)

packed_struct(HelpMessageEntry,
	s32 offset;
	s16 id;
	s16 short_id;
	s16 third_person_id;
	s16 coop_id;
	s16 vag;
	s16 character;
)

template <bool is_korean>
struct HelpMessageBlock {
	static void read(std::vector<HelpMessage>& dest, Buffer src, Game game) {
		auto& header = src.read<HelpMessageHeader>(0, "string block header");
		auto table = src.read_multiple<HelpMessageEntry>(8, header.count, "string table");
		
		if(game == Game::RAC3 || game == Game::DL) {
			src = src.subbuf(8);
		}
		
		for(HelpMessageEntry entry : table) {
			HelpMessage message;
			if(entry.offset != 0) {
				message.string = src.read_string(entry.offset, is_korean);
			}
			message.id = entry.id;
			message.short_id = entry.short_id;
			message.third_person_id = entry.third_person_id;
			message.coop_id = entry.coop_id;
			message.vag = entry.vag;
			message.character = entry.character;
			dest.emplace_back(std::move(message));
		}
	}
	
	static void write(OutBuffer dest, const std::vector<HelpMessage>& src, Game game) {
		s64 header_ofs = dest.alloc<HelpMessageHeader>();
		s64 table_ofs = dest.alloc_multiple<HelpMessageEntry>(src.size());
		
		s64 base_ofs;
		if(game == Game::RAC3 || game == Game::DL) {
			base_ofs = table_ofs;
		} else {
			base_ofs = header_ofs;
		}
		
		s64 entry_ofs = table_ofs;
		for(const HelpMessage& message : src) {
			HelpMessageEntry entry {0};
			if(message.string) {
				entry.offset = dest.tell() - base_ofs;
			}
			entry.id = message.id;
			entry.short_id = message.short_id;
			entry.third_person_id = message.third_person_id;
			entry.coop_id = message.coop_id;
			entry.vag = message.vag;
			entry.character = message.character;
			dest.write(entry_ofs, entry);
			entry_ofs += sizeof(HelpMessageEntry);
			if(message.string) {
				for(char c : *message.string) {
					dest.write(c);
				}
				dest.write('\0');
				if(game == Game::RAC1 || game == Game::RAC2) {
					dest.pad(0x4, 0);
				}
			}
		}
		HelpMessageHeader header;
		header.count = src.size();
		header.size = dest.tell() - base_ofs;
		dest.write(header_ofs, header);
	}
};

packed_struct(LightTriggerPacked,
	Mat3 matrix;
	Vec4f point_2;
	s32 unknown_40;
	s32 unknown_44;
	s32 light;
	s32 unknown_4c;
	s32 unknown_50;
	s32 unknown_54;
	s32 unknown_58;
	s32 unknown_5c;
	s32 unknown_60;
	s32 unknown_64;
	s32 unknown_68;
	s32 unknown_6c;
	s32 unknown_70;
	s32 unknown_74;
	s32 unknown_78;
	s32 unknown_7c;
)

struct LightTriggerBlock {
	static void read(std::vector<LightTriggerInstance>& dest, Buffer src, Game game) {
		TableHeader header = src.read<TableHeader>(0, "GC 84 block header");
		dest.resize(header.count_1);
		s64 ofs = 0x10;
		auto points = src.read_multiple<Vec4f>(ofs, header.count_1, "GC 84 points");
		ofs += header.count_1 * sizeof(Vec4f);
		auto data = src.read_multiple<LightTriggerPacked>(ofs, header.count_1, "GC 84 data");
		for(s64 i = 0; i < header.count_1; i++) {
			dest[i].id = i;
			dest[i].point = points[i];
			LightTriggerPacked packed = data[i];
			swap_gc_84(dest[i], packed);
		}
	}
	
	static void write(OutBuffer dest, const std::vector<LightTriggerInstance>& src, Game game) {
		TableHeader header = {(s32) src.size()};
		dest.write(header);
		for(const LightTriggerInstance& inst : src) {
			dest.write(inst.point);
		}
		for(LightTriggerInstance inst : src) {
			LightTriggerPacked packed;
			swap_gc_84(inst, packed);
			dest.write(packed);
		}
	}
	
	static void swap_gc_84(LightTriggerInstance& l, LightTriggerPacked& r) {
		SWAP_PACKED(l.matrix, r.matrix);
		SWAP_PACKED(l.point_2, r.point_2);
		SWAP_PACKED(l.unknown_40, r.unknown_40);
		SWAP_PACKED(l.unknown_44, r.unknown_44);
		SWAP_PACKED(l.light, r.light);
		SWAP_PACKED(l.unknown_4c, r.unknown_4c);
		SWAP_PACKED(l.unknown_50, r.unknown_50);
		SWAP_PACKED(l.unknown_54, r.unknown_54);
		SWAP_PACKED(l.unknown_58, r.unknown_58);
		SWAP_PACKED(l.unknown_5c, r.unknown_5c);
		SWAP_PACKED(l.unknown_60, r.unknown_60);
		SWAP_PACKED(l.unknown_64, r.unknown_64);
		SWAP_PACKED(l.unknown_68, r.unknown_68);
		SWAP_PACKED(l.unknown_6c, r.unknown_6c);
		SWAP_PACKED(l.unknown_70, r.unknown_70);
		SWAP_PACKED(l.unknown_74, r.unknown_74);
		SWAP_PACKED(l.unknown_78, r.unknown_78);
		SWAP_PACKED(l.unknown_7c, r.unknown_7c);
	}
};

template <typename ThisInstance, typename Packed>
struct InstanceBlock {
	static void read(std::vector<ThisInstance>& dest, Buffer src, Game game) {
		TableHeader header = src.read<TableHeader>(0, "instance block header");
		auto entries = src.read_multiple<Packed>(0x10, header.count_1, "instances");
		s32 index = 0;
		for(Packed packed : entries) {
			ThisInstance inst;
			inst.set_id_value(index++);
			swap_instance(inst, packed);
			dest.push_back(inst);
		}
	}
	
	static void write(OutBuffer dest, const std::vector<ThisInstance>& src, Game game) {
		TableHeader header = {(s32) src.size()};
		dest.write(header);
		for(ThisInstance camera : src) {
			Packed packed;
			swap_instance(camera, packed);
			dest.write(packed);
		}
	}
};

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
	/* 0x38 */ s32 unknown_38;
	/* 0x3c */ s32 unknown_3c;
	/* 0x40 */ Vec3f position;
	/* 0x4c */ Vec3f rotation;
	/* 0x58 */ s32 group;
	/* 0x5c */ s32 is_rooted;
	/* 0x60 */ f32 rooted_distance;
	/* 0x64 */ s32 unknown_4c;
	/* 0x68 */ s32 pvar_index;
	/* 0x6c */ s32 occlusion;
	/* 0x70 */ s32 mode_bits;
	/* 0x74 */ Rgb96 light_colour;
	/* 0x80 */ s32 light;
	/* 0x84 */ s32 unknown_84;
)

struct RAC23MobyBlock {
	static void read(LevelWad& wad, Gameplay& gameplay, Buffer src, Game game) {
		auto& header = src.read<MobyBlockHeader>(0, "moby block header");
		gameplay.dynamic_moby_count = header.dynamic_count;
		s32 index = 0;
		gameplay.moby_instances = std::vector<MobyInstance>();
		gameplay.moby_instances->reserve(header.static_count);
		for(MobyInstanceRAC23 entry : src.read_multiple<MobyInstanceRAC23>(0x10, header.static_count, "moby instances")) {
			verify(entry.size == 0x88, "Moby size field has invalid value.");
			MobyInstance instance;
			instance.set_id_value(index++);
			swap_moby(instance, entry);
			gameplay.moby_instances->push_back(instance);
		}
	}
	
	static bool write(OutBuffer dest, const LevelWad& wad, const Gameplay& gameplay, Game game) {
		verify(gameplay.dynamic_moby_count.has_value(), "Missing dynamic moby count field.");
		verify(gameplay.moby_instances.has_value(), "Missing moby instances array.");
		MobyBlockHeader header = {0};
		header.static_count = gameplay.moby_instances->size();
		header.dynamic_count = *gameplay.dynamic_moby_count;
		dest.write(header);
		for(MobyInstance instance : *gameplay.moby_instances) {
			MobyInstanceRAC23 entry;
			swap_moby(instance, entry);
			dest.write(entry);
		}
		return true;
	}
	
	static void swap_moby(MobyInstance& l, MobyInstanceRAC23& r) {
		r.size = 0x88;
		swap_position_rotation_scale(l, r);
		SWAP_PACKED(l.temp_pvar_index(), r.pvar_index);
		SWAP_PACKED(l.draw_distance(), r.draw_distance);
		SWAP_PACKED(l.colour().r, r.light_colour.r);
		SWAP_PACKED(l.colour().g, r.light_colour.g);
		SWAP_PACKED(l.colour().b, r.light_colour.b);
		SWAP_PACKED(l.mission, r.mission);
		SWAP_PACKED(l.rac23_unknown_8, r.unknown_8);
		SWAP_PACKED(l.rac23_unknown_c, r.unknown_c);
		SWAP_PACKED(l.uid, r.uid);
		SWAP_PACKED(l.bolts, r.bolts);
		SWAP_PACKED(l.rac23_unknown_18, r.unknown_18);
		SWAP_PACKED(l.rac23_unknown_1c, r.unknown_1c);
		SWAP_PACKED(l.rac23_unknown_20, r.unknown_20);
		SWAP_PACKED(l.rac23_unknown_24, r.unknown_24);
		SWAP_PACKED(l.o_class, r.o_class);
		SWAP_PACKED(l.update_distance, r.update_distance);
		SWAP_PACKED(l.rac23_unknown_38, r.unknown_38);
		SWAP_PACKED(l.rac23_unknown_3c, r.unknown_3c);
		SWAP_PACKED(l.group, r.group);
		SWAP_PACKED(l.is_rooted, r.is_rooted);
		SWAP_PACKED(l.rooted_distance, r.rooted_distance);
		SWAP_PACKED(l.rac23_unknown_4c, r.unknown_4c);
		SWAP_PACKED(l.occlusion, r.occlusion);
		SWAP_PACKED(l.mode_bits, r.mode_bits);
		SWAP_PACKED(l.light, r.light);
		SWAP_PACKED(l.rac23_unknown_84, r.unknown_84);
	}
};

packed_struct(MobyInstanceDL,
	/* 0x00 */ s32 size; // Always 0x70.
	/* 0x04 */ s32 mission;
	/* 0x08 */ s32 uid;
	/* 0x0c */ s32 bolts;
	/* 0x10 */ s32 o_class;
	/* 0x14 */ f32 scale;
	/* 0x18 */ s32 draw_distance;
	/* 0x1c */ s32 update_distance;
	/* 0x20 */ s32 unknown_20;
	/* 0x24 */ s32 unknown_24;
	/* 0x28 */ Vec3f position;
	/* 0x34 */ Vec3f rotation;
	/* 0x40 */ s32 group;
	/* 0x44 */ s32 is_rooted;
	/* 0x48 */ f32 rooted_distance;
	/* 0x4c */ s32 unknown_4c;
	/* 0x50 */ s32 pvar_index;
	/* 0x54 */ s32 occlusion;
	/* 0x58 */ s32 mode_bits;
	/* 0x5c */ Rgb96 light_colour;
	/* 0x68 */ s32 light;
	/* 0x6c */ s32 unknown_6c;
)
static_assert(sizeof(MobyInstanceDL) == 0x70);

struct DeadlockedMobyBlock {
	static void read(LevelWad& wad, Gameplay& gameplay, Buffer src, Game game) {
		auto& header = src.read<MobyBlockHeader>(0, "moby block header");
		gameplay.dynamic_moby_count = header.dynamic_count;
		gameplay.moby_instances = std::vector<MobyInstance>();
		gameplay.moby_instances->reserve(header.static_count);
		s32 index = 0;
		for(MobyInstanceDL entry : src.read_multiple<MobyInstanceDL>(0x10, header.static_count, "moby instances")) {
			verify(entry.size == 0x70, "Moby size field has invalid value.");
			verify(entry.unknown_20 == 32, "Moby field has weird value.");
			verify(entry.unknown_24 == 64, "Moby field has weird value.");
			verify(entry.unknown_4c == 1, "Moby field has weird value.");
			verify(entry.unknown_6c == -1, "Moby field has weird value.");
			
			MobyInstance instance;
			instance.set_id_value(index++);
			swap_moby(instance, entry);
			gameplay.moby_instances->push_back(instance);
		}
	}
	
	static bool write(OutBuffer dest, const LevelWad& wad, const Gameplay& gameplay, Game game) {
		verify(gameplay.dynamic_moby_count.has_value(), "Missing dynamic moby count field.");
		verify(gameplay.moby_instances.has_value(), "Missing moby instances array.");
		MobyBlockHeader header = {0};
		header.static_count = gameplay.moby_instances->size();
		header.dynamic_count = *gameplay.dynamic_moby_count;
		dest.write(header);
		for(MobyInstance instance : *gameplay.moby_instances) {
			MobyInstanceDL entry;
			swap_moby(instance, entry);
			dest.write(entry);
		}
		return true;
	}
	
	static void swap_moby(MobyInstance& l, MobyInstanceDL& r) {
		r.size = 0x70;
		swap_position_rotation_scale(l, r);
		SWAP_PACKED(l.temp_pvar_index(), r.pvar_index);
		SWAP_PACKED(l.draw_distance(), r.draw_distance);
		SWAP_PACKED(l.colour().r, r.light_colour.r);
		SWAP_PACKED(l.colour().g, r.light_colour.g);
		SWAP_PACKED(l.colour().b, r.light_colour.b);
		SWAP_PACKED(l.mission, r.mission);
		SWAP_PACKED(l.uid, r.uid);
		SWAP_PACKED(l.bolts, r.bolts);
		SWAP_PACKED(l.o_class, r.o_class);
		SWAP_PACKED(l.update_distance, r.update_distance);
		r.unknown_20 = 32;
		r.unknown_24 = 64;
		SWAP_PACKED(l.group, r.group);
		SWAP_PACKED(l.is_rooted, r.is_rooted);
		SWAP_PACKED(l.rooted_distance, r.rooted_distance);
		r.unknown_4c = 1;
		SWAP_PACKED(l.occlusion, r.occlusion);
		SWAP_PACKED(l.mode_bits, r.mode_bits);
		SWAP_PACKED(l.light, r.light);
		r.unknown_6c = -1;
	}
};

struct PvarTableBlock {
	static void read(LevelWad& wad, Gameplay& dest, Buffer src, Game game) {
		s32 pvar_count = 0;
		dest.for_each_pvar_instance([&](Instance& inst) {
			pvar_count = std::max(pvar_count, inst.temp_pvar_index() + 1);
		});
		
		// Store the table for use in PvarDataBlock::read.
		dest.pvars_temp = src.read_multiple<PvarTableEntry>(0, pvar_count, "pvar table").copy();
	}
	
	static bool write(OutBuffer dest, const LevelWad& wad, const Gameplay& src, Game game) {
		s32 data_offset = 0;
		src.for_each_pvar_instance_const([&](const Instance& inst) {
			if(inst.pvars().size() > 0) {
				dest.write(data_offset);
				dest.write((s32) inst.pvars().size());
				data_offset += inst.pvars().size();
			}
		});
		return true;
	}
};

struct PvarDataBlock {
	static void read(LevelWad& wad, Gameplay& dest, Buffer src, Game game) {
		assert(dest.pvars_temp.has_value());
		dest.for_each_pvar_instance([&](Instance& inst) {
			if(inst.temp_pvar_index() >= 0) {
				PvarTableEntry& entry = dest.pvars_temp->at(inst.temp_pvar_index());
				inst.pvars() = src.read_multiple<u8>(entry.offset, entry.size, "pvar data").copy();
			}
		});
		
		dest.pvars_temp = {};
	}
	
	static bool write(OutBuffer dest, const LevelWad& wad, const Gameplay& src, Game game) {
		src.for_each_pvar_instance_const([&](const Instance& inst) {
			dest.write_multiple(inst.pvars());
		});
		return true;
	}
};

packed_struct(PvarPointerEntry,
	s32 pvar_index;
	u32 pointer_offset;
)

static PvarType& get_pvar_type(s32 pvar_index, LevelWad& wad, Gameplay& dest) {
	Opt<std::string> pvar_type_name;
	for(Camera& inst : opt_iterator(dest.cameras)) {
		if(inst.temp_pvar_index() == pvar_index) {
			CameraClass& cls = wad.lookup_camera_class(inst.type);
			return wad.pvar_types[cls.pvar_type];
		}
	}
	for(SoundInstance& inst : opt_iterator(dest.sound_instances)) {
		if(inst.temp_pvar_index() == pvar_index) {
			SoundClass& cls = wad.lookup_sound_class(inst.o_class);
			return wad.pvar_types[cls.pvar_type];
		}
	}
	for(MobyInstance& inst : opt_iterator(dest.moby_instances)) {
		if(inst.temp_pvar_index() == pvar_index) {
			MobyClass& cls = wad.lookup_moby_class(inst.o_class);
			return wad.pvar_types[cls.pvar_type];
		}
	}
	verify_not_reached("Invalid pvar index.");
}

auto write_pvar_pointer_entries(PvarFieldDescriptor descriptor, OutBuffer dest, const LevelWad& wad, const Gameplay& src, Game game) {
	src.for_each_pvar_instance_const(wad, [&](const Instance& inst, const PvarType& type) {
		for(const PvarField& field : type.fields) {
			if(field.descriptor == descriptor && (descriptor == PVAR_RELATIVE_POINTER || Buffer(inst.pvars()).read<s32>(field.offset, "pvar pointer") >= 0)) {
				PvarPointerEntry entry;
				entry.pvar_index = inst.temp_pvar_index();
				entry.pointer_offset = field.offset;
				dest.write(entry);
			}
		}
	});
	
	PvarPointerEntry terminator;
	terminator.pvar_index = -1;
	terminator.pointer_offset = -1;
	dest.write(terminator);
}

struct PvarScratchpadBlock {
	static void read(LevelWad& wad, Gameplay& dest, Buffer src, Game game) {
		for(s64 offset = 0;; offset += sizeof(PvarPointerEntry)) {
			auto& entry = src.read<PvarPointerEntry>(offset, "pvar scratchpad block");
			if(entry.pvar_index < 0) {
				break;
			}
			
			// If the field already exists, this will do nothing.
			PvarType& type = get_pvar_type(entry.pvar_index, wad, dest);
			PvarField field;
			field.offset = entry.pointer_offset;
			field.descriptor = PVAR_SCRATCHPAD_POINTER;
			verify(type.insert_field(field, true), "Conflicting pvar type information.");
		}
	}
	
	static bool write(OutBuffer dest, const LevelWad& wad, const Gameplay& src, Game game) {
		write_pvar_pointer_entries(PVAR_SCRATCHPAD_POINTER, dest, wad, src, game);
		return true;
	}
};

struct PvarPointerRewireBlock {
	static void read(LevelWad& wad, Gameplay& dest, Buffer src, Game game) {
		for(s64 offset = 0;; offset += sizeof(PvarPointerEntry)) {
			auto& entry = src.read<PvarPointerEntry>(offset, "pvar pointer rewire block");
			if(entry.pvar_index < 0) {
				break;
			}
			
			// If the field already exists, this will do nothing.
			PvarType& type = get_pvar_type(entry.pvar_index, wad, dest);
			PvarField field;
			field.offset = entry.pointer_offset;
			field.descriptor = PVAR_RELATIVE_POINTER;
			verify(type.insert_field(field, false), "Conflicting pvar type information.");
		}
	}
	
	static bool write(OutBuffer dest, const LevelWad& wad, const Gameplay& src, Game game) {
		write_pvar_pointer_entries(PVAR_RELATIVE_POINTER, dest, wad, src, game);
		return true;
	}
};

packed_struct(GroupHeader,
	s32 group_count;
	s32 data_size;
	s32 pad[2];
)

struct GroupBlock {
	static void read(std::vector<Group>& dest, Buffer src, Game game) {
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
			Group group;
			group.id = index++;
			s32 member;
			if(pointer >= 0) {
				do {
					member = members[member_index++];
					group.members.push_back(member & 0x7fff);
				} while(!(member & 0x8000));
			}
			dest.emplace_back(std::move(group));
		}
	}
	
	static void write(OutBuffer dest, const std::vector<Group>& src, Game game) {
		s64 header_ofs = dest.alloc<GroupHeader>();
		s64 pointer_ofs = dest.alloc_multiple<s32>(src.size());
		dest.pad(0x10, 0);
		s64 data_ofs = dest.tell();
		
		std::vector<s32> pointers;
		pointers.reserve(src.size());
		for(const Group& group : src) {
			if(group.members.size() > 0) {
				pointers.push_back(dest.tell() - data_ofs);
				for(size_t i = 0; i < group.members.size(); i++) {
					if(i == group.members.size() - 1) {
						dest.write<u16>(group.members[i] | 0x8000);
					} else {
						dest.write<u16>(group.members[i]);
					}
				}
			} else {
				pointers.push_back(-1);
			}
		}
		
		dest.pad(0x10, 0);
		
		GroupHeader header = {0};
		header.group_count = src.size();
		header.data_size = dest.tell() - data_ofs;
		dest.write(header_ofs, header);
		dest.write_multiple(pointer_ofs, pointers);
	}
};

packed_struct(GlobalPvarBlockHeader,
	s32 global_pvar_size;
	s32 pointer_count;
	s32 pad[2];
)

packed_struct(GlobalPvarPointer,
	u16 pvar_index;
	u16 pointer_offset;
	s32 global_pvar_offset;
)

struct GlobalPvarBlock {
	static void read(LevelWad& wad, Gameplay& dest, Buffer src, Game game) {
		auto& header = src.read<GlobalPvarBlockHeader>(0, "global pvar block header");
		dest.global_pvar = src.read_multiple<u8>(0x10, header.global_pvar_size, "global pvar").copy();
		auto pointers = src.read_multiple<GlobalPvarPointer>(0x10 + header.global_pvar_size, header.pointer_count, "global pvar pointers");
		for(const GlobalPvarPointer& entry : pointers) {
			PvarType& type = get_pvar_type(entry.pvar_index, wad, dest);
			PvarField field;
			field.offset = entry.pointer_offset;
			field.descriptor = PVAR_GLOBAL_PVAR_POINTER;
			verify(type.insert_field(field, false), "Conflicting pvar type information.");
			
			dest.for_each_pvar_instance([&](Instance& inst) {
				if(inst.temp_pvar_index() == entry.pvar_index) {
					verify(inst.pvars().size() >= (size_t) entry.pointer_offset + 4, "Pvar too small.");
					*(s32*) &inst.pvars()[entry.pointer_offset] = entry.global_pvar_offset;
				}
			});
		}
	}
	
	static bool write(OutBuffer dest, const LevelWad& wad, const Gameplay& src, Game game) {
		if(!src.global_pvar.has_value()) {
			GlobalPvarBlockHeader header {0};
			dest.write(header);
			return true;
		}
		
		s32 header_ofs = dest.alloc<GlobalPvarBlockHeader>();
		GlobalPvarBlockHeader header = {0};
		header.global_pvar_size = src.global_pvar->size();
		
		dest.write_multiple(*src.global_pvar);
		
		std::vector<GlobalPvarPointer> pointers;
		src.for_each_pvar_instance_const(wad, [&](const Instance& inst, const PvarType& type) {
			for(auto& [offset, value] : inst.temp_global_pvar_pointers()) {
				GlobalPvarPointer entry;
				entry.pvar_index = inst.temp_pvar_index();
				entry.pointer_offset = offset;
				entry.global_pvar_offset = value;
				pointers.push_back(entry);
			}
		});
		std::stable_sort(BEGIN_END(pointers), [](const GlobalPvarPointer& l, const GlobalPvarPointer& r)
			{ return l.global_pvar_offset < r.global_pvar_offset; });
		header.pointer_count = pointers.size();
		dest.write_multiple(pointers);
		dest.write(header_ofs, header);
		return true;
	}
};

static std::vector<std::vector<glm::vec4>> read_splines(Buffer src, s32 count, s32 data_offset) {
	std::vector<std::vector<glm::vec4>> splines;
	auto relative_offsets = src.read_multiple<s32>(0, count, "spline offsets");
	for(s32 relative_offset : relative_offsets) {
		s32 spline_offset = data_offset + relative_offset;
		auto header = src.read<TableHeader>(spline_offset, "spline vertex count");
		static_assert(sizeof(glm::vec4) == 16);
		splines.emplace_back(src.read_multiple<glm::vec4>(spline_offset + 0x10, header.count_1, "spline vertices").copy());
	}
	return splines;
}

static s32 write_splines(OutBuffer dest, const std::vector<std::vector<glm::vec4>>& src) {
	s64 offsets_pos = dest.alloc_multiple<s32>(src.size());
	dest.pad(0x10, 0);
	s32 data_offset = (s32) dest.tell();
	for(std::vector<glm::vec4> spline : src) {
		dest.pad(0x10, 0);
		s32 offset = dest.tell() - data_offset;
		dest.write(offsets_pos, offset);
		offsets_pos += 4;
		
		TableHeader header {(s32) spline.size()};
		dest.write(header);
		static_assert(sizeof(glm::vec4) == 16);
		dest.write_multiple(spline);
	}
	return data_offset;
}

packed_struct(PathBlockHeader,
	s32 spline_count;
	s32 data_offset;
	s32 data_size;
	s32 pad;
)

struct PathBlock {
	static void read(std::vector<Path>& dest, Buffer src, Game game) {
		auto& header = src.read<PathBlockHeader>(0, "path block header");
		std::vector<std::vector<glm::vec4>> splines = read_splines(src.subbuf(0x10), header.spline_count, header.data_offset - 0x10);
		for(size_t i = 0; i < splines.size(); i++) {
			Path inst;
			inst.set_id_value(i);
			inst.spline() = std::move(splines[i]);
			dest.emplace_back(std::move(inst));
		}
	}
	
	static void write(OutBuffer dest, const std::vector<Path>& src, Game game) {
		std::vector<std::vector<glm::vec4>> splines;
		for(const Path& inst : src) {
			splines.emplace_back(inst.spline());
		}
		
		s64 header_pos = dest.alloc<PathBlockHeader>();
		PathBlockHeader header = {0};
		header.spline_count = src.size();
		header.data_offset = write_splines(dest, splines);
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
	static void read(GC_80_DL_64& dest, Buffer src, Game game) {
		auto header = src.read<TableHeader>(0, "block header");
		dest.first_part = src.read_multiple<u8>(0x10, 0x800, "first part of block").copy();
		dest.second_part = src.read_multiple<u8>(0x810, header.count_1 * 0x10, "second part of block").copy();
	}
	
	static void write(OutBuffer dest, const GC_80_DL_64& src, Game game) {
		TableHeader header {(s32) src.second_part.size() / 0x10};
		dest.write(header);
		dest.write_multiple(src.first_part);
		dest.write_multiple(src.second_part);
	}
};

packed_struct(GrindPathData,
	Vec4f bounding_sphere;
	s32 unknown_4;
	s32 wrap;
	s32 inactive;
	s32 pad = 0;
)

struct GrindPathBlock {
	static void read(std::vector<GrindPath>& dest, Buffer src, Game game) {
		auto& header = src.read<PathBlockHeader>(0, "spline block header");
		auto grindpaths = src.read_multiple<GrindPathData>(0x10, header.spline_count, "grindrail data");
		s64 offsets_pos = 0x10 + header.spline_count * sizeof(GrindPathData);
		auto splines = read_splines(src.subbuf(offsets_pos), header.spline_count, header.data_offset - offsets_pos);
		for(s64 i = 0; i < header.spline_count; i++) {
			GrindPath inst;
			inst.set_id_value(i);
			inst.bounding_sphere() = grindpaths[i].bounding_sphere.unpack();
			inst.unknown_4 = grindpaths[i].unknown_4;
			inst.wrap = grindpaths[i].wrap;
			inst.inactive = grindpaths[i].inactive;
			inst.spline() = splines[i];
			dest.emplace_back(std::move(inst));
		}
	}
	
	static void write(OutBuffer dest, const std::vector<GrindPath>& src, Game game) {
		s64 header_pos = dest.alloc<PathBlockHeader>();
		std::vector<std::vector<glm::vec4>> splines;
		for(const GrindPath& inst : src) {
			GrindPathData packed;
			packed.bounding_sphere = Vec4f::pack(inst.bounding_sphere());
			packed.unknown_4 = inst.unknown_4;
			packed.wrap = inst.wrap;
			packed.inactive = inst.inactive;
			dest.write(packed);
			splines.emplace_back(inst.spline());
		}
		PathBlockHeader header = {0};
		header.spline_count = src.size();
		header.data_offset = write_splines(dest, splines);
		header.data_size = dest.tell() - header.data_offset;
		header.data_offset -= header_pos;
		dest.write(header_pos, header);
	}
};

packed_struct(AreasHeader,
	s32 area_count;
	s32 part_offsets[5];
	s32 unknown_1c;
	s32 unknown_20;
)

packed_struct(GameplayAreaPacked,
	Vec4f bounding_sphere;
	s16 part_counts[5];
	s16 last_update_time;
	s32 relative_part_offsets[5];
)

struct AreasBlock {
	static void read(std::vector<Area>& dest, Buffer src, Game game) {
		src = src.subbuf(4); // Skip past size field.
		s64 header_size = sizeof(AreasHeader);
		auto header = src.read<AreasHeader>(0, "area list block header");
		auto entries = src.read_multiple<GameplayAreaPacked>(header_size, header.area_count, "area list table");
		s32 index = 0;
		for(const GameplayAreaPacked& entry : entries) {
			Area area;
			area.id = index++;
			area.bounding_sphere = entry.bounding_sphere.unpack();
			area.last_update_time = entry.last_update_time;
			for(s32 part = 0; part < 5; part++) {
				s32 part_ofs = header.part_offsets[part] + entry.relative_part_offsets[part];
				area.parts[part] = src.read_multiple<s32>(part_ofs, entry.part_counts[part], "area list data").copy();
			}
			dest.emplace_back(std::move(area));
		}
	}
	
	static void write(OutBuffer dest, const std::vector<Area>& src, Game game) {
		s64 size_ofs = dest.alloc<s32>();
		s64 header_ofs = dest.alloc<AreasHeader>();
		s64 table_ofs = dest.alloc_multiple<GameplayAreaPacked>(src.size());
		
		s64 total_part_counts[5] = {0, 0, 0, 0, 0};
		
		AreasHeader header;
		std::vector<GameplayAreaPacked> table;
		for(const Area& area : src) {
			GameplayAreaPacked packed;
			packed.bounding_sphere = Vec4f::pack(area.bounding_sphere);
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
	static void read(std::vector<TieAmbientRgbas>& dest, Buffer src, Game game) {
		s64 ofs = 0;
		for(;;) {
			TieAmbientRgbas part;
			part.id = src.read<s16>(ofs, "id");
			ofs += 2;
			if(part.id == -1) {
				break;
			}
			s64 size = ((s64) src.read<s16>(ofs, "size")) * 2;
			ofs += 2;
			part.data = src.read_multiple<u8>(ofs, size, "tie rgba data").copy();
			dest.emplace_back(std::move(part));
			ofs += size;
		}
	}
	
	static void write(OutBuffer dest, const std::vector<TieAmbientRgbas>& src, Game game) {
		for(const TieAmbientRgbas& part : src) {
			dest.write(part.id);
			assert(part.data.size() % 2 == 0);
			dest.write<s16>(part.data.size() / 2);
			dest.write_multiple(part.data);
		}
		dest.write<s16>(-1);
	}
};

struct TieClassBlock {
	static void read(LevelWad& wad, Gameplay& dest, Buffer src, Game game) {}
	
	static bool write(OutBuffer dest, const LevelWad& wad, const Gameplay& src, Game game) {
		std::vector<s32> classes;
		for(const TieInstance& inst : opt_iterator(src.tie_instances)) {
			if(std::find(BEGIN_END(classes), inst.o_class) == classes.end()) {
				classes.push_back(inst.o_class);
			}
		}
		dest.write<s32>(classes.size());
		dest.write_multiple(classes);
		return true;
	}
};

struct ShrubClassBlock {
	static void read(LevelWad& wad, Gameplay& dest, Buffer src, Game game) {}
	
	static bool write(OutBuffer dest, const LevelWad& wad, const Gameplay& src, Game game) {
		std::vector<s32> classes;
		for(const ShrubInstance& inst : opt_iterator(src.shrub_instances)) {
			if(std::find(BEGIN_END(classes), inst.o_class) == classes.end()) {
				classes.push_back(inst.o_class);
			}
		}
		dest.write<s32>(classes.size());
		dest.write_multiple(classes);
		return true;
	}
};

packed_struct(OcclusionHeader,
	s32 count_1;
	s32 count_2;
	s32 count_3;
	s32 pad = 0;
)

struct OcclusionBlock {
	static void read(Occlusion& dest, Buffer src, Game game) {
		auto& header = src.read<OcclusionHeader>(0, "occlusion header");
		s64 ofs = 0x10;
		dest.first_part = src.read_multiple<u8>(ofs, header.count_1 * 8, "first part of occlusion").copy();
		ofs += header.count_1 * 8;
		dest.second_part = src.read_multiple<u8>(ofs, header.count_2 * 8, "second part of occlusion").copy();
		ofs += header.count_2 * 8;
		dest.third_part = src.read_multiple<u8>(ofs, header.count_3 * 8, "third part of occlusion").copy();
	}
	
	static void write(OutBuffer dest, const Occlusion& src, Game game) {
		OcclusionHeader header;
		header.count_1 = (s32) src.first_part.size() / 8;
		header.count_2 = (s32) src.second_part.size() / 8;
		header.count_3 = (s32) src.third_part.size() / 8;
		dest.write(header);
		dest.write_multiple(src.first_part);
		dest.write_multiple(src.second_part);
		dest.write_multiple(src.third_part);
		dest.pad(0x40, 0);
	}
};

std::vector<u8> write_occlusion(const Gameplay& gameplay, Game game) {
	std::vector<u8> dest;
	if(gameplay.occlusion.has_value()) {
		OcclusionBlock::write(dest, *gameplay.occlusion, game);
	}
	return dest;
}

packed_struct(CameraPacked,
	/* 0x00 */ s32 type;
	/* 0x04 */ Vec3f position;
	/* 0x10 */ Vec3f rotation;
	/* 0x1c */ s32 pvar_index;
)

static void swap_instance(Camera& l, CameraPacked& r) {
	swap_position_rotation(l, r);
	SWAP_PACKED(l.temp_pvar_index(), r.pvar_index);
	SWAP_PACKED(l.type, r.type);
}

packed_struct(SoundInstancePacked,
	s16 o_class;
	s16 m_class;
	u32 update_fun_ptr;
	s32 pvar_index;
	f32 range;
	Mat4 matrix;
	Mat3 inverse_matrix;
	Vec3f rotation;
	f32 pad;
)

static void swap_instance(SoundInstance& l, SoundInstancePacked& r) {
	swap_matrix_inverse_rotation(l, r);
	SWAP_PACKED(l.temp_pvar_index(), r.pvar_index);
	SWAP_PACKED(l.o_class, r.o_class);
	SWAP_PACKED(l.m_class, r.m_class);
	r.update_fun_ptr = 0;
	SWAP_PACKED(l.range, r.range);
	r.pad = 0.f;
}

packed_struct(ShapePacked,
	Mat4 matrix;
	Mat3 inverse_matrix;
	Vec3f rotation;
	f32 pad;
)

static void swap_instance(Cuboid& l, ShapePacked& r) {
	swap_matrix_inverse_rotation(l, r);
	r.pad = 0.f;
}

static void swap_instance(Sphere& l, ShapePacked& r) {
	swap_matrix_inverse_rotation(l, r);
	r.pad = 0.f;
}

static void swap_instance(Cylinder& l, ShapePacked& r) {
	swap_matrix_inverse_rotation(l, r);
	r.pad = 0.f;
}

packed_struct(DirectionalLightPacked,
	Vec4f colour_a;
	Vec4f direction_a;
	Vec4f colour_b;
	Vec4f direction_b;
)

static void swap_instance(DirectionalLight& l, DirectionalLightPacked& r) {
	r.colour_a.swap(l.colour_a);
	r.direction_a.swap(l.direction_a);
	r.colour_b.swap(l.colour_b);
	r.direction_b.swap(l.direction_b);
}

packed_struct(TieInstancePacked,
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
static_assert(sizeof(TieInstancePacked) == 0x60);

static void swap_instance(TieInstance& l, TieInstancePacked& r) {
	swap_matrix(l, r);
	SWAP_PACKED(l.draw_distance(), r.draw_distance);
	SWAP_PACKED(l.o_class, r.o_class);
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
	/* 0x08 */ s32 unknown_8;
	/* 0x0c */ s32 unknown_c;
	/* 0x10 */ Mat4 matrix;
	/* 0x50 */ Rgb96 light_colour;
	/* 0x5c */ s32 unknown_5c;
	/* 0x60 */ s32 unknown_60;
	/* 0x64 */ s32 unknown_64;
	/* 0x68 */ s32 unknown_68;
	/* 0x6c */ s32 unknown_6c;
)

static void swap_instance(ShrubInstance& l, ShrubInstancePacked& r) {
	swap_matrix(l, r);
	SWAP_PACKED(l.draw_distance(), r.draw_distance);
	SWAP_PACKED(l.colour().r, r.light_colour.r);
	SWAP_PACKED(l.colour().g, r.light_colour.g);
	SWAP_PACKED(l.colour().b, r.light_colour.b);
	SWAP_PACKED(l.o_class, r.o_class);
	SWAP_PACKED(l.unknown_8, r.unknown_8);
	SWAP_PACKED(l.unknown_c, r.unknown_c);
	SWAP_PACKED(l.unknown_5c, r.unknown_5c);
	SWAP_PACKED(l.unknown_60, r.unknown_60);
	SWAP_PACKED(l.unknown_64, r.unknown_64);
	SWAP_PACKED(l.unknown_68, r.unknown_68);
	SWAP_PACKED(l.unknown_6c, r.unknown_6c);
}

static void rewire_pvar_indices(Gameplay& gameplay) {
	s32 pvar_index = 0;
	gameplay.for_each_pvar_instance([&](Instance& inst) {
		if(inst.pvars().size() > 0) {
			inst.temp_pvar_index() = pvar_index++;
		} else {
			inst.temp_pvar_index() = -1;
		}
	});
}

static void rewire_global_pvar_pointers(const LevelWad& wad, Gameplay& gameplay) {
	gameplay.for_each_pvar_instance(wad, [&](Instance& inst, const PvarType& type) {
		for(const PvarField& field : type.fields) {
			if(field.descriptor == PVAR_GLOBAL_PVAR_POINTER) {
				verify(inst.pvars().size() >= (size_t) field.offset + 4, "Pvars too small.");
				inst.temp_global_pvar_pointers().emplace_back(field.offset, *(s32*) &inst.pvars()[field.offset]);
				*(s32*) &inst.pvars()[field.offset] = 0;
			}
		}
	});
}

template <typename Block, typename Field>
static GameplayBlockFuncs bf(Field field) {
	// e.g. if field = &Gameplay::moby_instances then FieldType = std::vector<MobyInstance>.
	using FieldType = typename std::remove_reference<decltype(Gameplay().*field)>::type::value_type;
	
	GameplayBlockFuncs funcs;
	funcs.read = [field](LevelWad&, Gameplay& gameplay, Buffer src, Game game) {
		FieldType value;
		Block::read(value, src, game);
		gameplay.*field = std::move(value);
	};
	funcs.write = [field](OutBuffer dest, const LevelWad&, const Gameplay& gameplay, Game game) {
		if(!(gameplay.*field).has_value()) {
			return false;
		}
		Block::write(dest, *(gameplay.*field), game);
		return true;
	};
	return funcs;
}

const std::vector<GameplayBlockDescription> RAC23_GAMEPLAY_BLOCKS = {
	{0x8c, bf<TableBlock<GC_8c_DL_70>>(&Gameplay::gc_8c_dl_70), "GC 8c DL 70"},
	{0x00, bf<PropertiesBlock>(&Gameplay::properties), "properties"},
	{0x10, bf<HelpMessageBlock<false>>(&Gameplay::us_english_help_messages), "us english help messages"},
	{0x14, bf<HelpMessageBlock<false>>(&Gameplay::uk_english_help_messages), "uk english help messages"},
	{0x18, bf<HelpMessageBlock<false>>(&Gameplay::french_help_messages), "french help messages"},
	{0x1c, bf<HelpMessageBlock<false>>(&Gameplay::german_help_messages), "german help messages"},
	{0x20, bf<HelpMessageBlock<false>>(&Gameplay::spanish_help_messages), "spanish help messages"},
	{0x24, bf<HelpMessageBlock<false>>(&Gameplay::italian_help_messages), "italian help messages"},
	{0x28, bf<HelpMessageBlock<false>>(&Gameplay::japanese_help_messages), "japanese help messages"},
	{0x2c, bf<HelpMessageBlock<true>>(&Gameplay::korean_help_messages), "korean help messages"},
	{0x04, bf<InstanceBlock<DirectionalLight, DirectionalLightPacked>>(&Gameplay::lights), "directional lights"},
	{0x84, bf<LightTriggerBlock>(&Gameplay::light_triggers), "light triggers"},
	{0x08, bf<InstanceBlock<Camera, CameraPacked>>(&Gameplay::cameras), "cameras"},
	{0x0c, bf<InstanceBlock<SoundInstance, SoundInstancePacked>>(&Gameplay::sound_instances), "sound instances"},
	{0x48, bf<ClassBlock>(&Gameplay::moby_classes), "moby classes"},
	{0x4c, {RAC23MobyBlock::read, RAC23MobyBlock::write}, "moby instances"},
	{0x5c, {PvarTableBlock::read, PvarTableBlock::write}, "pvar table"},
	{0x60, {PvarDataBlock::read, PvarDataBlock::write}, "pvar data"},
	{0x58, {PvarScratchpadBlock::read, PvarScratchpadBlock::write}, "pvar pointer scratchpad table"},
	{0x64, {PvarPointerRewireBlock::read, PvarPointerRewireBlock::write}, "pvar pointer rewire table"},
	{0x50, bf<GroupBlock>(&Gameplay::moby_groups), "moby groups"},
	{0x54, {GlobalPvarBlock::read, GlobalPvarBlock::write}, "global pvar"},
	{0x30, {TieClassBlock::read, TieClassBlock::write}, "tie classes"},
	{0x34, bf<InstanceBlock<TieInstance, TieInstancePacked>>(&Gameplay::tie_instances), "tie instances"},
	{0x94, bf<TieAmbientRgbaBlock>(&Gameplay::tie_ambient_rgbas), "tie ambient rgbas"},
	{0x38, bf<GroupBlock>(&Gameplay::tie_groups), "tie groups"},
	{0x3c, {ShrubClassBlock::read, ShrubClassBlock::write}, "shrub classes"},
	{0x40, bf<InstanceBlock<ShrubInstance, ShrubInstancePacked>>(&Gameplay::shrub_instances), "shrub instances"},
	{0x44, bf<GroupBlock>(&Gameplay::shrub_groups), "shrub groups"},
	{0x78, bf<PathBlock>(&Gameplay::paths), "paths"},
	{0x68, bf<InstanceBlock<Cuboid, ShapePacked>>(&Gameplay::cuboids), "cuboids"},
	{0x6c, bf<InstanceBlock<Sphere, ShapePacked>>(&Gameplay::spheres), "spheres"},
	{0x70, bf<InstanceBlock<Cylinder, ShapePacked>>(&Gameplay::cylinders), "cylinders"},
	{0x74, bf<TableBlock<s32>>(&Gameplay::gc_74_dl_58), "GC 74 DL 58"},
	{0x88, bf<GC_88_DL_6c_Block>(&Gameplay::gc_88_dl_6c), "GC 88 DL 6c"},
	{0x80, bf<GC_80_DL_64_Block>(&Gameplay::gc_80_dl_64), "GC 80 DL 64"},
	{0x7c, bf<GrindPathBlock>(&Gameplay::grind_paths), "grindpaths"},
	{0x98, bf<AreasBlock>(&Gameplay::areas), "areas"},
	{0x90, bf<OcclusionBlock>(&Gameplay::occlusion), "occlusion"}
};

const std::vector<GameplayBlockDescription> DL_GAMEPLAY_CORE_BLOCKS = {
	{0x70, bf<TableBlock<GC_8c_DL_70>>(&Gameplay::gc_8c_dl_70), "GC 8c DL 70"},
	{0x00, bf<PropertiesBlock>(&Gameplay::properties), "properties"},
	{0x0c, bf<HelpMessageBlock<false>>(&Gameplay::us_english_help_messages), "us english help messages"},
	{0x10, bf<HelpMessageBlock<false>>(&Gameplay::uk_english_help_messages), "uk english help messages"},
	{0x14, bf<HelpMessageBlock<false>>(&Gameplay::french_help_messages), "french help messages"},
	{0x18, bf<HelpMessageBlock<false>>(&Gameplay::german_help_messages), "german help messages"},
	{0x1c, bf<HelpMessageBlock<false>>(&Gameplay::spanish_help_messages), "spanish help messages"},
	{0x20, bf<HelpMessageBlock<false>>(&Gameplay::italian_help_messages), "italian help messages"},
	{0x24, bf<HelpMessageBlock<false>>(&Gameplay::japanese_help_messages), "japanese help messages"},
	{0x28, bf<HelpMessageBlock<true>>(&Gameplay::korean_help_messages), "korean help messages"},
	{0x04, bf<InstanceBlock<Camera, CameraPacked>>(&Gameplay::cameras), "import cameras"},
	{0x08, bf<InstanceBlock<SoundInstance, SoundInstancePacked>>(&Gameplay::sound_instances), "sound instances"},
	{0x2c, bf<ClassBlock>(&Gameplay::moby_classes), "moby classes"},
	{0x30, {DeadlockedMobyBlock::read, DeadlockedMobyBlock::write}, "moby instances"},
	{0x40, {PvarTableBlock::read, PvarTableBlock::write}, "pvar table"},
	{0x44, {PvarDataBlock::read, PvarDataBlock::write}, "pvar data"},
	{0x3c, {PvarScratchpadBlock::read, PvarScratchpadBlock::write}, "pvar pointer scratchpad table"},
	{0x48, {PvarPointerRewireBlock::read, PvarPointerRewireBlock::write}, "pvar pointer rewire table"},
	{0x34, bf<GroupBlock>(&Gameplay::moby_groups), "moby groups"},
	{0x38, {GlobalPvarBlock::read, GlobalPvarBlock::write}, "global pvar"},
	{0x5c, bf<PathBlock>(&Gameplay::paths), "paths"},
	{0x4c, bf<InstanceBlock<Cuboid, ShapePacked>>(&Gameplay::cuboids), "cuboids"},
	{0x50, bf<InstanceBlock<Sphere, ShapePacked>>(&Gameplay::spheres), "spheres"},
	{0x54, bf<InstanceBlock<Cylinder, ShapePacked>>(&Gameplay::cylinders), "cylinders"},
	{0x58, bf<TableBlock<s32>>(&Gameplay::gc_74_dl_58), "GC 74 DL 58"},
	{0x6c, bf<GC_88_DL_6c_Block>(&Gameplay::gc_88_dl_6c), "GC 88 DL 6c"},
	{0x64, bf<GC_80_DL_64_Block>(&Gameplay::gc_80_dl_64), "GC 80 DL 64"},
	{0x60, bf<GrindPathBlock>(&Gameplay::grind_paths), "grindpaths"},
	{0x74, bf<AreasBlock>(&Gameplay::areas), "areas"},
	{0x68, {nullptr, nullptr}, "pad"}
};

const std::vector<GameplayBlockDescription> DL_ART_INSTANCE_BLOCKS = {
	{0x00, bf<InstanceBlock<DirectionalLight, DirectionalLightPacked>>(&Gameplay::lights), "directional lights"},
	{0x04, {TieClassBlock::read, TieClassBlock::write}, "tie classes"},
	{0x08, bf<InstanceBlock<TieInstance, TieInstancePacked>>(&Gameplay::tie_instances), "tie instances"},
	{0x20, bf<TieAmbientRgbaBlock>(&Gameplay::tie_ambient_rgbas), "tie ambient rgbas"},
	{0x0c, bf<GroupBlock>(&Gameplay::tie_groups), "tie groups"},
	{0x10, {ShrubClassBlock::read, ShrubClassBlock::write}, "shrub classes"},
	{0x14, bf<InstanceBlock<ShrubInstance, ShrubInstancePacked>>(&Gameplay::shrub_instances), "shrub instances"},
	{0x18, bf<GroupBlock>(&Gameplay::shrub_groups), "art instance shrub groups"},
	{0x1c, bf<OcclusionBlock>(&Gameplay::occlusion), "occlusion"},
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
	{0x10, {PvarScratchpadBlock::read, PvarScratchpadBlock::write}, "pvar pointer scratchpad table"},
	{0x1c, {PvarPointerRewireBlock::read, PvarPointerRewireBlock::write}, "pvar pointer rewire table"},
	{0x08, bf<GroupBlock>(&Gameplay::moby_groups), "moby groups"},
	{0x0c, {GlobalPvarBlock::read, GlobalPvarBlock::write}, "global pvar"},
};
