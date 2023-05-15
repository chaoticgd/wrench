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

#ifndef INSTANCEMGR_GAMEPLAY_IMPL_ENV_H
#define INSTANCEMGR_GAMEPLAY_IMPL_ENV_H

#include <instancemgr/gameplay_impl_common.h>

packed_struct(RacEnvSamplePointPacked,
	/* 0x00 */ Vec3f pos;
	/* 0x0c */ f32 one;
	/* 0x10 */ s32 hero_colour_r;
	/* 0x14 */ s32 hero_colour_g;
	/* 0x18 */ s32 hero_colour_b;
	/* 0x1c */ s32 hero_light;
	/* 0x20 */ s32 reverb_depth;
	/* 0x24 */ u8 reverb_type;
	/* 0x25 */ u8 reverb_delay;
	/* 0x26 */ u8 reverb_feedback;
	/* 0x27 */ u8 enable_reverb_params;
	/* 0x28 */ s32 music_track;
	/* 0x2c */ s32 unused_2c;
)

struct RacEnvSamplePointBlock {
	static void read(std::vector<EnvSamplePointInstance>& dest, Buffer src, Game game) {
		TableHeader header = src.read<TableHeader>(0, "env sample points block header");
		auto data = src.read_multiple<RacEnvSamplePointPacked>(0x10, header.count_1, "env sample points");
		dest.reserve(header.count_1);
		for(s64 i = 0; i < header.count_1; i++) {
			RacEnvSamplePointPacked packed = data[i];
			EnvSamplePointInstance& inst = dest.emplace_back();
			inst.set_id_value(i);
			inst.set_transform(packed.pos.unpack());
			swap_env_params(inst, packed);
		}
	}
	
	static void write(OutBuffer dest, const std::vector<EnvSamplePointInstance>& src, Game game) {
		TableHeader header = {(s32) src.size()};
		dest.write(header);
		for(EnvSamplePointInstance inst : src) {
			RacEnvSamplePointPacked packed;
			packed.pos = Vec3f::pack(inst.position());
			packed.one = 1.f;
			swap_env_params(inst, packed);
			packed.unused_2c = 0x11223344;
			dest.write(packed);
		}
	}
	
	static void swap_env_params(EnvSamplePointInstance& l, RacEnvSamplePointPacked& r) {
		SWAP_PACKED(l.hero_colour_r, r.hero_colour_r);
		SWAP_PACKED(l.hero_colour_g, r.hero_colour_g);
		SWAP_PACKED(l.hero_colour_b, r.hero_colour_b);
		SWAP_PACKED(l.hero_light, r.hero_light);
		SWAP_PACKED(l.reverb_depth, r.reverb_depth);
		SWAP_PACKED(l.reverb_type, r.reverb_type);
		SWAP_PACKED(l.reverb_delay, r.reverb_delay);
		SWAP_PACKED(l.reverb_feedback, r.reverb_feedback);
		SWAP_PACKED(l.enable_reverb_params, r.enable_reverb_params);
		SWAP_PACKED(l.music_track, r.music_track);
	}
};

// Fog only applied if fog_near_dist < fog_far_dist.
packed_struct(GcUyaDlEnvSamplePointPacked,
	/* 0x00 */ s32 hero_light;
	/* 0x04 */ s16 pos_x;
	/* 0x06 */ s16 pos_y;
	/* 0x08 */ s16 pos_z;
	/* 0x0a */ s16 reverb_depth;
	/* 0x0c */ s16 music_track;
	/* 0x0e */ u8 fog_near_intensity;
	/* 0x0f */ u8 fog_far_intensity;
	/* 0x10 */ u8 hero_colour_r;
	/* 0x11 */ u8 hero_colour_g;
	/* 0x12 */ u8 hero_colour_b;
	/* 0x13 */ u8 reverb_type;
	/* 0x14 */ u8 reverb_delay;
	/* 0x15 */ u8 reverb_feedback;
	/* 0x16 */ u8 enable_reverb_params;
	/* 0x17 */ u8 fog_r;
	/* 0x18 */ u8 fog_g;
	/* 0x19 */ u8 fog_b;
	/* 0x1a */ s16 fog_near_dist;
	/* 0x1c */ s16 fog_far_dist;
	/* 0x1e */ u16 unused_1e;
)

