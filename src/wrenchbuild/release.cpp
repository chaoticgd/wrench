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

#include "release.h"

static const Release RELEASES[] = {
	{"pbpx_955.16", Game::RAC, Region::JAPAN}, // japan original
	{"sced_510.75", Game::RAC, Region::EU}, // eu demo
	{"sces_509.16", Game::RAC, Region::EU}, // eu black label/plantinum
	{"scus_971.99", Game::RAC, Region::US}, // us original/greatest hits
	{"scus_972.09", Game::RAC, Region::US}, // us demo 1
	{"scus_972.40", Game::RAC, Region::US}, // us demo 2
	{"scaj_200.52", Game::GC, Region::JAPAN}, // japan original
	{"sces_516.07", Game::GC, Region::EU}, // eu original/platinum
	{"scus_972.68", Game::GC, Region::US}, // us greatest hits
	{"scus_972.68", Game::GC, Region::US}, // us original
	{"scus_973.22", Game::GC, Region::US}, // us demo
	{"scus_973.23", Game::GC, Region::US}, // us retail employees demo
	{"scus_973.74", Game::GC, Region::US}, // us rac2 + jak demo
	{"pcpx_966.53", Game::UYA, Region::JAPAN}, // japan promotional
	{"sced_528.47", Game::UYA, Region::EU}, // eu demo
	{"sced_528.48", Game::UYA, Region::EU}, // r&c3 + sly 2 demo
	{"sces_524.56", Game::UYA, Region::EU}, // eu original/plantinum
	{"scps_150.84", Game::UYA, Region::JAPAN}, // japan original
	{"scus_973.53", Game::UYA, Region::US}, // us original
	{"scus_974.11", Game::UYA, Region::US}, // us demo
	{"scus_974.13", Game::UYA, Region::US}, // us beta
	{"tces_524.56", Game::UYA, Region::EU}, // eu beta trial code
	{"pcpx_980.17", Game::DL, Region::JAPAN}, // japan demo
	{"sced_536.60", Game::DL, Region::EU}, // jak x glaiator demo
	{"sces_532.85", Game::DL, Region::EU}, // eu original/platinum
	{"scps_150.99", Game::DL, Region::JAPAN}, // japan special gift package
	{"scps_193.28", Game::DL, Region::JAPAN}, // japan reprint
	{"scus_974.65", Game::DL, Region::US}, // us original
	{"scus_974.85", Game::DL, Region::US}, // us demo
	{"scus_974.87", Game::DL, Region::US} // us public beta
};

Release identify_release(const IsoDirectory& root) {
	Release result;
	for(const IsoFileRecord& file : root.files) {
		for(const Release& release : RELEASES) {
			if(release.elf_name == file.name) {
				result = release;
				break;
			}
		}
	}
	return result;
}
