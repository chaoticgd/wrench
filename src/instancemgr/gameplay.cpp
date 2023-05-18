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

#include "gameplay.h"

#include <engine/basic_types.h>

const s32 NONE = -1;

static void rewire_pvar_indices(Gameplay& gameplay);
static void rewire_global_pvar_pointers(const PvarTypes& types, Gameplay& gameplay);

void read_gameplay(Gameplay& gameplay, PvarTypes& types, Buffer src, Game game, const std::vector<GameplayBlockDescription>& blocks) {
	for(const GameplayBlockDescription& block : blocks) {
		s32 block_offset = src.read<s32>(block.header_pointer_offset, "gameplay header");
		if(block_offset != 0 && block.funcs.read != nullptr) {
			block.funcs.read(types, gameplay, src.subbuf(block_offset), game);
		}
	}
}

std::vector<u8> write_gameplay(const Gameplay& gameplay_arg, const PvarTypes& types, Game game, const std::vector<GameplayBlockDescription>& blocks) {
	Gameplay gameplay = gameplay_arg;
	
	s32 header_size = 0;
	s32 block_count = 0;
	for(const GameplayBlockDescription& block : blocks) {
		header_size = std::max(header_size, block.header_pointer_offset + 4);
		if(block.header_pointer_offset != NONE) {
			block_count++;
		}
	}
	verify_fatal(header_size == block_count * 4);
	
	rewire_pvar_indices(gameplay); // Set pvar_index fields.
	rewire_global_pvar_pointers(types, gameplay); // Extract global pvar pointers from pvars.
	
	std::vector<u8> dest_vec(header_size, 0);
	OutBuffer dest(dest_vec);
	for(const GameplayBlockDescription& block : blocks) {
		if(block.header_pointer_offset != NONE && block.funcs.write != nullptr) {
			if(strcmp(block.name, "us english help messages") != 0 && strcmp(block.name, "occlusion") != 0) {
				dest.pad(0x10, 0);
			}
			if(strcmp(block.name, "occlusion") == 0 && gameplay.occlusion.has_value()) {
				dest.pad(0x40, 0);
			}
			s32 ofs = (s32) dest_vec.size();
			if(block.funcs.write(dest, types, gameplay, game)) {
				verify_fatal(block.header_pointer_offset + 4 <= (s32) dest_vec.size());
				*(s32*) &dest_vec[block.header_pointer_offset] = ofs;
			}
		}
	}
	return dest_vec;
}

const std::vector<GameplayBlockDescription>* gameplay_block_descriptions_from_game(Game game) {
	const std::vector<GameplayBlockDescription>* gbd = nullptr;
	switch(game) {
		case Game::RAC: gbd = &RAC_GAMEPLAY_BLOCKS; break;
		case Game::GC: gbd = &GC_UYA_GAMEPLAY_BLOCKS; break;
		case Game::UYA: gbd = &GC_UYA_GAMEPLAY_BLOCKS; break;
		case Game::DL: gbd = &DL_GAMEPLAY_CORE_BLOCKS; break;
		default: verify_not_reached("Invalid game!"); break;
	}
	return gbd;
}

// Pull in the implementations for all the gameplay blocks.
#include "gameplay_impl_common.h"
#include "gameplay_impl_classes.h"
#include "gameplay_impl_env.h"
#include "gameplay_impl_misc.h"

template <typename Block, typename Field>
static GameplayBlockFuncs bf(Field field) {
	// e.g. if field = &Gameplay::moby_instances then FieldType = std::vector<MobyInstance>.
	using FieldType = typename std::remove_reference<decltype(Gameplay().*field)>::type::value_type;
	
	GameplayBlockFuncs funcs;
	funcs.read = [field](PvarTypes&, Gameplay& gameplay, Buffer src, Game game) {
		FieldType value;
		Block::read(value, src, game);
		gameplay.*field = std::move(value);
	};
	funcs.write = [field](OutBuffer dest, const PvarTypes&, const Gameplay& gameplay, Game game) {
		if(!(gameplay.*field).has_value()) {
			return false;
		}
		Block::write(dest, *(gameplay.*field), game);
		return true;
	};
	return funcs;
}