struct GcUyaDlEnvSamplePointBlock {
	static void read(std::vector<EnvSamplePointInstance>& dest, Buffer src, Game game) {
		TableHeader header = src.read<TableHeader>(0, "env sample points block header");
		auto data = src.read_multiple<GcUyaDlEnvSamplePointPacked>(0x10, header.count_1, "env sample points");
		dest.reserve(header.count_1);
		for(s64 i = 0; i < header.count_1; i++) {
			GcUyaDlEnvSamplePointPacked packed = data[i];
			EnvSamplePointInstance& inst = dest.emplace_back();
			inst.set_id_value(i);
			inst.set_transform(glm::vec3(packed.pos_x, packed.pos_y, packed.pos_z));
			if(packed.fog_far_dist > packed.fog_near_dist) {
				inst.enable_fog_params = true;
				inst.fog_near_dist = packed.fog_near_dist;
				inst.fog_far_dist = packed.fog_far_dist;
			}
			swap_env_params(inst, packed);
		}
	}
	
	static void write(OutBuffer dest, const std::vector<EnvSamplePointInstance>& src, Game game) {
		TableHeader header = {(s32) src.size()};
		dest.write(header);
		for(EnvSamplePointInstance inst : src) {
			GcUyaDlEnvSamplePointPacked packed;
			packed.pos_x = (s16) inst.position().x;
			packed.pos_y = (s16) inst.position().y;
			packed.pos_z = (s16) inst.position().z;
			if(inst.enable_fog_params) {
				packed.fog_near_dist = inst.fog_near_dist;
				packed.fog_far_dist = inst.fog_far_dist;
			} else {
				packed.fog_near_dist = 0;
				packed.fog_far_dist = 0;
			}
			packed.unused_1e = 0xffff;
			swap_env_params(inst, packed);
			dest.write(packed);
		}
	}
	
	static void swap_env_params(EnvSamplePointInstance& l, GcUyaDlEnvSamplePointPacked& r) {
		SWAP_PACKED(l.hero_light, r.hero_light);
		SWAP_PACKED(l.reverb_depth, r.reverb_depth);
		SWAP_PACKED(l.music_track, r.music_track);
		SWAP_PACKED(l.fog_near_intensity, r.fog_near_intensity);
		SWAP_PACKED(l.fog_far_intensity, r.fog_far_intensity);
		SWAP_PACKED(l.hero_colour_r, r.hero_colour_r);
		SWAP_PACKED(l.hero_colour_g, r.hero_colour_g);
		SWAP_PACKED(l.hero_colour_b, r.hero_colour_b);
		SWAP_PACKED(l.reverb_type, r.reverb_type);
		SWAP_PACKED(l.reverb_delay, r.reverb_delay);
		SWAP_PACKED(l.reverb_feedback, r.reverb_feedback);
		SWAP_PACKED(l.enable_reverb_params, r.enable_reverb_params);
		SWAP_PACKED(l.fog_r, r.fog_r);
		SWAP_PACKED(l.fog_g, r.fog_g);
		SWAP_PACKED(l.fog_b, r.fog_b);
	}
};

packed_struct(EnvTransitionPacked,
	/* 0x00 */ Mat4 matrix;
	/* 0x40 */ Rgb32 hero_colour_1;
	/* 0x44 */ Rgb32 hero_colour_2;
	/* 0x48 */ s32 hero_light_1;
	/* 0x4c */ s32 hero_light_2;
	/* 0x50 */ u32 flags;
	/* 0x54 */ Rgb32 fog_colour_1;
	/* 0x58 */ Rgb32 fog_colour_2;
	/* 0x5c */ f32 fog_near_dist_1;
	/* 0x60 */ f32 fog_near_intensity_1;
	/* 0x64 */ f32 fog_far_dist_1;
	/* 0x68 */ f32 fog_far_intensity_1;
	/* 0x6c */ f32 fog_near_dist_2;
	/* 0x70 */ f32 fog_near_intensity_2;
	/* 0x74 */ f32 fog_far_dist_2;
	/* 0x78 */ f32 fog_far_intensity_2;
	/* 0x7c */ s32 unused_7c;
)

