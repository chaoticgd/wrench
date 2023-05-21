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

#ifndef INSTANCEMGR_GAMEPLAY_IMPL_MISC_H
#define INSTANCEMGR_GAMEPLAY_IMPL_MISC_H

#include <instancemgr/gameplay_impl_common.inl>

packed_struct(RacLevelSettingsFirstPart,
	/* 0x00 */ Rgb96 background_colour;
	/* 0x0c */ Rgb96 fog_colour;
	/* 0x18 */ f32 fog_near_distance;
	/* 0x1c */ f32 fog_far_distance;
	/* 0x20 */ f32 fog_near_intensity;
	/* 0x24 */ f32 fog_far_intensity;
	/* 0x28 */ f32 death_height;
	/* 0x2c */ Vec3f ship_position;
	/* 0x38 */ f32 ship_rotation_z;
	/* 0x3c */ Rgb96 unknown_colour;
	/* 0x48 */ u32 pad[2];
)
static_assert(sizeof(RacLevelSettingsFirstPart) == 0x50);

packed_struct(GcUyaDlLevelSettingsFirstPart,
	/* 0x00 */ Rgb96 background_colour;
	/* 0x0c */ Rgb96 fog_colour;
	/* 0x18 */ f32 fog_near_distance;
	/* 0x1c */ f32 fog_far_distance;
	/* 0x20 */ f32 fog_near_intensity;
	/* 0x24 */ f32 fog_far_intensity;
	/* 0x28 */ f32 death_height;
	/* 0x2c */ s32 is_spherical_world;
	/* 0x30 */ Vec3f sphere_centre;
	/* 0x3c */ Vec3f ship_position;
	/* 0x48 */ f32 ship_rotation_z;
	/* 0x4c */ Rgb96 unknown_colour;
	/* 0x58 */ u32 pad;
)
static_assert(sizeof(GcUyaDlLevelSettingsFirstPart) == 0x5c);

packed_struct(ChunkPlanePacked,
	/* 0x00 */ f32 point_x;
	/* 0x04 */ f32 point_y;
	/* 0x08 */ f32 point_z;
	/* 0x0c */ s32 plane_count;
	/* 0x10 */ f32 normal_x;
	/* 0x14 */ f32 normal_y;
	/* 0x18 */ f32 normal_z;
	/* 0x1c */ u32 pad;
)
static_assert(sizeof(ChunkPlanePacked) == 0x20);

struct LevelSettingsBlock {
	static void read(LevelSettings& dest, Buffer src, Game game) {
		s32 ofs = 0;
		if(game == Game::RAC) {
			RacLevelSettingsFirstPart first_part = src.read<RacLevelSettingsFirstPart>(ofs, "level settings");
			swap_rac_first_part(dest, first_part);
		} else {
			GcUyaDlLevelSettingsFirstPart first_part = src.read<GcUyaDlLevelSettingsFirstPart>(ofs, "level settings");
			swap_gc_uya_dl_first_part(dest, first_part);
			ofs += sizeof(GcUyaDlLevelSettingsFirstPart);
			s32 chunk_plane_count = src.read<s32>(ofs + 0xc, "second part count");
			if(chunk_plane_count > 0) {
				auto chunk_planes = src.read_multiple<ChunkPlanePacked>(ofs, chunk_plane_count, "second part").copy();
				for(ChunkPlanePacked& packed : chunk_planes) {
					ChunkPlane& plane = dest.chunk_planes.emplace_back();
					plane.point.x = packed.point_x;
					plane.point.y = packed.point_y;
					plane.point.z = packed.point_z;
					plane.normal.x = packed.normal_x;
					plane.normal.y = packed.normal_y;
					plane.normal.z = packed.normal_z;
				}
				ofs += chunk_plane_count * sizeof(ChunkPlanePacked);
			} else {
				ofs += sizeof(ChunkPlanePacked);
			}
			dest.core_sounds_count = src.read<s32>(ofs, "core sounds count");
			ofs += 4;
			if(game == Game::UYA) {
				dest.rac3_third_part = src.read<s32>(ofs, "R&C3 third part");
			} else if(game == Game::DL) {
				s64 third_part_count = src.read<s32>(ofs, "third part count");
				ofs += 4;
				if(third_part_count >= 0) {
					dest.third_part = src.read_multiple<LevelSettingsThirdPart>(ofs, third_part_count, "third part").copy();
					ofs += third_part_count * sizeof(LevelSettingsThirdPart);
					dest.fourth_part = src.read<LevelSettingsFourthPart>(ofs, "fourth part");
					ofs += sizeof(LevelSettingsFourthPart);
				} else {
					ofs += sizeof(LevelSettingsThirdPart);
				}
				dest.fifth_part = src.read<LevelSettingsFifthPart>(ofs, "fifth part");
				ofs += sizeof(LevelSettingsFifthPart);
				dest.sixth_part = src.read_multiple<s8>(ofs, dest.fifth_part->sixth_part_count, "sixth part").copy();
			}
		}
	}
	
