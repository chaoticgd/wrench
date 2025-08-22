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

#include <instancemgr/gameplay_impl_common.inl>

//#define GAMEPLAY_DEBUG_LIGHT_GRID

packed_struct(RacEnvSamplePointPacked,
	/* 0x00 */ Vec3f pos;
	/* 0x0c */ f32 one;
	/* 0x10 */ Rgb96 hero_col;
	/* 0x1c */ s32 hero_light;
	/* 0x20 */ s32 reverb_depth;
	/* 0x24 */ u8 reverb_type;
	/* 0x25 */ u8 reverb_delay;
	/* 0x26 */ u8 reverb_feedback;
	/* 0x27 */ u8 enable_reverb_params;
	/* 0x28 */ s32 music_track;
	/* 0x2c */ s32 unused_2c;
)

struct RacEnvSamplePointBlock
{
	static void read(std::vector<EnvSamplePointInstance>& dest, Buffer src, Game game)
	{
		TableHeader header = src.read<TableHeader>(0, "env sample points block header");
		auto data = src.read_multiple<RacEnvSamplePointPacked>(0x10, header.count_1, "env sample points");
		dest.reserve(header.count_1);
		for(s64 i = 0; i < header.count_1; i++) {
			RacEnvSamplePointPacked packed = data[i];
			EnvSamplePointInstance& inst = dest.emplace_back();
			inst.set_id_value(i);
			inst.transform().set_from_pos_rot_scale(packed.pos.unpack());
			swap_env_params(inst, packed);
		}
	}
	
	static void write(OutBuffer dest, const std::vector<EnvSamplePointInstance>& src, Game game)
	{
		TableHeader header = {(s32) src.size()};
		dest.write(header);
		for(EnvSamplePointInstance inst : src) {
			RacEnvSamplePointPacked packed;
			packed.pos = Vec3f::pack(inst.transform().pos());
			packed.one = 1.f;
			swap_env_params(inst, packed);
			packed.unused_2c = 0x11223344;
			dest.write(packed);
		}
	}
	
	static void swap_env_params(EnvSamplePointInstance& l, RacEnvSamplePointPacked& r)
	{
		SWAP_COLOUR(l.hero_col, r.hero_col);
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
	/* 0x10 */ Rgb24 hero_col;
	/* 0x13 */ u8 reverb_type;
	/* 0x14 */ u8 reverb_delay;
	/* 0x15 */ u8 reverb_feedback;
	/* 0x16 */ u8 enable_reverb_params;
	/* 0x17 */ Rgb24 fog_col;
	/* 0x1a */ s16 fog_near_dist;
	/* 0x1c */ s16 fog_far_dist;
	/* 0x1e */ u16 unused_1e;
)

struct GcUyaDlEnvSamplePointBlock
{
	static void read(std::vector<EnvSamplePointInstance>& dest, Buffer src, Game game)
	{
		TableHeader header = src.read<TableHeader>(0, "env sample points block header");
		auto data = src.read_multiple<GcUyaDlEnvSamplePointPacked>(0x10, header.count_1, "env sample points");
		dest.reserve(header.count_1);
		for(s64 i = 0; i < header.count_1; i++) {
			GcUyaDlEnvSamplePointPacked packed = data[i];
			EnvSamplePointInstance& inst = dest.emplace_back();
			inst.set_id_value(i);
			f32 x = packed.pos_x * (1.f / 4.f);
			f32 y = packed.pos_y * (1.f / 4.f);
			f32 z = packed.pos_z * (1.f / 4.f);
			inst.transform().set_from_pos_rot_scale(glm::vec3(x, y, z));
			if(packed.fog_far_dist > packed.fog_near_dist) {
				inst.enable_fog_params = true;
				inst.fog_near_dist = packed.fog_near_dist;
				inst.fog_far_dist = packed.fog_far_dist;
			}
			swap_env_params(inst, packed);
		}
	}
	