struct EnvTransitionBlock {
	static void read(std::vector<EnvTransitionInstance>& dest, Buffer src, Game game) {
		TableHeader header = src.read<TableHeader>(0, "env transitions block header");
		s64 ofs = 0x10;
		auto bspheres = src.read_multiple<Vec4f>(ofs, header.count_1, "env transition bspheres");
		ofs += header.count_1 * sizeof(Vec4f);
		auto data = src.read_multiple<EnvTransitionPacked>(ofs, header.count_1, "env transitions");
		dest.reserve(header.count_1);
		for(s64 i = 0; i < header.count_1; i++) {
			EnvTransitionPacked packed = data[i];
			EnvTransitionInstance& inst = dest.emplace_back();
			inst.set_id_value(i);
			inst.set_transform(packed.matrix.unpack());
			inst.bounding_sphere() = bspheres[i].unpack();
			inst.enable_hero = packed.flags & 1;
			inst.enable_fog = (packed.flags & 2) >> 1;
			swap_env_transition(inst, packed);
		}
	}
	
	static void write(OutBuffer dest, const std::vector<EnvTransitionInstance>& src, Game game) {
		TableHeader header = {(s32) src.size()};
		dest.write(header);
		for(const EnvTransitionInstance& inst : src) {
			dest.write(Vec4f::pack(inst.bounding_sphere()));
		}
		for(EnvTransitionInstance inst : src) {
			EnvTransitionPacked packed;
			packed.matrix = Mat4::pack(inst.matrix());
			packed.flags = inst.enable_hero | (inst.enable_fog << 1);
			packed.unused_7c = 0;
			swap_env_transition(inst, packed);
			dest.write(packed);
		}
	}
	
	static void swap_env_transition(EnvTransitionInstance& l, EnvTransitionPacked& r) {
		SWAP_PACKED(l.hero_colour_1, r.hero_colour_1);
		SWAP_PACKED(l.hero_colour_2, r.hero_colour_2);
		SWAP_PACKED(l.hero_light_1, r.hero_light_1);
		SWAP_PACKED(l.hero_light_2, r.hero_light_2);
		SWAP_PACKED(l.fog_colour_1, r.fog_colour_1);
		SWAP_PACKED(l.fog_colour_2, r.fog_colour_2);
		SWAP_PACKED(l.fog_near_dist_1, r.fog_near_dist_1);
		SWAP_PACKED(l.fog_near_intensity_1, r.fog_near_intensity_1);
		SWAP_PACKED(l.fog_far_dist_1, r.fog_far_dist_1);
		SWAP_PACKED(l.fog_far_intensity_1, r.fog_far_intensity_1);
		SWAP_PACKED(l.fog_near_dist_2, r.fog_near_dist_2);
		SWAP_PACKED(l.fog_near_intensity_2, r.fog_near_intensity_2);
		SWAP_PACKED(l.fog_far_dist_2, r.fog_far_dist_2);
		SWAP_PACKED(l.fog_far_intensity_2, r.fog_far_intensity_2);
	}
};

packed_struct(CamCollGridPrim,
	/* 0x00 */ Vec4f bsphere;
	/* 0x10 */ s32 type;
	/* 0x14 */ s32 index;
	/* 0x18 */ s32 flags;
	/* 0x1c */ s32 i_value;
	/* 0x20 */ f32 f_value;
	/* 0x24 */ s32 pad[3];
)

enum CamCollGridVolumeType {
	CAM_COLL_CUBOID = 3,
	CAM_COLL_SPHERE = 5,
	CAM_COLL_CYLINDER = 6,
	CAM_COLL_PILL = 7
};