const std::vector<GameplayBlockDescription> RAC_GAMEPLAY_BLOCKS = {
	{0x88, bf<RacEnvSamplePointBlock>(&Gameplay::env_sample_points), "env sample points"},
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
	{0x80, bf<EnvTransitionBlock>(&Gameplay::env_transitions), "env transitions"},
	{0x08, bf<InstanceBlock<Camera, CameraPacked>>(&Gameplay::cameras), "cameras"},
	{0x0c, bf<InstanceBlock<SoundInstance, SoundInstancePacked>>(&Gameplay::sound_instances), "sound instances"},
	{0x40, bf<ClassBlock>(&Gameplay::moby_classes), "moby classes"},
	{0x44, {RAC1MobyBlock::read, RAC1MobyBlock::write}, "moby instances"},
	{0x54, {PvarTableBlock::read, PvarTableBlock::write}, "pvar table"},
	{0x58, {PvarDataBlock::read, PvarDataBlock::write}, "pvar data"},
	{0x50, {PvarScratchpadBlock::read, PvarScratchpadBlock::write}, "pvar pointer scratchpad table"},
	{0x5c, {PvarPointerRewireBlock::read, PvarPointerRewireBlock::write}, "pvar pointer rewire table"},
	{0x48, bf<GroupBlock>(&Gameplay::moby_groups), "moby groups"},
	{0x4c, {GlobalPvarBlock::read, GlobalPvarBlock::write}, "global pvar"},
	{0x30, {TieClassBlock::read, TieClassBlock::write}, "tie classes"},
	{0x34, bf<InstanceBlock<TieInstance, RacTieInstance>>(&Gameplay::tie_instances), "tie instances"},
	{0x38, {ShrubClassBlock::read, ShrubClassBlock::write}, "shrub classes"},
	{0x3c, bf<InstanceBlock<ShrubInstance, ShrubInstancePacked>>(&Gameplay::shrub_instances), "shrub instances"},
	{0x70, bf<PathBlock>(&Gameplay::paths), "paths"},
	{0x60, bf<InstanceBlock<Cuboid, ShapePacked>>(&Gameplay::cuboids), "cuboids"},
	{0x64, bf<InstanceBlock<Sphere, ShapePacked>>(&Gameplay::spheres), "spheres"},
	{0x68, bf<InstanceBlock<Cylinder, ShapePacked>>(&Gameplay::cylinders), "cylinders"},
	{0x6c, bf<InstanceBlock<Pill, ShapePacked>>(&Gameplay::pills), "pills"},
	{0x84, {CamCollGridBlock::read, CamCollGridBlock::write}, "cam coll grid"},
	{0x7c, bf<InstanceBlock<PointLight, PointLightPacked>>(&Gameplay::point_lights), "point lights"},
	{0x78, {PointLightGridBlock::read, PointLightGridBlock::write}, "point light grid"},
	{0x74, bf<GrindPathBlock>(&Gameplay::grind_paths), "grindpaths"},
	{0x8c, bf<OcclusionMappingsBlock>(&Gameplay::occlusion), "occlusion"},
	{0x90, {nullptr, nullptr}, "pad"}
};