	static void write(OutBuffer dest, const std::vector<EnvSamplePointInstance>& src, Game game)
	{
		TableHeader header = {(s32) src.size()};
		dest.write(header);
		for(EnvSamplePointInstance inst : src) {
			GcUyaDlEnvSamplePointPacked packed;
			const TransformComponent& transform = inst.transform();
			packed.pos_x = (s16) roundf(transform.pos().x * 4.f);
			packed.pos_y = (s16) roundf(transform.pos().y * 4.f);
			packed.pos_z = (s16) roundf(transform.pos().z * 4.f);
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
	
	static void swap_env_params(EnvSamplePointInstance& l, GcUyaDlEnvSamplePointPacked& r)
	{
		SWAP_PACKED(l.hero_light, r.hero_light);
		SWAP_PACKED(l.reverb_depth, r.reverb_depth);
		SWAP_PACKED(l.music_track, r.music_track);
		SWAP_PACKED(l.fog_near_intensity, r.fog_near_intensity);
		SWAP_PACKED(l.fog_far_intensity, r.fog_far_intensity);
		SWAP_COLOUR(l.hero_col, r.hero_col);
		SWAP_PACKED(l.reverb_type, r.reverb_type);
		SWAP_PACKED(l.reverb_delay, r.reverb_delay);
		SWAP_PACKED(l.reverb_feedback, r.reverb_feedback);
		SWAP_PACKED(l.enable_reverb_params, r.enable_reverb_params);
		SWAP_COLOUR(l.fog_col, r.fog_col);
	}
};

packed_struct(EnvTransitionPacked,
	/* 0x00 */ Mat4 inverse_matrix;
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

struct EnvTransitionBlock
{
	static void read(Gameplay& gameplay, Buffer src, Game game)
	{
		TableHeader header = src.read<TableHeader>(0, "env transitions block header");
		s64 ofs = 0x10;
		ofs += header.count_1 * sizeof(Vec4f);
		auto data = src.read_multiple<EnvTransitionPacked>(ofs, header.count_1, "env transitions");
		gameplay.env_transitions.emplace();
		gameplay.env_transitions->reserve(header.count_1);
		for(s64 i = 0; i < header.count_1; i++) {
			EnvTransitionPacked packed = data[i];
			EnvTransitionInstance& inst = gameplay.env_transitions->emplace_back();
			inst.set_id_value(i);
			glm::mat4 inverse_matrix = packed.inverse_matrix.unpack();
			glm::mat4 matrix = glm::inverse(inverse_matrix);
			inst.transform().set_from_matrix(&matrix, &inverse_matrix);
			inst.enable_hero = packed.flags & 1;
			inst.enable_fog = (packed.flags & 2) >> 1;
			swap_env_transition(inst, packed);
		}
	}
	
	static bool write(OutBuffer dest, const Gameplay& gameplay, Game game)
	{
		TableHeader header = {(s32) gameplay.env_transitions->size()};
		dest.write(header);
		for(const EnvTransitionInstance& inst : opt_iterator(gameplay.env_transitions)) {
			const glm::mat4* cuboid = &inst.transform().matrix();
			glm::vec4 bsphere = approximate_bounding_sphere(&cuboid, 1, nullptr, 0);
			dest.write(Vec4f::pack(bsphere));
		}
		for(EnvTransitionInstance inst : opt_iterator(gameplay.env_transitions)) {
			EnvTransitionPacked packed = {};
			packed.inverse_matrix = Mat4::pack(inst.transform().inverse_matrix());
			packed.flags = inst.enable_hero | (inst.enable_fog << 1);
			packed.unused_7c = 0;
			swap_env_transition(inst, packed);
			dest.write(packed);
		}
		
		return true;
	}
	
	static void swap_env_transition(EnvTransitionInstance& l, EnvTransitionPacked& r)
	{
		SWAP_COLOUR(l.hero_col_1, r.hero_colour_1);
		SWAP_COLOUR(l.hero_col_2, r.hero_colour_2);
		SWAP_PACKED(l.hero_light_1, r.hero_light_1);
		SWAP_PACKED(l.hero_light_2, r.hero_light_2);
		SWAP_COLOUR(l.fog_col_1, r.fog_colour_1);
		SWAP_COLOUR(l.fog_col_2, r.fog_colour_2);
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

enum CamCollGridVolumeType
{
	CAM_COLL_CUBOID = 3,
	CAM_COLL_SPHERE = 5,
	CAM_COLL_CYLINDER = 6,
	CAM_COLL_PILL = 7
};

struct CamCollGridBlock
{
	static const s32 GRID_SIZE_X = 0x40;
	static const s32 GRID_SIZE_Y = 0x40;
	
	static void read(Gameplay& dest, Buffer src, Game game)
	{
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
	
	static bool write(OutBuffer dest, const Gameplay& src, Game game)
	{
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
		
		// Write out the lists of primitives.
		s64 header_ofs = dest.alloc<TableHeader>();
		s64 grid_ofs = dest.alloc_multiple<s32>(GRID_SIZE_X * GRID_SIZE_Y);
		std::vector<s32> offsets(GRID_SIZE_X * GRID_SIZE_Y, 0);
		for(s32 y = 0; y < GRID_SIZE_Y; y++) {
			for(s32 x = 0; x < GRID_SIZE_X; x++) {
				std::vector<CamCollGridPrim>& prims = grid[y * GRID_SIZE_X + x];
				if(!prims.empty()) {
					dest.pad(0x10);
					offsets[y * GRID_SIZE_X + x] = (s32) (dest.tell() - grid_ofs);
					TableHeader list_header = {};
					list_header.count_1 = (s32) prims.size();
					dest.write<TableHeader>(list_header);
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
	
	static void populate_grid_with_instance(
		std::vector<std::vector<CamCollGridPrim>>& grid, const Instance& instance, s32 index)
	{
		const CameraCollisionParams& params = instance.camera_collision();
		if(!params.enabled) {
			return;
		}
		
		glm::mat4 matrix = instance.transform().matrix();
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
		bsphere.z = 0.f;
		
		// Handle edge cases.
		if(xmin < 0) xmin = 0;
		if(ymin < 0) ymin = 0;
		if(xmax > GRID_SIZE_X) xmax = GRID_SIZE_X;
		if(ymax > GRID_SIZE_Y) ymax = GRID_SIZE_Y;
		
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

// TODO: Fix up all this stuff later.

#ifdef GAMEPLAY_DEBUG_LIGHT_GRID
#define rgb(r, g ,b) {r, g, b, 0}
static Rgb32 palette[] = {
	rgb(255,0,0),
	rgb(255,255,0),
	rgb(0,234,255),
	rgb(170,0,255),
	rgb(255,127,0),
	rgb(191,255,0),
	rgb(0,149,255),
	rgb(255,0,170),
	rgb(255,212,0),
	rgb(106,255,0),
	rgb(0,64,255),
	rgb(237,185,185),
	rgb(185,215,237),
	rgb(231,233,185),
	rgb(220,185,237),
	rgb(185,237,224),
	rgb(143,35,35),
	rgb(35,98,143),
	rgb(143,106,35),
	rgb(107,35,143),
	rgb(79,143,35),
	rgb(0,0,0),
	rgb(115,115,115),
	rgb(204,204,204),
};
#define PXL(x, y) (((y) * 1024 + (x)) * 4)
#include <core/png.h>
#endif

struct PointLightGridBlock
{
	static const s32 GRID_SIZE_X = 0x40;
	static const s32 GRID_SIZE_Y = 0x40;
	
	static void read(Gameplay& dest, Buffer src, Game game)
	{
#ifdef GAMEPLAY_DEBUG_LIGHT_GRID
		Texture debug;
		debug.format = PixelFormat::RGBA;
		debug.width = 1024;
		debug.height = 1024;
		debug.data.resize(1024*1024*4, 0);
		srand(time(NULL));
		auto grid = src.read_multiple<s32>(0x10, GRID_SIZE_X * GRID_SIZE_Y, "camera collision grid");
		for(s32 y = 0; y < 0x40; y++) {
			for(s32 x = 0; x < 0x40; x++) {
				s32 list_offset = grid[y * GRID_SIZE_X + x];
				if(list_offset != 0) {
					s32 prim_count = src.read<s32>(0x10 + list_offset);
					for(s32 i = 0; i < prim_count; i++) {
						s32 index = src.read<s32>(0x10 + list_offset + 4+ i * 4);
						for(s32 j = 0; j < 16; j++) {
							if(j%prim_count!=i) continue;
							debug.data[PXL(x*16+j,y*16)+0] = palette[index%ARRAY_SIZE(palette)].r;
							debug.data[PXL(x*16+j,y*16)+1] = palette[index%ARRAY_SIZE(palette)].g;
							debug.data[PXL(x*16+j,y*16)+2] = palette[index%ARRAY_SIZE(palette)].b;
							debug.data[PXL(x*16+j,y*16)+3] = 255;
						}
						for(s32 j = 0; j < 16; j++) {
							if(j%prim_count!=i) continue;
							debug.data[PXL(x*16+j,y*16+15)+0] = palette[index%ARRAY_SIZE(palette)].r;
							debug.data[PXL(x*16+j,y*16+15)+1] = palette[index%ARRAY_SIZE(palette)].g;
							debug.data[PXL(x*16+j,y*16+15)+2] = palette[index%ARRAY_SIZE(palette)].b;
							debug.data[PXL(x*16+j,y*16+15)+3] = 255;
						}
					}
				}
			}
		}
		
		s32 col = 0;
		for(const PointLight& pl : opt_iterator(dest.point_lights)) {
			glm::vec2 pos(pl.position());
			s32 x = pos.x;
			s32 y = pos.y;
			if(x < 0) x =0;
			if(y < 0) y =0;
			if(x > 1023)x=1023;
			if(y>1023)y=1023;
			for(s32 i = 0; i < pl.radius; i++) {if(i%2==0&&i!=0&&i!=2)continue;
				debug.data[PXL(x,y+i)+0] = palette[col%ARRAY_SIZE(palette)].r;
				debug.data[PXL(x,y+i)+1] = palette[col%ARRAY_SIZE(palette)].g;
				debug.data[PXL(x,y+i)+2] = palette[col%ARRAY_SIZE(palette)].b;
				debug.data[PXL(x,y+i)+3] = 255;
			}
			col++;
		}
		
		std::vector<u8> png;
		MemoryOutputStream mos(png);
		write_png(mos, debug);
		write_file("/tmp/lights.png", png);
#endif
	}
	
	static bool write(OutBuffer dest, const Gameplay& src, Game game)
	{
		// Determine which grid cells intersect with the point lights.
		std::vector<std::vector<s32>> grid(GRID_SIZE_X * GRID_SIZE_Y);
		for(s32 i = 0; i < (s32) opt_size(src.point_lights); i++) {
			const PointLightInstance& light = (*src.point_lights)[i];
			glm::vec2 position = light.transform().pos();
			f32 radius = light.radius * 0.2f;
			
			s32 xmin = (s32) floorf((position.x - radius) * 0.0625f);
			s32 ymin = (s32) floorf((position.y - radius) * 0.0625f);
			s32 xmax = (s32) ceilf((position.x + radius) * 0.0625f);
			s32 ymax = (s32) ceilf((position.y + radius) * 0.0625f);
			
			// Handle edge cases.
			if(xmin < 0) xmin = 0;
			if(ymin < 0) ymin = 0;
			if(xmax > GRID_SIZE_X) xmax = GRID_SIZE_X;
			if(ymax > GRID_SIZE_Y) ymax = GRID_SIZE_Y;
			
			for(s32 y = ymin; y < ymax; y++) {
				for(s32 x = xmin; x < xmax; x++) {
					grid[y * GRID_SIZE_X + x].emplace_back(i);
				}
			}
		}
		
		// Write out the lists of lights.
		s64 header_ofs = dest.alloc<TableHeader>();
		s64 grid_ofs = dest.alloc_multiple<s32>(GRID_SIZE_X * GRID_SIZE_Y);
		std::vector<s32> offsets(GRID_SIZE_X * GRID_SIZE_Y, 0);
		for(s32 y = 0; y < GRID_SIZE_Y; y++) {
			for(s32 x = 0; x < GRID_SIZE_X; x++) {
				std::vector<s32>& lights = grid[y * GRID_SIZE_X + x];
				if(!lights.empty()) {
					dest.pad(0x10);
					offsets[y * GRID_SIZE_X + x] = (s32) (dest.tell() - grid_ofs);
					dest.write<s32>((s32) lights.size());
					dest.write_multiple(lights);
				}
			}
		}
		dest.pad(0x10);
		
		// Write out the header and grid.
		TableHeader header = {};
		header.count_1 = (s32) (dest.tell() - header_ofs - 0x4);
		dest.write(header_ofs, header);
		dest.write_multiple(grid_ofs, offsets);
		
		return true;
	}
	
	static bool sphere_intersects_grid_cell(const glm::vec2& position, f32 radius, s32 x, s32 y)
	{
		glm::vec2 grid_cell_centre = glm::vec2(x, y) * 16.f + 8.f;
		glm::vec2 relative = glm::abs(position - grid_cell_centre);
		if(relative.x > 8.f + radius || relative.y > 8.f + radius) return false;
		if(relative.x < 8.f || relative.y < 8.f) return true;
		return glm::distance(relative, glm::vec2(8.f, 8.f)) < radius;
	}
};

packed_struct(CameraPacked,
	/* 0x00 */ s32 type;
	/* 0x04 */ Vec3f position;
	/* 0x10 */ Vec3f rotation;
	/* 0x1c */ s32 pvar_index;
)

static void swap_instance(CameraInstance& l, CameraPacked& r)
{
	swap_position_rotation(l, r);
	SWAP_PACKED(l.pvars().temp_pvar_index, r.pvar_index);
	SWAP_PACKED(l.o_class(), r.type);
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

static void swap_instance(SoundInstance& l, SoundInstancePacked& r)
{
	swap_matrix_inverse_rotation(l, r);
	SWAP_PACKED(l.pvars().temp_pvar_index, r.pvar_index);
	SWAP_PACKED(l.o_class(), r.o_class);
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

static void swap_instance(DirLightInstance& l, DirectionalLightPacked& r)
{
	r.colour_a.swap(l.col_a);
	r.direction_a.swap(l.dir_a);
	r.colour_b.swap(l.col_b);
	r.direction_b.swap(l.dir_b);
}

packed_struct(PointLightPacked,
	/* 0x00 */ Vec3f position;
	/* 0x0c */ f32 radius;
	/* 0x10 */ Rgb32 colour;
	/* 0x14 */ u32 unused_14;
	/* 0x18 */ u32 unused_18;
	/* 0x1c */ u32 unused_1c;
)

static void swap_instance(PointLightInstance& l, PointLightPacked& r)
{
	swap_position(l, r);
	SWAP_PACKED(l.radius, r.radius);
	SWAP_COLOUR(l.colour(), r.colour);
	r.colour.pad = 0;
	r.unused_14 = 0;
	r.unused_18 = 0;
	r.unused_1c = 0;
}

packed_struct(GcUyaPointLightPacked,
	/* 0x0 */ u16 pos_x;
	/* 0x2 */ u16 pos_y;
	/* 0x4 */ u16 pos_z;
	/* 0x6 */ u16 radius;
	/* 0x8 */ u16 colour_r;
	/* 0xa */ u16 colour_g;
	/* 0xc */ u16 colour_b;
	/* 0xe */ u16 unused_e;
)

struct GcUyaPointLightsBlock
{
	static void read(std::vector<PointLightInstance>& dest, Buffer src, Game game)
	{
		const TableHeader& header = src.read<TableHeader>(0, "point lights header");
		for(s32 i = 0; i < header.count_1; i++) {
			const GcUyaPointLightPacked& packed = src.read<GcUyaPointLightPacked>(0x10 + 0x800 + i * 0x10, "point light");
			PointLightInstance& inst = dest.emplace_back();
			inst.set_id_value(i);
			glm::vec3 pos;
			pos.x = packed.pos_x * (1.f / 64.f);
			pos.y = packed.pos_y * (1.f / 64.f);
			pos.z = packed.pos_z * (1.f / 64.f);
			inst.transform().set_from_pos_rot_scale(pos);
			inst.radius = packed.radius * (1.f / 64.f);
			inst.colour().r = packed.colour_r * (1.f / 65535.f);
			inst.colour().g = packed.colour_g * (1.f / 65535.f);
			inst.colour().b = packed.colour_b * (1.f / 65535.f);
		}
	}
	
	static void write(OutBuffer dest, const std::vector<PointLightInstance>& src, Game game)
	{
		verify(src.size() < 128, "Too many point lights (max 128)!");
		
		TableHeader header = {};
		header.count_1 = (s32) src.size();
		dest.write(header);
		
		// Write out the grid.
		for(s32 x = 0; x < 0x40; x++) {
			std::array<u8, 16> mask = {};
			for(s32 light = 0; light < (s32) src.size(); light++) {
				const TransformComponent& transform = src[light].transform();
				f32 lower = transform.pos().x - src[light].radius;
				f32 upper = transform.pos().x + src[light].radius;
				if(lower < (x + 1) * 16.f && upper > x * 16.f) {
					mask[light >> 3] |= 1 << (light & 7);
				}
			}
			dest.write_multiple(mask);
		}
		for(s32 y = 0; y < 0x40; y++) {
			std::array<u8, 16> mask = {};
			for(s32 light = 0; light < (s32) src.size(); light++) {
				const TransformComponent& transform = src[light].transform();
				f32 lower = transform.pos().y - src[light].radius;
				f32 upper = transform.pos().y + src[light].radius;
				if(lower < (y + 1) * 16.f && upper > y * 16.f) {
					mask[light >> 3] |= 1 << (light & 7);
				}
			}
			dest.write_multiple(mask);
		}
		
		// Write out the lights.
		for(const PointLightInstance& inst : src) {
			GcUyaPointLightPacked packed = {};
			const TransformComponent& transform = inst.transform();
			packed.pos_x = (u16) roundf(transform.pos().x * 64.f);
			packed.pos_y = (u16) roundf(transform.pos().y * 64.f);
			packed.pos_z = (u16) roundf(transform.pos().z * 64.f);
			packed.radius = inst.radius * 64.f;
			packed.colour_r = (u16) roundf(inst.colour().r * 65535.f);
			packed.colour_g = (u16) roundf(inst.colour().g * 65535.f);
			packed.colour_b = (u16) roundf(inst.colour().b * 65535.f);
			dest.write(packed);
		}
	}
};

#endif