struct CamCollGridBlock {
	static const s32 GRID_SIZE_X = 0x40;
	static const s32 GRID_SIZE_Y = 0x40;
	
	static void read(PvarTypes& types, Gameplay& dest, Buffer src, Game game) {
		auto grid = src.read_multiple<s32>(0x10, GRID_SIZE_X * GRID_SIZE_Y, "camera collision grid");
		for(s32 list_offset : grid) {
			if(list_offset != 0) {
				s32 prim_count = src.read<s32>(0x10 + list_offset);
				for(s32 i = 0; i < prim_count; i++) {
					s32 prim_offset = 0x10 + list_offset + 0x10 + i * sizeof(CamCollGridPrim);
					const CamCollGridPrim& prim = src.read<CamCollGridPrim>(prim_offset, "camera collision grid primitive");
					Instance* instance;
					switch(prim.type) {
						case CAM_COLL_CUBOID: {
							verify(dest.cuboids.has_value(), "Referenced cuboid doesn't exist.");
							instance = &dest.cuboids->at(prim.index);
							break;
						}
						case CAM_COLL_SPHERE: {
							verify(dest.spheres.has_value(), "Referenced sphere doesn't exist.");
							instance = &dest.spheres->at(prim.index);
							break;
						}
						case CAM_COLL_CYLINDER: {
							verify(dest.cylinders.has_value(), "Referenced cylinder doesn't exist.");
							instance = &dest.cylinders->at(prim.index);
							break;
						}
						case CAM_COLL_PILL: {
							verify(dest.pills.has_value(), "Referenced pill doesn't exist.");
							instance = &dest.pills->at(prim.index);
							break;
						}
						default: verify_not_reached("Camera collision grid primitive has bad type field.");
					}
					CameraCollisionParams& params = instance->camera_collision();
					params.enabled = true;
					params.flags = prim.flags;
					params.i_value = prim.i_value;
					params.f_value = prim.f_value;
				}
			}
		}
	}
	
	static bool write(OutBuffer dest, const PvarTypes& types, const Gameplay& src, Game game) {
		// Determine which grid cells intersect with the types of volumes we
		// care about.
		std::vector<std::vector<CamCollGridPrim>> grid(GRID_SIZE_X * GRID_SIZE_Y);
		for(s32 i = 0; i < (s32) opt_size(src.cuboids); i++) {
			populate_grid_with_instance(grid, (*src.cuboids)[i], i);
		}
		for(s32 i = 0; i < (s32) opt_size(src.spheres); i++) {
			populate_grid_with_instance(grid, (*src.spheres)[i], i);
		}
		for(s32 i = 0; i < (s32) opt_size(src.cylinders); i++) {
			populate_grid_with_instance(grid, (*src.cylinders)[i], i);
		}
		for(s32 i = 0; i < (s32) opt_size(src.pills); i++) {
			populate_grid_with_instance(grid, (*src.pills)[i], i);
		}
		
		// Write out the primitives.
		s64 header_ofs = dest.alloc<TableHeader>();
		s64 grid_ofs = dest.alloc_multiple<s32>(GRID_SIZE_X * GRID_SIZE_Y);
		std::vector<s32> offsets(GRID_SIZE_X * GRID_SIZE_Y, 0);
		for(s32 y = 0; y < GRID_SIZE_Y; y++) {
			for(s32 x = 0; x < GRID_SIZE_X; x++) {
				std::vector<CamCollGridPrim>& prims = grid[y * GRID_SIZE_X + x];
				if(!prims.empty()) {
					dest.pad(0x10);
					offsets[y * GRID_SIZE_X + x] = (s32) (dest.tell() - grid_ofs);
					TableHeader prim_header = {};
					prim_header.count_1 = (s32) prims.size();
					dest.write<TableHeader>(prim_header);
					dest.write_multiple(prims);
				}
			}
		}
		
		// Write out the header and grid.
		TableHeader header = {};
		header.count_1 = (s32) (dest.tell() - header_ofs - 0x4);
		dest.write(header_ofs, header);
		dest.write_multiple(grid_ofs, offsets);
		
		return true;
	}
	