	static void write(OutBuffer dest, const LevelSettings& src, Game game) {
		LevelSettings copy = src;
		if(game == Game::RAC) {
			RacLevelSettingsFirstPart first_part_packed;
			swap_rac_first_part(copy, first_part_packed);
			dest.write(first_part_packed);
		} else {
			GcUyaDlLevelSettingsFirstPart first_part_packed;
			swap_gc_uya_dl_first_part(copy, first_part_packed);
			dest.write(first_part_packed);
			if(!src.chunk_planes.empty()) {
				for(const ChunkPlane& plane : src.chunk_planes) {
					ChunkPlanePacked packed = {};
					packed.point_x = plane.point.x;
					packed.point_y = plane.point.y;
					packed.point_z = plane.point.z;
					packed.plane_count = (s32) src.chunk_planes.size();
					packed.normal_x = plane.normal.x;
					packed.normal_y = plane.normal.y;
					packed.normal_z = plane.normal.z;
					dest.write(packed);
				}
			} else {
				ChunkPlane terminator = {};
				dest.write(terminator);
			}
			verify(src.core_sounds_count.has_value(), "Missing core_sounds_count in level settings block.");
			dest.write(*src.core_sounds_count);
			if(game == Game::UYA) {
				verify(src.rac3_third_part.has_value(), "Missing rac3_third_part in level settings block.");
				dest.write(*src.rac3_third_part);
			} else if(game == Game::DL) {
				verify(src.third_part.has_value(), "Missing third_part in level settings block.");
				dest.write((s32) src.third_part->size());
				if(src.third_part->size() > 0) {
					dest.write_multiple(*src.third_part);
					verify(src.fourth_part.has_value(), "Missing fourth_part in level settings block.");
					dest.write(*src.fourth_part);
				} else {
					dest.vec.resize(dest.tell() + 0x18, 0);
				}
				verify(src.fifth_part.has_value(), "Missing fifth in level settings block.");
				dest.write(*src.fifth_part);
				verify(src.sixth_part.has_value(), "Missing sixth_part in level settings block.");
				dest.write_multiple(*src.sixth_part);
			}
		}
	}
	
	static void swap_rac_first_part(LevelSettings& l, RacLevelSettingsFirstPart& r) {
		SWAP_COLOUR_OPT(l.background_colour, r.background_colour);
		SWAP_COLOUR_OPT(l.fog_colour, r.fog_colour);
		SWAP_PACKED(l.fog_near_dist, r.fog_near_distance);
		SWAP_PACKED(l.fog_far_dist, r.fog_far_distance);
		SWAP_PACKED(l.fog_near_intensity, r.fog_near_intensity);
		SWAP_PACKED(l.fog_far_intensity, r.fog_far_intensity);
		SWAP_PACKED(l.death_height, r.death_height);
		SWAP_PACKED(l.ship_pos.x, r.ship_position.x);
		SWAP_PACKED(l.ship_pos.y, r.ship_position.y);
		SWAP_PACKED(l.ship_pos.z, r.ship_position.z);
		SWAP_PACKED(l.ship_rot_z, r.ship_rotation_z);
		SWAP_COLOUR_OPT(l.unknown_colour, r.unknown_colour);
		r.pad[0] = 0;
		r.pad[1] = 0;
	}
	
