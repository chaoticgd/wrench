/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2022 chaoticgd

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

#include "wad_identifier.h"

struct WadFileDescription
{
	const char* name;
	Game game;
	WadType type;
	s32 header_size;
	s32 secondary_offset = -1; // Secondary offset to check if multiple types of files have the same header size.
	s32 secondary_value_min;
	s32 secondary_value_max;
	s32 tertiary_offset = -1;
	s32 tertiary_value_not_equal;
};

static WadFileDescription WAD_FILE_TYPES[] = {
	{"global", Game::RAC    , WadType::GLOBAL     , 0x2960},
	{"level",  Game::RAC    , WadType::LEVEL      , 0x0030},
	{"audio",  Game::RAC    , WadType::LEVEL_AUDIO, 0x0164},
	{"scene",  Game::RAC    , WadType::LEVEL_SCENE, 0x22b8},
	{"mpeg"  , Game::GC     , WadType::MPEG       , 0x0328},
	{"misc"  , Game::GC     , WadType::MISC       , 0x0040},
	{"hud"   , Game::GC     , WadType::HUD        , 0x1870},
	{"bonus" , Game::GC     , WadType::BONUS      , 0x0a48},
	{"audio" , Game::GC     , WadType::AUDIO      , 0x1800},
	{"space" , Game::GC     , WadType::SPACE      , 0x0ba8},
	{"scene" , Game::GC     , WadType::SCENE      , 0x0170},
	{"gadget", Game::GC     , WadType::GADGET     , 0x03c8, 0x8, 0, 0x586}, // 0xb1
	{"gadget", Game::UYA    , WadType::GADGET     , 0x03c8, 0x8, 0x587, 0x1000}, // 0xa5d
	{"gadget", Game::UNKNOWN, WadType::GADGET     , 0x03c8},
	{"armor" , Game::GC     , WadType::ARMOR      , 0x00f8},
	{"level" , Game::UNKNOWN, WadType::LEVEL      , 0x0060},
	{"audio" , Game::GC     , WadType::LEVEL_AUDIO, 0x1018},
	{"scene" , Game::GC     , WadType::LEVEL_SCENE, 0x137c},
	{"mpeg"  , Game::UYA    , WadType::MPEG       , 0x0648, 0xc, 0, 0x3b}, // 0x38
	{"mpeg"  , Game::DL     , WadType::MPEG       , 0x0648, 0xc, 0x3c, 0x100}, // 0x40
	{"mpeg"  , Game::UNKNOWN, WadType::MPEG       , 0x0648},
	{"misc"  , Game::UYA    , WadType::MISC       , 0x0048},
	{"bonus" , Game::UYA    , WadType::BONUS      , 0x0bf0},
	{"space" , Game::UYA    , WadType::SPACE      , 0x0c30},
	{"armor" , Game::UYA    , WadType::ARMOR      , 0x0398},
	{"audio" , Game::UYA    , WadType::AUDIO      , 0x2340},
	{"hud"   , Game::UYA    , WadType::HUD        , 0x2ab0},
	{"audio" , Game::UYA    , WadType::LEVEL_AUDIO, 0x1818},
	{"scene" , Game::UNKNOWN, WadType::LEVEL_SCENE, 0x26f0},
	{"misc"  , Game::DL     , WadType::MISC       , 0x0050},
	{"bonus" , Game::DL     , WadType::BONUS      , 0x02a8},
	{"space" , Game::DL     , WadType::SPACE      , 0x0068, 0xc, 0, 0x75d, 0x14, 0x1}, // secondary: 0x252, 0x255
	{"online", Game::DL     , WadType::ONLINE     , 0x0068, 0xc, 0x75e, 0x1000, 0x14, 0x1}, // secondary: 0xc6a
	{"level" , Game::UNKNOWN, WadType::LEVEL      , 0x0068},
	{"armor" , Game::DL     , WadType::ARMOR      , 0x0228},
	{"audio" , Game::DL     , WadType::AUDIO      , 0xa870},
	{"hud"   , Game::DL     , WadType::HUD        , 0x0f88},
	{"level" , Game::DL     , WadType::LEVEL      , 0x0c68},
	{"audio" , Game::DL     , WadType::LEVEL_AUDIO, 0x02a0},
	{"audio" , Game::UNKNOWN, WadType::LEVEL_AUDIO, 0x1000},
	{"scene" , Game::UNKNOWN, WadType::LEVEL_SCENE, 0x2420}
};

std::tuple<Game, WadType, const char*> identify_wad(Buffer header)
{
	for (WadFileDescription& desc : WAD_FILE_TYPES) {
		if (desc.header_size != header.size()) {
			continue;
		}
		
		if (desc.secondary_offset > -1) {
			s32 value = header.read<s32>(desc.secondary_offset, "header");
			if (value < desc.secondary_value_min || value > desc.secondary_value_max) {
				continue;
			}
		}
		
		if (desc.tertiary_offset > -1) {
			s32 value = header.read<s32>(desc.tertiary_offset, "header");
			if (value == desc.tertiary_value_not_equal) {
				continue;
			}
		}
		
		return {desc.game, desc.type, desc.name};
	}
	return {Game::UNKNOWN, WadType::UNKNOWN, "unknown"};
}

s32 header_size_of_wad(Game game, WadType type)
{
	for (WadFileDescription& desc : WAD_FILE_TYPES) {
		if (desc.game == game && desc.type == type) {
			return desc.header_size;
		}
	}
	for (WadFileDescription& desc : WAD_FILE_TYPES) {
		if (desc.game == Game::UNKNOWN && desc.type == type) {
			return desc.header_size;
		}
	}
	verify_not_reached("Failed to identify WAD header.");
}
