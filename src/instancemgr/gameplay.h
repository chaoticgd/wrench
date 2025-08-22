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

#ifndef INSTANCEMGR_GAMEPLAY_H
#define INSTANCEMGR_GAMEPLAY_H

#include <functional>

#include <instancemgr/pvar.h>
#include <instancemgr/instances.h>

packed_struct(PvarTableEntry,
	s32 offset;
	s32 size;
)

packed_struct(PvarFixupEntry,
	/* 0x0 */ s32 pvar_index;
	/* 0x4 */ u32 offset;
)

packed_struct(SharedDataEntry,
	/* 0x0 */ u16 pvar_index;
	/* 0x2 */ u16 pointer_offset;
	/* 0x4 */ s32 shared_data_offset;
)

// Represents a packed gameplay file.
struct Gameplay
{
	Opt<LevelSettings> level_settings;
	Opt<std::vector<u8>> us_english_help_messages;
	Opt<std::vector<u8>> uk_english_help_messages;
	Opt<std::vector<u8>> french_help_messages;
	Opt<std::vector<u8>> german_help_messages;
	Opt<std::vector<u8>> spanish_help_messages;
	Opt<std::vector<u8>> italian_help_messages;
	Opt<std::vector<u8>> japanese_help_messages;
	Opt<std::vector<u8>> korean_help_messages;
	
	Opt<std::vector<MobyInstance>> moby_instances;
	Opt<s32> spawnable_moby_count;
	Opt<std::vector<s32>> moby_classes;
	Opt<std::vector<MobyGroupInstance>> moby_groups;
	Opt<std::vector<TieInstance>> tie_instances;
	Opt<std::vector<TieGroupInstance>> tie_groups;
	Opt<std::vector<ShrubInstance>> shrub_instances;
	Opt<std::vector<ShrubGroupInstance>> shrub_groups;
	
	Opt<std::vector<DirLightInstance>> dir_lights;
	Opt<std::vector<PointLightInstance>> point_lights;
	Opt<std::vector<EnvSamplePointInstance>> env_sample_points;
	Opt<std::vector<EnvTransitionInstance>> env_transitions;
	
	Opt<std::vector<CuboidInstance>> cuboids;
	Opt<std::vector<SphereInstance>> spheres;
	Opt<std::vector<CylinderInstance>> cylinders;
	Opt<std::vector<PillInstance>> pills;
	
	Opt<std::vector<CameraInstance>> cameras;
	Opt<std::vector<SoundInstance>> sound_instances;
	Opt<std::vector<PathInstance>> paths;
	Opt<std::vector<GrindPathInstance>> grind_paths;
	Opt<std::vector<AreaInstance>> areas;
	
	Opt<std::vector<u8>> occlusion;
	
	Opt<std::vector<PvarTableEntry>> pvar_table;
	Opt<std::vector<u8>> pvar_data;
	Opt<std::vector<PvarFixupEntry>> pvar_moby_links;
	Opt<std::vector<PvarFixupEntry>> pvar_relative_pointers;
	Opt<std::vector<u8>> shared_data;
	Opt<std::vector<SharedDataEntry>> shared_data_table;
	
	s32 core_moby_count = 0; // Used while unpacking missions to offset the generated moby IDs, and while packing to offset indices.
};

// *****************************************************************************

using GameplayBlockReadFunc = std::function<void(Gameplay& gameplay, Buffer src, Game game)>;
using GameplayBlockWriteFunc = std::function<bool(OutBuffer dest, const Gameplay& gameplay, Game game)>;

struct GameplayBlockFuncs
{
	GameplayBlockReadFunc read;
	GameplayBlockWriteFunc write;
};

struct GameplayBlockDescription
{
	s32 header_pointer_offset;
	GameplayBlockFuncs funcs;
	const char* name;
};

extern const std::vector<GameplayBlockDescription> RAC_GAMEPLAY_BLOCKS;
extern const std::vector<GameplayBlockDescription> GC_UYA_GAMEPLAY_BLOCKS;
extern const std::vector<GameplayBlockDescription> DL_GAMEPLAY_CORE_BLOCKS;
extern const std::vector<GameplayBlockDescription> DL_ART_INSTANCE_BLOCKS;
extern const std::vector<GameplayBlockDescription> DL_GAMEPLAY_MISSION_INSTANCE_BLOCKS;

void read_gameplay(
	Gameplay& gameplay, Buffer src, Game game, const std::vector<GameplayBlockDescription>& blocks);
std::vector<u8> write_gameplay(
	const Gameplay& gameplay_arg, Game game, const std::vector<GameplayBlockDescription>& blocks);
const std::vector<GameplayBlockDescription>* gameplay_block_descriptions_from_game(Game game);
std::vector<u8> write_occlusion_mappings(const Gameplay& gameplay, Game game);

#endif