	static void swap_gc_uya_dl_first_part(LevelSettings& l, GcUyaDlLevelSettingsFirstPart& r) {
		SWAP_COLOUR_OPT(l.background_colour, r.background_colour);
		SWAP_COLOUR_OPT(l.fog_colour, r.fog_colour);
		SWAP_PACKED(l.fog_near_dist, r.fog_near_distance);
		SWAP_PACKED(l.fog_far_dist, r.fog_far_distance);
		SWAP_PACKED(l.fog_near_intensity, r.fog_near_intensity);
		SWAP_PACKED(l.fog_far_intensity, r.fog_far_intensity);
		SWAP_PACKED(l.death_height, r.death_height);
		SWAP_PACKED(l.is_spherical_world, r.is_spherical_world);
		SWAP_PACKED(l.sphere_pos.x, r.sphere_centre.x);
		SWAP_PACKED(l.sphere_pos.y, r.sphere_centre.y);
		SWAP_PACKED(l.sphere_pos.z, r.sphere_centre.z);
		SWAP_PACKED(l.ship_pos.x, r.ship_position.x);
		SWAP_PACKED(l.ship_pos.y, r.ship_position.y);
		SWAP_PACKED(l.ship_pos.z, r.ship_position.z);
		SWAP_PACKED(l.ship_rot_z, r.ship_rotation_z);
		SWAP_COLOUR_OPT(l.unknown_colour, r.unknown_colour);
		r.pad = 0;
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
		
		if(game == Game::UYA || game == Game::DL) {
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
		if(game == Game::UYA || game == Game::DL) {
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
				if(game == Game::RAC || game == Game::GC) {
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
	static void read(std::vector<PathInstance>& dest, Buffer src, Game game) {
		auto& header = src.read<PathBlockHeader>(0, "path block header");
		std::vector<std::vector<glm::vec4>> splines = read_splines(src.subbuf(0x10), header.spline_count, header.data_offset - 0x10);
		for(size_t i = 0; i < splines.size(); i++) {
			PathInstance inst;
			inst.set_id_value(i);
			inst.spline() = std::move(splines[i]);
			dest.emplace_back(std::move(inst));
		}
	}
	
	static void write(OutBuffer dest, const std::vector<PathInstance>& src, Game game) {
		std::vector<std::vector<glm::vec4>> splines;
		for(const PathInstance& inst : src) {
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

packed_struct(GrindPathData,
	Vec4f bounding_sphere;
	s32 unknown_4;
	s32 wrap;
	s32 inactive;
	s32 pad = 0;
)

struct GrindPathBlock {
	static void read(std::vector<GrindPathInstance>& dest, Buffer src, Game game) {
		auto& header = src.read<PathBlockHeader>(0, "spline block header");
		auto grindpaths = src.read_multiple<GrindPathData>(0x10, header.spline_count, "grindrail data");
		s64 offsets_pos = 0x10 + header.spline_count * sizeof(GrindPathData);
		auto splines = read_splines(src.subbuf(offsets_pos), header.spline_count, header.data_offset - offsets_pos);
		for(s64 i = 0; i < header.spline_count; i++) {
			GrindPathInstance inst;
			inst.set_id_value(i);
			inst.bounding_sphere() = grindpaths[i].bounding_sphere.unpack();
			inst.unknown_4 = grindpaths[i].unknown_4;
			inst.wrap = grindpaths[i].wrap;
			inst.inactive = grindpaths[i].inactive;
			inst.spline() = splines[i];
			dest.emplace_back(std::move(inst));
		}
	}
	
	static void write(OutBuffer dest, const std::vector<GrindPathInstance>& src, Game game) {
		s64 header_ofs = dest.alloc<PathBlockHeader>();
		std::vector<std::vector<glm::vec4>> splines;
		for(const GrindPathInstance& inst : src) {
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
		s32 abs_data_offset = write_splines(dest, splines);
		header.data_offset = abs_data_offset - header_ofs;
		header.data_size = dest.tell() - abs_data_offset;
		dest.write(header_ofs, header);
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
		auto header = src.read<AreasHeader>(0, "area list block header");
		auto entries = src.read_multiple<GameplayAreaPacked>(sizeof(AreasHeader), header.area_count, "area list table");
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

packed_struct(OcclusionMappingsHeader,
	s32 tfrag_mapping_count;
	s32 tie_mapping_count;
	s32 moby_mapping_count;
	s32 pad = 0;
)

struct OcclusionMappingsBlock {
	static void read(OcclusionMappings& dest, Buffer src, Game game) {
		auto& header = src.read<OcclusionMappingsHeader>(0, "occlusion header");
		s64 ofs = 0x10;
		dest.tfrag_mappings = src.read_multiple<OcclusionMapping>(ofs, header.tfrag_mapping_count, "tfrag occlusion mappings").copy();
		ofs += header.tfrag_mapping_count * 8;
		dest.tie_mappings = src.read_multiple<OcclusionMapping>(ofs, header.tie_mapping_count, "tie occlusion mappings").copy();
		ofs += header.tie_mapping_count * 8;
		dest.moby_mappings = src.read_multiple<OcclusionMapping>(ofs, header.moby_mapping_count, "moby occlusion mappings").copy();
	}
	
	static void write(OutBuffer dest, const OcclusionMappings& src, Game game) {
		OcclusionMappingsHeader header;
		header.tfrag_mapping_count = (s32) src.tfrag_mappings.size();
		header.tie_mapping_count = (s32) src.tie_mappings.size();
		header.moby_mapping_count = (s32) src.moby_mappings.size();
		dest.write(header);
		dest.write_multiple(src.tfrag_mappings);
		dest.write_multiple(src.tie_mappings);
		dest.write_multiple(src.moby_mappings);
	}
};

std::vector<u8> write_occlusion_mappings(const Gameplay& gameplay, Game game) {
	std::vector<u8> dest;
	if(gameplay.occlusion.has_value()) {
		OcclusionMappingsBlock::write(dest, *gameplay.occlusion, game);
	}
	return dest;
}

packed_struct(ShapePacked,
	/* 0x00 */ Mat4 matrix;
	/* 0x40 */ Mat3 inverse_matrix;
	/* 0x70 */ Vec3f rotation;
	/* 0x7c */ f32 unused_7c;
)

static void swap_instance(CuboidInstance& l, ShapePacked& r) {
	swap_matrix_inverse_rotation(l, r);
	r.unused_7c = 0.f;
}

static void swap_instance(SphereInstance& l, ShapePacked& r) {
	swap_matrix_inverse_rotation(l, r);
	r.unused_7c = 0.f;
}

static void swap_instance(CylinderInstance& l, ShapePacked& r) {
	swap_matrix_inverse_rotation(l, r);
	r.unused_7c = 0.f;
}

static void swap_instance(PillInstance& l, ShapePacked& r) {
	swap_matrix_inverse_rotation(l, r);
	r.unused_7c = 0.f;
}

#endif
