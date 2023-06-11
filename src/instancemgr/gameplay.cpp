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
#include <instancemgr/pvar.h>

const s32 NONE = -1;

void read_gameplay(Gameplay& gameplay, Buffer src, Game game, const std::vector<GameplayBlockDescription>& blocks) {
	for(const GameplayBlockDescription& block : blocks) {
		s32 block_offset = src.read<s32>(block.header_pointer_offset, "gameplay header");
		if(block_offset != 0 && block.funcs.read != nullptr) {
			block.funcs.read(gameplay, src.subbuf(block_offset), game);
		}
	}
}

std::vector<u8> write_gameplay(const Gameplay& gameplay_arg, Game game, const std::vector<GameplayBlockDescription>& blocks) {
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
			if(block.funcs.write(dest, gameplay, game)) {
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
#include "gameplay_impl_common.inl"
#include "gameplay_impl_classes.inl"
#include "gameplay_impl_env.inl"
#include "gameplay_impl_misc.inl"

template <typename Block, typename Field>
static GameplayBlockFuncs bf(Field field) {
	// e.g. if field = &Gameplay::moby_instances then FieldType = std::vector<MobyInstance>.
	using FieldType = typename std::remove_reference<decltype(Gameplay().*field)>::type::value_type;
	
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

const std::vector<GameplayBlockDescription> RAC_GAMEPLAY_BLOCKS = {
	{0x88, bf<RacEnvSamplePointBlock>(&Gameplay::env_sample_points), "env sample points"},
	{0x00, bf<LevelSettingsBlock>(&Gameplay::level_settings), "level settings"},
	{0x10, bf<BinHelpMessageBlock<false>>(&Gameplay::us_english_help_messages), "us english help messages"},
	{0x14, bf<BinHelpMessageBlock<false>>(&Gameplay::uk_english_help_messages), "uk english help messages"},
	{0x18, bf<BinHelpMessageBlock<false>>(&Gameplay::french_help_messages), "french help messages"},
	{0x1c, bf<BinHelpMessageBlock<false>>(&Gameplay::german_help_messages), "german help messages"},
	{0x20, bf<BinHelpMessageBlock<false>>(&Gameplay::spanish_help_messages), "spanish help messages"},
	{0x24, bf<BinHelpMessageBlock<false>>(&Gameplay::italian_help_messages), "italian help messages"},
	{0x28, bf<BinHelpMessageBlock<false>>(&Gameplay::japanese_help_messages), "japanese help messages"},
	{0x2c, bf<BinHelpMessageBlock<true>>(&Gameplay::korean_help_messages), "korean help messages"},
	{0x04, bf<InstanceBlock<DirLightInstance, DirectionalLightPacked>>(&Gameplay::dir_lights), "directional lights"},
	{0x80, bf<EnvTransitionBlock>(&Gameplay::env_transitions), "env transitions"},
	{0x08, bf<InstanceBlock<CameraInstance, CameraPacked>>(&Gameplay::cameras), "cameras"},
	{0x0c, bf<InstanceBlock<SoundInstance, SoundInstancePacked>>(&Gameplay::sound_instances), "sound instances"},
	{0x40, bf<ClassBlock>(&Gameplay::moby_classes), "moby classes"},
	{0x44, {RacMobyBlock::read, RacMobyBlock::write}, "moby instances"},
	{0x54, {PvarTableBlock::read, PvarTableBlock::write}, "pvar table"},
	{0x58, {PvarDataBlock::read, PvarDataBlock::write}, "pvar data"},
	{0x50, bf<PvarFixupBlock>(&Gameplay::pvar_moby_links), "moby link fixup table"},
	{0x5c, bf<PvarFixupBlock>(&Gameplay::pvar_sub_vars), "sub var fixup table"},
	{0x48, bf<GroupBlock<MobyGroupInstance>>(&Gameplay::moby_groups), "moby groups"},
	{0x4c, {GlobalPvarBlock::read, GlobalPvarBlock::write}, "global pvar"},
	{0x30, {TieClassBlock::read, TieClassBlock::write}, "tie classes"},
	{0x34, bf<InstanceBlock<TieInstance, RacTieInstance>>(&Gameplay::tie_instances), "tie instances"},
	{0x38, {ShrubClassBlock::read, ShrubClassBlock::write}, "shrub classes"},
	{0x3c, bf<InstanceBlock<ShrubInstance, ShrubInstancePacked>>(&Gameplay::shrub_instances), "shrub instances"},
	{0x70, bf<PathBlock>(&Gameplay::paths), "paths"},
	{0x60, bf<InstanceBlock<CuboidInstance, ShapePacked>>(&Gameplay::cuboids), "cuboids"},
	{0x64, bf<InstanceBlock<SphereInstance, ShapePacked>>(&Gameplay::spheres), "spheres"},
	{0x68, bf<InstanceBlock<CylinderInstance, ShapePacked>>(&Gameplay::cylinders), "cylinders"},
	{0x6c, bf<InstanceBlock<PillInstance, ShapePacked>>(&Gameplay::pills), "pills"},
	{0x84, {CamCollGridBlock::read, CamCollGridBlock::write}, "cam coll grid"},
	{0x7c, bf<InstanceBlock<PointLightInstance, PointLightPacked>>(&Gameplay::point_lights), "point lights"},
	{0x78, {PointLightGridBlock::read, PointLightGridBlock::write}, "point light grid"},
	{0x74, bf<GrindPathBlock>(&Gameplay::grind_paths), "grindpaths"},
	{0x8c, bf<OcclusionMappingsBlock>(&Gameplay::occlusion), "occlusion"},
	{0x90, {nullptr, nullptr}, "pad"}
};

const std::vector<GameplayBlockDescription> GC_UYA_GAMEPLAY_BLOCKS = {
	{0x8c, bf<GcUyaDlEnvSamplePointBlock>(&Gameplay::env_sample_points), "env sample points"},
	{0x00, bf<LevelSettingsBlock>(&Gameplay::level_settings), "level settings"},
	{0x10, bf<BinHelpMessageBlock<false>>(&Gameplay::us_english_help_messages), "us english help messages"},
	{0x14, bf<BinHelpMessageBlock<false>>(&Gameplay::uk_english_help_messages), "uk english help messages"},
	{0x18, bf<BinHelpMessageBlock<false>>(&Gameplay::french_help_messages), "french help messages"},
	{0x1c, bf<BinHelpMessageBlock<false>>(&Gameplay::german_help_messages), "german help messages"},
	{0x20, bf<BinHelpMessageBlock<false>>(&Gameplay::spanish_help_messages), "spanish help messages"},
	{0x24, bf<BinHelpMessageBlock<false>>(&Gameplay::italian_help_messages), "italian help messages"},
	{0x28, bf<BinHelpMessageBlock<false>>(&Gameplay::japanese_help_messages), "japanese help messages"},
	{0x2c, bf<BinHelpMessageBlock<true>>(&Gameplay::korean_help_messages), "korean help messages"},
	{0x04, bf<InstanceBlock<DirLightInstance, DirectionalLightPacked>>(&Gameplay::dir_lights), "directional lights"},
	{0x84, bf<EnvTransitionBlock>(&Gameplay::env_transitions), "env transitions"},
	{0x08, bf<InstanceBlock<CameraInstance, CameraPacked>>(&Gameplay::cameras), "cameras"},
	{0x0c, bf<InstanceBlock<SoundInstance, SoundInstancePacked>>(&Gameplay::sound_instances), "sound instances"},
	{0x48, bf<ClassBlock>(&Gameplay::moby_classes), "moby classes"},
	{0x4c, {GcUyaMobyBlock::read, GcUyaMobyBlock::write}, "moby instances"},
	{0x5c, {PvarTableBlock::read, PvarTableBlock::write}, "pvar table"},
	{0x60, {PvarDataBlock::read, PvarDataBlock::write}, "pvar data"},
	{0x58, bf<PvarFixupBlock>(&Gameplay::pvar_moby_links), "moby link fixup table"},
	{0x64, bf<PvarFixupBlock>(&Gameplay::pvar_sub_vars), "sub var fixup table"},
	{0x50, bf<GroupBlock<MobyGroupInstance>>(&Gameplay::moby_groups), "moby groups"},
	{0x54, {GlobalPvarBlock::read, GlobalPvarBlock::write}, "global pvar"},
	{0x30, {TieClassBlock::read, TieClassBlock::write}, "tie classes"},
	{0x34, bf<InstanceBlock<TieInstance, GcUyaDlTieInstance>>(&Gameplay::tie_instances), "tie instances"},
	{0x94, {TieAmbientRgbaBlock::read, TieAmbientRgbaBlock::write}, "tie ambient rgbas"},
	{0x38, bf<GroupBlock<TieGroupInstance>>(&Gameplay::tie_groups), "tie groups"},
	{0x3c, {ShrubClassBlock::read, ShrubClassBlock::write}, "shrub classes"},
	{0x40, bf<InstanceBlock<ShrubInstance, ShrubInstancePacked>>(&Gameplay::shrub_instances), "shrub instances"},
	{0x44, bf<GroupBlock<ShrubGroupInstance>>(&Gameplay::shrub_groups), "shrub groups"},
	{0x78, bf<PathBlock>(&Gameplay::paths), "paths"},
	{0x68, bf<InstanceBlock<CuboidInstance, ShapePacked>>(&Gameplay::cuboids), "cuboids"},
	{0x6c, bf<InstanceBlock<SphereInstance, ShapePacked>>(&Gameplay::spheres), "spheres"},
	{0x70, bf<InstanceBlock<CylinderInstance, ShapePacked>>(&Gameplay::cylinders), "cylinders"},
	{0x74, bf<InstanceBlock<PillInstance, ShapePacked>>(&Gameplay::pills), "pills"},
	{0x88, {CamCollGridBlock::read, CamCollGridBlock::write}, "cam coll grid"},
	{0x80, bf<GcUyaPointLightsBlock>(&Gameplay::point_lights), "point lights"},
	{0x7c, bf<GrindPathBlock>(&Gameplay::grind_paths), "grindpaths"},
	{0x98, bf<AreasBlock>(&Gameplay::areas), "areas"},
	{0x90, bf<OcclusionMappingsBlock>(&Gameplay::occlusion), "occlusion"}
};

const std::vector<GameplayBlockDescription> DL_GAMEPLAY_CORE_BLOCKS = {
	{0x70, bf<GcUyaDlEnvSamplePointBlock>(&Gameplay::env_sample_points), "env sample points"},
	{0x00, bf<LevelSettingsBlock>(&Gameplay::level_settings), "level settings"},
	{0x0c, bf<BinHelpMessageBlock<false>>(&Gameplay::us_english_help_messages), "us english help messages"},
	{0x10, bf<BinHelpMessageBlock<false>>(&Gameplay::uk_english_help_messages), "uk english help messages"},
	{0x14, bf<BinHelpMessageBlock<false>>(&Gameplay::french_help_messages), "french help messages"},
	{0x18, bf<BinHelpMessageBlock<false>>(&Gameplay::german_help_messages), "german help messages"},
	{0x1c, bf<BinHelpMessageBlock<false>>(&Gameplay::spanish_help_messages), "spanish help messages"},
	{0x20, bf<BinHelpMessageBlock<false>>(&Gameplay::italian_help_messages), "italian help messages"},
	{0x24, bf<BinHelpMessageBlock<false>>(&Gameplay::japanese_help_messages), "japanese help messages"},
	{0x28, bf<BinHelpMessageBlock<true>>(&Gameplay::korean_help_messages), "korean help messages"},
	{0x04, bf<InstanceBlock<CameraInstance, CameraPacked>>(&Gameplay::cameras), "import cameras"},
	{0x08, bf<InstanceBlock<SoundInstance, SoundInstancePacked>>(&Gameplay::sound_instances), "sound instances"},
	{0x2c, bf<ClassBlock>(&Gameplay::moby_classes), "moby classes"},
	{0x30, {DlMobyBlock::read, DlMobyBlock::write}, "moby instances"},
	{0x40, {PvarTableBlock::read, PvarTableBlock::write}, "pvar table"},
	{0x44, {PvarDataBlock::read, PvarDataBlock::write}, "pvar data"},
	{0x3c, bf<PvarFixupBlock>(&Gameplay::pvar_moby_links), "moby link fixup table"},
	{0x48, bf<PvarFixupBlock>(&Gameplay::pvar_sub_vars), "sub var fixup table"},
	{0x34, bf<GroupBlock<MobyGroupInstance>>(&Gameplay::moby_groups), "moby groups"},
	{0x38, {GlobalPvarBlock::read, GlobalPvarBlock::write}, "global pvar"},
	{0x5c, bf<PathBlock>(&Gameplay::paths), "paths"},
	{0x4c, bf<InstanceBlock<CuboidInstance, ShapePacked>>(&Gameplay::cuboids), "cuboids"},
	{0x50, bf<InstanceBlock<SphereInstance, ShapePacked>>(&Gameplay::spheres), "spheres"},
	{0x54, bf<InstanceBlock<CylinderInstance, ShapePacked>>(&Gameplay::cylinders), "cylinders"},
	{0x58, bf<InstanceBlock<PillInstance, ShapePacked>>(&Gameplay::pills), "pills"},
	{0x6c, {CamCollGridBlock::read, CamCollGridBlock::write}, "cam coll grid"},
	{0x64, bf<GcUyaPointLightsBlock>(&Gameplay::point_lights), "point lights"},
	{0x60, bf<GrindPathBlock>(&Gameplay::grind_paths), "grindpaths"},
	{0x74, bf<AreasBlock>(&Gameplay::areas), "areas"},
	{0x68, {nullptr, nullptr}, "pad"}
};

const std::vector<GameplayBlockDescription> DL_ART_INSTANCE_BLOCKS = {
	{0x00, bf<InstanceBlock<DirLightInstance, DirectionalLightPacked>>(&Gameplay::dir_lights), "directional lights"},
	{0x04, {TieClassBlock::read, TieClassBlock::write}, "tie classes"},
	{0x08, bf<InstanceBlock<TieInstance, GcUyaDlTieInstance>>(&Gameplay::tie_instances), "tie instances"},
	{0x20, {TieAmbientRgbaBlock::read, TieAmbientRgbaBlock::write}, "tie ambient rgbas"},
	{0x0c, bf<GroupBlock<TieGroupInstance>>(&Gameplay::tie_groups), "tie groups"},
	{0x10, {ShrubClassBlock::read, ShrubClassBlock::write}, "shrub classes"},
	{0x14, bf<InstanceBlock<ShrubInstance, ShrubInstancePacked>>(&Gameplay::shrub_instances), "shrub instances"},
	{0x18, bf<GroupBlock<ShrubGroupInstance>>(&Gameplay::shrub_groups), "art instance shrub groups"},
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
	{0x04, {DlMobyBlock::read, DlMobyBlock::write}, "moby instances"},
	{0x14, {PvarTableBlock::read, PvarTableBlock::write}, "pvar table"},
	{0x18, {PvarDataBlock::read, PvarDataBlock::write}, "pvar data"},
	{0x10, bf<PvarFixupBlock>(&Gameplay::pvar_moby_links), "moby link fixup table"},
	{0x1c, bf<PvarFixupBlock>(&Gameplay::pvar_sub_vars), "sub var fixup table"},
	{0x08, bf<GroupBlock<MobyGroupInstance>>(&Gameplay::moby_groups), "moby groups"},
	{0x0c, {GlobalPvarBlock::read, GlobalPvarBlock::write}, "global pvar"},
};

void move_gameplay_to_instances(Instances& dest, HelpMessages* help_dest, OcclusionMappings* occl_dest, std::vector<CppType>& types_dest, Gameplay& src, Game game) {
	if(src.level_settings.has_value()) {
		dest.level_settings = std::move(*src.level_settings);
	}
	
	dest.moby_instances = std::move(opt_iterator(src.moby_instances));
	dest.spawnable_moby_count = opt_or_zero(src.spawnable_moby_count);
	dest.moby_classes = std::move(opt_iterator(src.moby_classes));
	dest.moby_groups = std::move(opt_iterator(src.moby_groups));
	dest.tie_instances = std::move(opt_iterator(src.tie_instances));
	dest.tie_groups = std::move(opt_iterator(src.tie_groups));
	dest.shrub_instances = std::move(opt_iterator(src.shrub_instances));
	dest.shrub_groups = std::move(opt_iterator(src.shrub_groups));
	dest.global_pvar = std::move(opt_iterator(src.global_pvar));
	
	dest.dir_lights = std::move(opt_iterator(src.dir_lights));
	dest.point_lights = std::move(opt_iterator(src.point_lights));
	dest.env_sample_points = std::move(opt_iterator(src.env_sample_points));
	dest.env_transitions = std::move(opt_iterator(src.env_transitions));
	
	dest.cuboids = std::move(opt_iterator(src.cuboids));
	dest.spheres = std::move(opt_iterator(src.spheres));
	dest.cylinders = std::move(opt_iterator(src.cylinders));
	dest.pills = std::move(opt_iterator(src.pills));
	
	dest.cameras = std::move(opt_iterator(src.cameras));
	dest.sound_instances = std::move(opt_iterator(src.sound_instances));
	dest.paths = std::move(opt_iterator(src.paths));
	dest.grind_paths = std::move(opt_iterator(src.grind_paths));
	dest.areas = std::move(opt_iterator(src.areas));
	
	if(help_dest) {
		help_dest->us_english = std::move(opt_iterator(src.us_english_help_messages));
		help_dest->uk_english = std::move(opt_iterator(src.uk_english_help_messages));
		help_dest->french = std::move(opt_iterator(src.french_help_messages));
		help_dest->german = std::move(opt_iterator(src.german_help_messages));
		help_dest->spanish = std::move(opt_iterator(src.spanish_help_messages));
		help_dest->italian = std::move(opt_iterator(src.italian_help_messages));
		help_dest->japanese = std::move(opt_iterator(src.japanese_help_messages));
		help_dest->korean = std::move(opt_iterator(src.korean_help_messages));
	}
	
	if(occl_dest && src.occlusion.has_value()) {
		*occl_dest = *src.occlusion;
	}
	
	recover_pvars(dest, types_dest, src, game);
}

void move_instances_to_gameplay(Gameplay& dest, Instances& src, HelpMessages* help_src, OcclusionMappings* occlusion_src, const std::map<std::string, CppType>& types_src) {
	build_pvars(dest, src, types_src);
	
	dest.level_settings = std::move(src.level_settings);
	
	if(help_src) {
		dest.us_english_help_messages = std::move(help_src->us_english);
		dest.uk_english_help_messages = std::move(help_src->uk_english);
		dest.french_help_messages = std::move(help_src->french);
		dest.german_help_messages = std::move(help_src->german);
		dest.spanish_help_messages = std::move(help_src->spanish);
		dest.italian_help_messages = std::move(help_src->italian);
		dest.japanese_help_messages = std::move(help_src->japanese);
		dest.korean_help_messages = std::move(help_src->korean);
	}
	
	dest.moby_instances = src.moby_instances.release();
	dest.spawnable_moby_count = src.spawnable_moby_count;
	dest.moby_classes = std::move(src.moby_classes);
	dest.moby_groups = src.moby_groups.release();
	dest.tie_instances = src.tie_instances.release();
	dest.tie_groups = src.tie_groups.release();
	dest.shrub_instances = (src.shrub_instances).release();
	dest.shrub_groups = src.shrub_groups.release();
	dest.global_pvar = std::move(src.global_pvar);
	
	dest.dir_lights = src.dir_lights.release();
	dest.point_lights = src.point_lights.release();
	dest.env_sample_points = src.env_sample_points.release();
	dest.env_transitions = src.env_transitions.release();
	
	dest.cuboids = src.cuboids.release();
	dest.spheres = src.spheres.release();
	dest.cylinders = src.cylinders.release();
	dest.pills = src.pills.release();
	
	dest.cameras = src.cameras.release();
	dest.sound_instances = src.sound_instances.release();
	dest.paths = src.paths.release();
	dest.grind_paths = src.grind_paths.release();
	dest.areas = src.areas.release();
	
	if(occlusion_src && (!occlusion_src->tfrag_mappings.empty()
			|| !occlusion_src->tie_mappings.empty()
			|| !occlusion_src->moby_mappings.empty())) {
		dest.occlusion = *occlusion_src;
	}
}
