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

#ifndef INSTANCEMGR_LEVEL_SETTINGS_H
#define INSTANCEMGR_LEVEL_SETTINGS_H

#include <map>

#include <core/util.h>
#include <core/collada.h>
#include <core/buffer.h>
#include <core/texture.h>
#include <core/filesystem.h>
#include <core/build_config.h>
#include <instancemgr/instance.h>

// A plane that defines the bounds of a chunk. Everything on the side of the
// plane in the direction that the normal is pointing is inside the chunk.
struct ChunkPlane
{
	glm::vec3 point;
	glm::vec3 normal;
};

struct LevelSettingsThirdPart
{
	s32 unknown_0;
	s32 unknown_4;
	s32 unknown_8;
	s32 unknown_c;
};

struct LevelSettingsRewardStats
{
	f32 xp_decay_rate;
	f32 xp_decay_min;
	f32 bolt_decay_rate;
	f32 bolt_decay_min;
	s32 unknown_10;
	s32 unknown_14;
};

struct LevelSettingsFifthPart
{
	s32 unknown_0;
	s32 moby_inst_count;
	s32 unknown_8;
	s32 unknown_c;
	s32 unknown_10;
	s32 dbg_hit_points;
};

struct LevelSettings
{
	Opt<glm::vec3> background_colour;
	Opt<glm::vec3> fog_colour;
	f32 fog_near_dist = 0.f;
	f32 fog_far_dist = 0.f;
	f32 fog_near_intensity = 0.f;
	f32 fog_far_intensity = 0.f;
	f32 death_height = 0.f;
	bool is_spherical_world = false;
	glm::vec3 sphere_pos = {0.f, 0.f, 0.f};
	glm::vec3 ship_pos = {0.f, 0.f, 0.f};
	f32 ship_rot_z = 0.f;
	pathlink ship_path = {-1};
	cuboidlink ship_camera_cuboid_start = {-1};
	cuboidlink ship_camera_cuboid_end = {-1};
	// Planes specifying the volumes of the level chunks. The first element
	// represents the second chunk, and the second element represents the third
	// chunk. If both tests fail, you can assume it's the first chunk (chunk 0).
	std::vector<ChunkPlane> chunk_planes;
	Opt<s32> core_sounds_count;
	Opt<s32> rac3_third_part;
	Opt<std::vector<LevelSettingsThirdPart>> third_part;
	Opt<LevelSettingsRewardStats> reward_stats;
	Opt<LevelSettingsFifthPart> fifth_part;
	Opt<std::vector<u8>> dbg_attack_damage;
};

struct WtfNode;
struct WtfWriter;

LevelSettings read_level_settings(const WtfNode* node);
void rewrite_level_settings_links(LevelSettings& settings, const Instances& instances);
void write_level_settings(WtfWriter* ctx, const LevelSettings& settings);
s32 chunk_index_from_position(const glm::vec3& point, const LevelSettings& level_settings);

#endif
