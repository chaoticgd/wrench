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

struct WadFileDescription {
	const char* name;
	Game game;
	WadType type;
	s32 header_size;
	s32 secondary_offset = -1; // Secondary offset to check if multiple types of files have the same header size.
	s32 secondary_value = -1;
};

static WadFileDescription WAD_FILE_TYPES[] = {
	{"mpeg"  , Game::RAC2   , WadType::MPEG       , 0x0328},
	{"misc"  , Game::RAC2   , WadType::MISC       , 0x0040},
	{"hud"   , Game::RAC2   , WadType::HUD        , 0x1870},
	{"bonus" , Game::RAC2   , WadType::BONUS      , 0x0a48},
	{"audio" , Game::RAC2   , WadType::AUDIO      , 0x1800},
	{"space" , Game::RAC2   , WadType::SPACE      , 0x0ba8},
	{"scene" , Game::RAC2   , WadType::SCENE      , 0x0170},
	{"gadget", Game::RAC2   , WadType::GADGET     , 0x03c8, 0x8, 0x00b1},
	{"gadget", Game::RAC3   , WadType::GADGET     , 0x03c8, 0x8, 0x0a5d},
	{"gadget", Game::UNKNOWN, WadType::GADGET     , 0x03c8},
	{"armor" , Game::RAC2   , WadType::ARMOR      , 0x00f8},
	{"level" , Game::UNKNOWN, WadType::LEVEL      , 0x0060},
	{"audio" , Game::RAC2   , WadType::LEVEL_AUDIO, 0x1018},
	{"scene" , Game::RAC2   , WadType::LEVEL_SCENE, 0x137c},
	{"mpeg"  , Game::RAC3   , WadType::MPEG       , 0x0648, 0xc, 0x0038},
	{"mpeg"  , Game::DL     , WadType::MPEG       , 0x0648, 0xc, 0x0040},
	{"mpeg"  , Game::UNKNOWN, WadType::MPEG       , 0x0648},
	{"misc"  , Game::RAC3   , WadType::MISC       , 0x0048},
	{"bonus" , Game::RAC3   , WadType::BONUS      , 0x0bf0},
	{"space" , Game::RAC3   , WadType::SPACE      , 0x0c30},
	{"armor" , Game::RAC3   , WadType::ARMOR      , 0x0398},
	{"audio" , Game::RAC3   , WadType::AUDIO      , 0x2340},
	{"2ab0"  , Game::RAC3   , WadType::UNKNOWN    , 0x2ab0},
	{"audio" , Game::RAC3   , WadType::LEVEL_AUDIO, 0x1818},
	{"scene" , Game::UNKNOWN, WadType::LEVEL_SCENE, 0x26f0},
	{"misc"  , Game::DL     , WadType::MISC       , 0x0050},
	{"bonus" , Game::DL     , WadType::BONUS      , 0x02a8},
	{"space" , Game::DL     , WadType::SPACE      , 0x0068, 0xc, 0x0252},
	{"online", Game::DL     , WadType::ONLINE     , 0x0068, 0xc, 0x0c6a},
	{"level" , Game::UNKNOWN, WadType::LEVEL      , 0x0068},
	{"armor" , Game::DL     , WadType::ARMOR      , 0x0228},
	{"audio" , Game::DL     , WadType::AUDIO      , 0xa870},
	{"hud"   , Game::DL     , WadType::HUD        , 0x0f88},
	{"level" , Game::DL     , WadType::LEVEL      , 0x0c68},
	{"audio" , Game::DL     , WadType::LEVEL_AUDIO, 0x02a0},
	{"audio" , Game::UNKNOWN, WadType::LEVEL_AUDIO, 0x1000},
	{"scene" , Game::UNKNOWN, WadType::LEVEL_SCENE, 0x2420}
};

std::tuple<Game, WadType, const char*> identify_wad(Buffer header) {
	for(WadFileDescription& desc : WAD_FILE_TYPES) {
		if(desc.header_size != header.size()) {
			continue;
		}
		
		if(desc.secondary_offset > -1 && header.read<s32>(desc.secondary_offset, "header") != desc.secondary_value) {
			continue;
		}
		
		return {desc.game, desc.type, desc.name};
	}
	return {Game::UNKNOWN, WadType::UNKNOWN, "unknown"};
}

s32 header_size_of_wad(Game game, WadType type) {
	for(WadFileDescription& desc : WAD_FILE_TYPES) {
		if(desc.game == game && desc.type == type) {
			return desc.header_size;
		}
	}
	for(WadFileDescription& desc : WAD_FILE_TYPES) {
		if(desc.game == Game::UNKNOWN && desc.type == type) {
			return desc.header_size;
		}
	}
	verify_not_reached("Failed to identify WAD header.");
}