const std::vector<GameplayBlockDescription> GC_UYA_GAMEPLAY_BLOCKS = {
	{0x8c, bf<GcUyaDlEnvSamplePointBlock>(&Gameplay::env_sample_points), "env sample points"},
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
	{0x84, bf<EnvTransitionBlock>(&Gameplay::env_transitions), "env transitions"},
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
	{0x34, bf<InstanceBlock<TieInstance, GcUyaDlTieInstance>>(&Gameplay::tie_instances), "tie instances"},
	{0x94, {TieAmbientRgbaBlock::read, TieAmbientRgbaBlock::write}, "tie ambient rgbas"},
	{0x38, bf<GroupBlock>(&Gameplay::tie_groups), "tie groups"},
	{0x3c, {ShrubClassBlock::read, ShrubClassBlock::write}, "shrub classes"},
	{0x40, bf<InstanceBlock<ShrubInstance, ShrubInstancePacked>>(&Gameplay::shrub_instances), "shrub instances"},
	{0x44, bf<GroupBlock>(&Gameplay::shrub_groups), "shrub groups"},
	{0x78, bf<PathBlock>(&Gameplay::paths), "paths"},
	{0x68, bf<InstanceBlock<Cuboid, ShapePacked>>(&Gameplay::cuboids), "cuboids"},
	{0x6c, bf<InstanceBlock<Sphere, ShapePacked>>(&Gameplay::spheres), "spheres"},
	{0x70, bf<InstanceBlock<Cylinder, ShapePacked>>(&Gameplay::cylinders), "cylinders"},
	{0x74, bf<InstanceBlock<Pill, ShapePacked>>(&Gameplay::pills), "pills"},
	{0x88, {CamCollGridBlock::read, CamCollGridBlock::write}, "cam coll grid"},
	{0x80, bf<GcUyaPointLightsBlock>(&Gameplay::point_lights), "point lights"},
	{0x7c, bf<GrindPathBlock>(&Gameplay::grind_paths), "grindpaths"},
	{0x98, bf<AreasBlock>(&Gameplay::areas), "areas"},
	{0x90, bf<OcclusionMappingsBlock>(&Gameplay::occlusion), "occlusion"}
};

const std::vector<GameplayBlockDescription> DL_GAMEPLAY_CORE_BLOCKS = {
	{0x70, bf<GcUyaDlEnvSamplePointBlock>(&Gameplay::env_sample_points), "env sample points"},
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
	{0x58, bf<InstanceBlock<Pill, ShapePacked>>(&Gameplay::pills), "pills"},
	{0x6c, {CamCollGridBlock::read, CamCollGridBlock::write}, "cam coll grid"},
	{0x64, bf<GcUyaPointLightsBlock>(&Gameplay::point_lights), "point lights"},
	{0x60, bf<GrindPathBlock>(&Gameplay::grind_paths), "grindpaths"},
	{0x74, bf<AreasBlock>(&Gameplay::areas), "areas"},
	{0x68, {nullptr, nullptr}, "pad"}
};

const std::vector<GameplayBlockDescription> DL_ART_INSTANCE_BLOCKS = {
	{0x00, bf<InstanceBlock<DirectionalLight, DirectionalLightPacked>>(&Gameplay::lights), "directional lights"},
	{0x04, {TieClassBlock::read, TieClassBlock::write}, "tie classes"},
	{0x08, bf<InstanceBlock<TieInstance, GcUyaDlTieInstance>>(&Gameplay::tie_instances), "tie instances"},
	{0x20, {TieAmbientRgbaBlock::read, TieAmbientRgbaBlock::write}, "tie ambient rgbas"},
	{0x0c, bf<GroupBlock>(&Gameplay::tie_groups), "tie groups"},
	{0x10, {ShrubClassBlock::read, ShrubClassBlock::write}, "shrub classes"},
	{0x14, bf<InstanceBlock<ShrubInstance, ShrubInstancePacked>>(&Gameplay::shrub_instances), "shrub instances"},
	{0x18, bf<GroupBlock>(&Gameplay::shrub_groups), "art instance shrub groups"},
	{0x1c, bf<OcclusionMappingsBlock>(&Gameplay::occlusion), "occlusion"},
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

void Gameplay::clear_selection() {
	for_each_instance([&](Instance& inst) {
		inst.selected = false;
	});
}

std::vector<InstanceId> Gameplay::selected_instances() const {
	std::vector<InstanceId> ids;
	for_each_instance([&](const Instance& inst) {
		if(inst.selected) {
			ids.push_back(inst.id());
		}
	});
	return ids;
}
