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

std::tuple<Game, WadType, const char*> identify_wad(Buffer header) {
	s32 _0x8 = 0;
	s32 _0xc = 0;
	if(header.size() >= 0x10) {
		_0x8 = header.read<s32>(0x8, "header");
		_0xc = header.read<s32>(0xc, "header");
	}
	switch(header.size()) {
		case 0x0328: return {Game::RAC2, WadType::MPEG, "mpeg"};
		case 0x0040: return {Game::RAC2, WadType::MISC, "misc"};
		case 0x1870: return {Game::RAC2, WadType::HUD, "hud"};
		case 0x0a48: return {Game::RAC2, WadType::BONUS, "bonus"};
		case 0x1800: return {Game::RAC2, WadType::AUDIO, "audio"};
		case 0x0ba8: return {Game::RAC2, WadType::SPACE, "space"};
		case 0x0170: return {Game::RAC2, WadType::SCENE, "scene"};
		case 0x03c8: switch(_0x8) {
			case 0x00b1: return {Game::RAC2, WadType::GADGET, "gadget"};
			case 0x0a5d: return {Game::RAC3, WadType::GADGET, "gadget"};
			default: return {Game::UNKNOWN, WadType::GADGET, "gadget"};
		}
		case 0x00f8: return {Game::RAC2, WadType::ARMOR, "armor"};
		case 0x0060: return {Game::UNKNOWN, WadType::LEVEL, "level"};
		case 0x1018: return {Game::RAC2, WadType::LEVEL_AUDIO, "audio"};
		case 0x137c: return {Game::RAC2, WadType::LEVEL_SCENE, "scene"};
		case 0x0648: switch(_0xc) {
			case 0x0038: return {Game::RAC3, WadType::MPEG, "mpeg"};
			case 0x0040: return {Game::DL, WadType::MPEG, "mpeg"};
			default: return {Game::UNKNOWN, WadType::MPEG, "mpeg"};
		}
		case 0x0048: return {Game::RAC3, WadType::MISC, "misc"};
		case 0x0bf0: return {Game::RAC3, WadType::BONUS, "bonus"};
		case 0x0c30: return {Game::RAC3, WadType::SPACE, "space"};
		case 0x0398: return {Game::RAC3, WadType::ARMOR, "armor"};
		case 0x2340: return {Game::RAC3, WadType::AUDIO, "audio"};
		case 0x2ab0: return {Game::RAC3, WadType::UNKNOWN, "2ab0"};
		case 0x1818: return {Game::RAC3, WadType::LEVEL_AUDIO, "audio"};
		case 0x26f0: return {Game::UNKNOWN, WadType::LEVEL_SCENE, "scene"};
		case 0x0050: return {Game::DL, WadType::MISC, "misc"};
		case 0x02a8: return {Game::DL, WadType::BONUS, "bonus"};
		case 0x0068: switch(_0xc) {
			case 0x0252: return {Game::DL, WadType::SPACE, "space"};
			case 0x0c6a: return {Game::DL, WadType::ONLINE, "online"};
			default: return {Game::UNKNOWN, WadType::LEVEL, "level"};
		}
		case 0x0228: return {Game::DL, WadType::ARMOR, "armor"};
		case 0xa870: return {Game::DL, WadType::AUDIO, "audio"};
		case 0x0f88: return {Game::DL, WadType::HUD, "hud"};
		case 0x0c68: return {Game::DL, WadType::LEVEL, "level"};
		case 0x02a0: return {Game::DL, WadType::LEVEL_AUDIO, "audio"};
		case 0x1000: return {Game::UNKNOWN, WadType::LEVEL_AUDIO, "audio"};
		case 0x2420: return {Game::UNKNOWN, WadType::LEVEL_SCENE, "scene"};
		default: return {Game::UNKNOWN, WadType::UNKNOWN, "unknown"};
	}
}