	static void populate_grid_with_instance(std::vector<std::vector<CamCollGridPrim>>& grid, const Instance& instance, s32 index) {
		const CameraCollisionParams& params = instance.camera_collision();
		if(!params.enabled) {
			return;
		}
		
		glm::mat4 matrix = instance.matrix();
		matrix[3][3] = 1.f;
		
		// Detect which grid cells the cuboid could intersect with.
		s32 xmin = INT32_MAX, xmax = 0;
		s32 ymin = INT32_MAX, ymax = 0;
		std::vector<Vertex> bpshere_points;
		for(s32 z = -1; z <= 1; z += 2) {
			for(s32 y = -1; y <= 1; y += 2) {
				for(s32 x = -1; x <= 1; x += 2) {
					glm::vec3 corner = matrix * glm::vec4(x, y, z, 1.f);
					s32 cxmin = corner.x * 0.0625f;
					s32 cymin = corner.y * 0.0625f;
					s32 cxmax = ceilf(corner.x * 0.0625f);
					s32 cymax = ceilf(corner.y * 0.0625f);
					if(cxmin < xmin) xmin = cxmin;
					if(cymin < ymin) ymin = cymin;
					if(cxmax > xmax) xmax = cxmax;
					if(cymax > ymax) ymax = cymax;
					Vertex& v = bpshere_points.emplace_back();
					v.pos = corner;
				}
			}
		}
		
		// Calculate bounding sphere.
		glm::vec4 bsphere = approximate_bounding_sphere(bpshere_points);
		
		// Handle edge cases.
		if(xmin == xmax) { xmin--; xmax++; }
		if(ymin == ymax) { ymin--; ymax++; }
		if(xmin < 0) xmin = 0;
		if(ymin < 0) ymin = 0;
		if(xmax >= GRID_SIZE_X) xmax = GRID_SIZE_X;
		if(ymax >= GRID_SIZE_Y) ymax = GRID_SIZE_Y;
		
		// Populate the grid.
		for(s32 y = ymin; y < ymax; y++) {
			for(s32 x = xmin; x < xmax; x++) {
				CamCollGridPrim& prim = grid[y * GRID_SIZE_X + x].emplace_back();
				prim.bsphere = Vec4f::pack(bsphere);
				switch(instance.type()) {
					case InstanceType::INST_CUBOID: prim.type = CAM_COLL_CUBOID; break;
					case InstanceType::INST_SPHERE: prim.type = CAM_COLL_SPHERE; break;
					case InstanceType::INST_CYLINDER: prim.type = CAM_COLL_CYLINDER; break;
					case InstanceType::INST_PILL: prim.type = CAM_COLL_PILL; break;
					default: verify_not_reached_fatal("Instance is not a volume.");
				}
				prim.index = index;
				prim.flags = params.flags;
				prim.i_value = params.i_value;
				prim.f_value = params.f_value;
				prim.pad[0] = -1;
				prim.pad[1] = 0;
				prim.pad[2] = 0;
			}
		}
	}
};

struct RAC1_78_Block {
	static void read(std::vector<u8>& dest, Buffer src, Game game) {
		s32 size = src.read<s32>(0, "RAC 78 size");
		dest = src.read_multiple<u8>(4, size, "RAC 78 data").copy();
	}
	
	static void write(OutBuffer dest, const std::vector<u8>& src, Game game) {
		dest.write((s32) src.size());
		dest.write_multiple(src);
	}
};

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

packed_struct(DirectionalLightPacked,
	/* 0x00 */ Vec4f colour_a;
	/* 0x10 */ Vec4f direction_a;
	/* 0x20 */ Vec4f colour_b;
	/* 0x30 */ Vec4f direction_b;
)

static void swap_instance(DirectionalLight& l, DirectionalLightPacked& r) {
	r.colour_a.swap(l.colour_a);
	r.direction_a.swap(l.direction_a);
	r.colour_b.swap(l.colour_b);
	r.direction_b.swap(l.direction_b);
}

#endif
