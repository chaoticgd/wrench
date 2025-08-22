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
	/* 0x3c */ s32 ship_path;
	/* 0x40 */ s32 ship_camera_cuboid_start;
	/* 0x44 */ s32 ship_camera_cuboid_end;
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
	/* 0x4c */ s32 ship_path;
	/* 0x50 */ s32 ship_camera_cuboid_start;
	/* 0x54 */ s32 ship_camera_cuboid_end;
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

struct LevelSettingsBlock
{
	static void read(LevelSettings& dest, Buffer src, Game game)
	{
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
					dest.reward_stats = src.read<LevelSettingsRewardStats>(ofs, "reward stats");
					ofs += sizeof(LevelSettingsRewardStats);
				} else {
					ofs += sizeof(LevelSettingsThirdPart);
				}
				dest.fifth_part = src.read<LevelSettingsFifthPart>(ofs, "fifth part");
				ofs += sizeof(LevelSettingsFifthPart);
				s32 dbg_attack_damange_count = src.read<s32>(ofs);
				ofs += 4;
				dest.dbg_attack_damage = src.read_multiple<u8>(ofs, dbg_attack_damange_count, "dbg attack damage array").copy();
			}
		}
	}
	
	static void write(OutBuffer dest, const LevelSettings& src, Game game)
	{
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
				ChunkPlanePacked terminator = {};
				dest.write(terminator);
			}
			verify(src.core_sounds_count.has_value(), "Missing core_sounds_count in level settings block.");
			dest.write(*src.core_sounds_count);
			if(game == Game::UYA) {
				verify(src.rac3_third_part.has_value(), "Missing rac3_third_part in level settings block.");
				dest.write(*src.rac3_third_part);
			} else if(game == Game::DL) {
				dest.write((s32) opt_size(src.third_part));
				if(opt_size(src.third_part) > 0) {
					dest.write_multiple(*src.third_part);
					verify(src.reward_stats.has_value(), "Missing fourth_part in level settings block.");
					dest.write(*src.reward_stats);
				} else {
					dest.vec.resize(dest.tell() + 0x18, 0);
				}
				verify(src.fifth_part.has_value(), "Missing fifth in level settings block.");
				dest.write(*src.fifth_part);
				verify(src.dbg_attack_damage.has_value(), "Missing dbg attack damage array in level settings block.");
				dest.write<s32>(src.dbg_attack_damage->size());
				dest.write_multiple(*src.dbg_attack_damage);
			}
		}
	}
	
	static void swap_rac_first_part(LevelSettings& l, RacLevelSettingsFirstPart& r)
	{
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
		SWAP_PACKED(l.ship_path.id, r.ship_path);
		SWAP_PACKED(l.ship_camera_cuboid_start.id, r.ship_camera_cuboid_start);
		SWAP_PACKED(l.ship_camera_cuboid_end.id, r.ship_camera_cuboid_end);
		r.pad[0] = 0;
		r.pad[1] = 0;
	}
	
	static void swap_gc_uya_dl_first_part(LevelSettings& l, GcUyaDlLevelSettingsFirstPart& r)
	{
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
		SWAP_PACKED(l.ship_path.id, r.ship_path);
		SWAP_PACKED(l.ship_camera_cuboid_start.id, r.ship_camera_cuboid_start);
		SWAP_PACKED(l.ship_camera_cuboid_end.id, r.ship_camera_cuboid_end);
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
struct HelpMessageBlock
{
	static void read(std::vector<HelpMessage>& dest, Buffer src, Game game)
	{
		auto& header = src.read<HelpMessageHeader>(0, "string block header");
		auto table = src.read_multiple<HelpMessageEntry>(8, header.count, "string table");
		
		if(game == Game::UYA || game == Game::DL) {
			src = src.subbuf(8);
		}
		
		for(HelpMessageEntry entry : table) {
			HelpMessage& message = dest.emplace_back();
			if(entry.offset != 0) {
				message.string = src.read_string(entry.offset, is_korean);
			}
			message.id = entry.id;
			message.short_id = entry.short_id;
			message.third_person_id = entry.third_person_id;
			message.coop_id = entry.coop_id;
			message.vag = entry.vag;
			message.character = entry.character;
		}
	}
	
	static void write(OutBuffer dest, const std::vector<HelpMessage>& src, Game game)
	{
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
			HelpMessageEntry entry = {};
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

template <bool is_korean>
struct BinHelpMessageBlock
{
	static void read(std::vector<u8>& dest, Buffer src, Game game)
	{
		auto& header = src.read<HelpMessageHeader>(0, "string block header");
		
		s32 size;
		if(game == Game::UYA || game == Game::DL) {
			size = header.size + sizeof(HelpMessageHeader);
		} else {
			size = header.size;
		}
		
		dest = src.read_multiple<u8>(0, size, "help messages").copy();
	}
	
	static void write(OutBuffer dest, const std::vector<u8>& src, Game game)
	{
		dest.write_multiple(src);
	}
};

static std::vector<std::vector<glm::vec4>> read_splines(Buffer src, s32 count, s32 data_offset)
{
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

static s32 write_splines(OutBuffer dest, const std::vector<std::vector<glm::vec4>>& src)
{
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

struct PathBlock
{
	static void read(std::vector<PathInstance>& dest, Buffer src, Game game)
	{
		auto& header = src.read<PathBlockHeader>(0, "path block header");
		std::vector<std::vector<glm::vec4>> splines = read_splines(src.subbuf(0x10), header.spline_count, header.data_offset - 0x10);
		for(size_t i = 0; i < splines.size(); i++) {
			PathInstance& inst = dest.emplace_back();
			inst.set_id_value(i);
			inst.spline() = std::move(splines[i]);
		}
	}
	
	static void write(OutBuffer dest, const std::vector<PathInstance>& src, Game game)
	{
		std::vector<std::vector<glm::vec4>> splines;
		for(const PathInstance& inst : src) {
			splines.emplace_back(inst.spline());
		}
		
		s64 header_pos = dest.alloc<PathBlockHeader>();
		PathBlockHeader header = {};
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
	s32 pad;
)

struct GrindPathBlock
{
	static void read(Gameplay& gameplay, Buffer src, Game game)
	{
		auto& header = src.read<PathBlockHeader>(0, "spline block header");
		auto grindpaths = src.read_multiple<GrindPathData>(0x10, header.spline_count, "grindrail data");
		s64 offsets_pos = 0x10 + header.spline_count * sizeof(GrindPathData);
		auto splines = read_splines(src.subbuf(offsets_pos), header.spline_count, header.data_offset - offsets_pos);
		gameplay.grind_paths.emplace();
		gameplay.grind_paths->reserve(header.spline_count);
		for(s64 i = 0; i < header.spline_count; i++) {
			GrindPathInstance& inst = gameplay.grind_paths->emplace_back();
			inst.set_id_value(i);
			inst.unknown_4 = grindpaths[i].unknown_4;
			inst.wrap = grindpaths[i].wrap;
			inst.inactive = grindpaths[i].inactive;
			inst.spline() = splines[i];
		}
	}
	
	static bool write(OutBuffer dest, const Gameplay& gameplay, Game game)
	{
		s64 header_ofs = dest.alloc<PathBlockHeader>();
		std::vector<std::vector<glm::vec4>> splines;
		for(const GrindPathInstance& inst : opt_iterator(gameplay.grind_paths)) {
			std::pair<const glm::vec4*, size_t> spline = {
				inst.spline().data(),
				inst.spline().size()
			};
			
			GrindPathData packed = {};
			packed.bounding_sphere = Vec4f::pack(approximate_bounding_sphere(nullptr, 0, &spline, 1));
			packed.unknown_4 = inst.unknown_4;
			packed.wrap = inst.wrap;
			packed.inactive = inst.inactive;
			dest.write(packed);
			splines.emplace_back(inst.spline());
		}
		PathBlockHeader header = {};
		header.spline_count = gameplay.grind_paths->size();
		s32 abs_data_offset = write_splines(dest, splines);
		header.data_offset = abs_data_offset - header_ofs;
		header.data_size = dest.tell() - abs_data_offset;
		dest.write(header_ofs, header);
		
		return true;
	}
};

packed_struct(ShapePacked,
	/* 0x00 */ Mat4 matrix;
	/* 0x40 */ Mat3 inverse_matrix;
	/* 0x70 */ Vec3f rotation;
	/* 0x7c */ f32 unused_7c;
)

static void swap_instance(CuboidInstance& l, ShapePacked& r)
{
	swap_matrix_inverse_rotation(l, r);
	r.unused_7c = 0.f;
}

static void swap_instance(SphereInstance& l, ShapePacked& r)
{
	swap_matrix_inverse_rotation(l, r);
	r.unused_7c = 0.f;
}

static void swap_instance(CylinderInstance& l, ShapePacked& r)
{
	swap_matrix_inverse_rotation(l, r);
	r.unused_7c = 0.f;
}

static void swap_instance(PillInstance& l, ShapePacked& r)
{
	swap_matrix_inverse_rotation(l, r);
	r.unused_7c = 0.f;
}

packed_struct(AreasHeader,
	/* 0x00 */ s32 area_count;
	/* 0x04 */ s32 part_offsets[5];
	/* 0x1c */ s32 unused_1c;
	/* 0x20 */ s32 unused_20;
)

packed_struct(GameplayAreaPacked,
	/* 0x00 */ Vec4f bounding_sphere;
	/* 0x10 */ s16 part_counts[5];
	/* 0x1a */ s16 last_update_time;
	/* 0x1c */ s32 relative_part_offsets[5];
)

enum AreaPart
{
	AREA_PART_PATHS = 0,
	AREA_PART_CUBOIDS = 1,
	AREA_PART_SPHERES = 2,
	AREA_PART_CYLINDERS = 3,
	AREA_PART_NEGATIVE_CUBOIDS = 4
};

struct AreasBlock
{
	static void read(Gameplay& gameplay, Buffer src, Game game)
	{
		src = src.subbuf(4); // Skip past size field.
		auto header = src.read<AreasHeader>(0, "area list block header");
		auto table = src.read_multiple<GameplayAreaPacked>(sizeof(AreasHeader), header.area_count, "area list table");
		gameplay.areas.emplace();
		gameplay.areas->reserve(table.size());
		for(s32 i = 0; i < (s32) table.size(); i++) {
			const GameplayAreaPacked& packed = table[i];
			AreaInstance& inst = gameplay.areas->emplace_back();
			inst.set_id_value(i);
			inst.last_update_time = packed.last_update_time;
			
			s32 paths_ofs = header.part_offsets[AREA_PART_PATHS] + packed.relative_part_offsets[AREA_PART_PATHS];
			auto paths = src.read_multiple<s32>(paths_ofs, packed.part_counts[AREA_PART_PATHS], "area path links").copy();
			for(s32 link : paths) {
				inst.paths.emplace_back(link);
			}
			
			s32 cuboids_ofs = header.part_offsets[AREA_PART_CUBOIDS] + packed.relative_part_offsets[AREA_PART_CUBOIDS];
			auto cuboids = src.read_multiple<s32>(cuboids_ofs, packed.part_counts[AREA_PART_CUBOIDS], "area cuboid links").copy();
			for(s32 link : cuboids) {
				inst.cuboids.emplace_back(link);
			}
			
			s32 spheres_ofs = header.part_offsets[AREA_PART_SPHERES] + packed.relative_part_offsets[AREA_PART_SPHERES];
			auto spheres = src.read_multiple<s32>(spheres_ofs, packed.part_counts[AREA_PART_SPHERES], "area sphere links").copy();
			for(s32 link : spheres) {
				inst.spheres.emplace_back(link);
			}
			
			s32 cylinders_ofs = header.part_offsets[AREA_PART_CYLINDERS] + packed.relative_part_offsets[AREA_PART_CYLINDERS];
			auto cylinders = src.read_multiple<s32>(cylinders_ofs, packed.part_counts[AREA_PART_CYLINDERS], "cylinder links").copy();
			for(s32 link : cylinders) {
				inst.cylinders.emplace_back(link);
			}
			
			s32 negative_cuboids_ofs = header.part_offsets[AREA_PART_NEGATIVE_CUBOIDS] + packed.relative_part_offsets[AREA_PART_NEGATIVE_CUBOIDS];
			auto negative_cuboids = src.read_multiple<s32>(negative_cuboids_ofs, packed.part_counts[AREA_PART_NEGATIVE_CUBOIDS], "negative cuboids").copy();
			for(s32 link : negative_cuboids) {
				inst.negative_cuboids.emplace_back(link);
			}
		}
	}
	
	static bool write(OutBuffer dest, const Gameplay& gameplay, Game game)
	{
		s64 size_ofs = dest.alloc<s32>();
		s64 header_ofs = dest.alloc<AreasHeader>();
		s64 table_ofs = dest.alloc_multiple<GameplayAreaPacked>(opt_size(gameplay.areas));
		
		std::vector<GameplayAreaPacked> table;
		std::vector<s32> links[5];
		
		for(const AreaInstance& inst : opt_iterator(gameplay.areas)) {
			GameplayAreaPacked& packed = table.emplace_back();
			packed.last_update_time = inst.last_update_time;
			
			std::vector<const glm::mat4*> cuboids;
			std::vector<std::pair<const glm::vec4*, size_t>> splines;
			
			packed.relative_part_offsets[AREA_PART_PATHS] = (s32) !inst.paths.empty()
				? (links[AREA_PART_PATHS].size() * 4) : 0;
			packed.part_counts[AREA_PART_PATHS] = (s32) inst.paths.size();
			for(pathlink link : inst.paths) {
				links[AREA_PART_PATHS].emplace_back(link.id);
				verify(gameplay.paths.has_value(), "Path referenced, but there is no path block.");
				const PathInstance& path = (*gameplay.paths).at(link.id);
				splines.emplace_back(path.spline().data(), path.spline().size());
			}
			
			packed.relative_part_offsets[AREA_PART_CUBOIDS] = (s32) !inst.cuboids.empty()
				? (links[AREA_PART_CUBOIDS].size() * 4) : 0;
			packed.part_counts[AREA_PART_CUBOIDS] = (s32) inst.cuboids.size();
			for(cuboidlink link : inst.cuboids) {
				links[AREA_PART_CUBOIDS].emplace_back(link.id);
				verify(gameplay.cuboids.has_value(), "Cuboid referenced, but there is no cuboid block.");
				cuboids.emplace_back(&(*gameplay.cuboids).at(link.id).transform().matrix());
			}
			
			packed.relative_part_offsets[AREA_PART_SPHERES] = (s32) !inst.spheres.empty()
				? (links[AREA_PART_SPHERES].size() * 4) : 0;
			packed.part_counts[AREA_PART_SPHERES] = (s32) inst.spheres.size();
			for(spherelink link : inst.spheres) {
				links[AREA_PART_SPHERES].emplace_back(link.id);
				verify(gameplay.spheres.has_value(), "Spheres referenced, but there is no spheres block.");
				cuboids.emplace_back(&(*gameplay.spheres).at(link.id).transform().matrix());
			}
			
			packed.relative_part_offsets[AREA_PART_CYLINDERS] = (s32) !inst.cylinders.empty()
				? (links[AREA_PART_CYLINDERS].size() * 4) : 0;
			packed.part_counts[AREA_PART_CYLINDERS] = (s32) inst.cylinders.size();
			for(cylinderlink link : inst.cylinders) {
				links[AREA_PART_CYLINDERS].emplace_back(link.id);
				verify(gameplay.cylinders.has_value(), "Cylinders referenced, but there is no cylinder block.");
				cuboids.emplace_back(&(*gameplay.cylinders).at(link.id).transform().matrix());
			}
			
			packed.relative_part_offsets[AREA_PART_NEGATIVE_CUBOIDS] = (s32) !inst.negative_cuboids.empty()
				? (links[AREA_PART_NEGATIVE_CUBOIDS].size() * 4) : 0;
			packed.part_counts[AREA_PART_NEGATIVE_CUBOIDS] = (s32) inst.negative_cuboids.size();
			for(cuboidlink link : inst.negative_cuboids) {
				links[AREA_PART_NEGATIVE_CUBOIDS].emplace_back(link.id);
			}
			
			packed.bounding_sphere = Vec4f::pack(approximate_bounding_sphere(cuboids.data(), cuboids.size(), splines.data(), splines.size()));
		}
		
		AreasHeader header = {};
		header.area_count = (s32) opt_size(gameplay.areas);
		
		if(!links[AREA_PART_PATHS].empty()) {
			header.part_offsets[AREA_PART_PATHS] = (s32) (dest.tell() - header_ofs);
			dest.write_multiple(links[AREA_PART_PATHS]);
		}
		if(!links[AREA_PART_CUBOIDS].empty()) {
			header.part_offsets[AREA_PART_CUBOIDS] = (s32) (dest.tell() - header_ofs);
			dest.write_multiple(links[AREA_PART_CUBOIDS]);
		}
		if(!links[AREA_PART_SPHERES].empty()) {
			header.part_offsets[AREA_PART_SPHERES] = (s32) (dest.tell() - header_ofs);
			dest.write_multiple(links[AREA_PART_SPHERES]);
		}
		if(!links[AREA_PART_CYLINDERS].empty()) {
			header.part_offsets[AREA_PART_CYLINDERS] = (s32) (dest.tell() - header_ofs);
			dest.write_multiple(links[AREA_PART_CYLINDERS]);
		}
		if(!links[AREA_PART_NEGATIVE_CUBOIDS].empty()) {
			header.part_offsets[AREA_PART_NEGATIVE_CUBOIDS] = (s32) (dest.tell() - header_ofs);
			dest.write_multiple(links[AREA_PART_NEGATIVE_CUBOIDS]);
		}
		
		dest.write(size_ofs, dest.tell() - header_ofs);
		dest.write(header_ofs, header);
		dest.write_multiple(table_ofs, table);
		
		return true;
	}
};

packed_struct(OcclusionMappingsGameplayHeader,
	s32 tfrag_mapping_count;
	s32 tie_mapping_count;
	s32 moby_mapping_count;
	s32 pad = 0;
)

struct OcclusionMappingsBlock
{
	static void read(std::vector<u8>& dest, Buffer src, Game game)
	{
		auto& header = src.read<OcclusionMappingsGameplayHeader>(0, "occlusion header");
		s32 total_count = header.tfrag_mapping_count + header.tie_mapping_count + header.moby_mapping_count;
		dest = src.read_multiple<u8>(0, 0x10 + total_count * 8).copy();
	}
	
	static void write(OutBuffer dest, const std::vector<u8>& src, Game game)
	{
		dest.write_multiple(src);
	}
};

std::vector<u8> write_occlusion_mappings(const Gameplay& gameplay, Game game)
{
	std::vector<u8> dest;
	if(gameplay.occlusion.has_value()) {
		OcclusionMappingsBlock::write(dest, *gameplay.occlusion, game);
	}
	return dest;
}

#endif
