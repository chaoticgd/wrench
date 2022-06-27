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
	{"pbpx_955.16", Game::RAC, Region::JAPAN, "Ratchet & Clank"}, // japan original
	{"sced_510.75", Game::RAC, Region::EU, "Ratchet & Clank"}, // eu demo
	{"sces_509.16", Game::RAC, Region::EU, "Ratchet & Clank"}, // eu black label/plantinum
	{"scus_971.99", Game::RAC, Region::US, "Ratchet & Clank"}, // us original/greatest hits
	{"scus_972.09", Game::RAC, Region::US, "Ratchet & Clank"}, // us demo 1
	{"scus_972.40", Game::RAC, Region::US, "Ratchet & Clank"}, // us demo 2
	{"scaj_200.52", Game::GC, Region::JAPAN, "Ratchet & Clank: Going Commando"}, // japan original
	{"sces_516.07", Game::GC, Region::EU, "Ratchet & Clank 2"}, // eu original/platinum
	{"scus_972.68", Game::GC, Region::US, "Ratchet & Clank: Going Commando"}, // us greatest hits
	{"scus_972.68", Game::GC, Region::US, "Ratchet & Clank: Going Commando"}, // us original
	{"scus_973.22", Game::GC, Region::US, "Ratchet & Clank: Going Commando"}, // us demo
	{"scus_973.23", Game::GC, Region::US, "Ratchet & Clank: Going Commando"}, // us retail employees demo
	{"scus_973.74", Game::GC, Region::US, "Ratchet & Clank: Going Commando"}, // us rac2 + jak demo
	{"pcpx_966.53", Game::UYA, Region::JAPAN, "Ratchet & Clank: Up Your Arsenal"}, // japan promotional
	{"sced_528.47", Game::UYA, Region::EU, "Ratchet & Clank 3"}, // eu demo
	{"sced_528.48", Game::UYA, Region::EU, "Ratchet & Clank 3"}, // r&c3 + sly 2 demo
	{"sces_524.56", Game::UYA, Region::EU, "Ratchet & Clank 3"}, // eu original/plantinum
	{"scps_150.84", Game::UYA, Region::JAPAN, "Ratchet & Clank: Up Your Arsenal"}, // japan original
	{"scus_973.53", Game::UYA, Region::US, "Ratchet & Clank: Up Your Arsenal"}, // us original
	{"scus_974.11", Game::UYA, Region::US, "Ratchet & Clank: Up Your Arsenal"}, // us demo
	{"scus_974.13", Game::UYA, Region::US, "Ratchet & Clank: Up Your Arsenal"}, // us beta
	{"tces_524.56", Game::UYA, Region::EU, "Ratchet & Clank 3"}, // eu beta trial code
	{"pcpx_980.17", Game::DL, Region::JAPAN, "Ratchet & Clank 4"}, // japan demo
	{"sced_536.60", Game::DL, Region::EU, "Ratchet: Gladiator"}, // jak x glaiator demo
	{"sces_532.85", Game::DL, Region::EU, "Ratchet: Gladiator"}, // eu original/platinum
	{"scps_150.99", Game::DL, Region::JAPAN, "Ratchet & Clank 4"}, // japan special gift package
	{"scps_193.28", Game::DL, Region::JAPAN, "Ratchet & Clank 4"}, // japan reprint
	{"scus_974.65", Game::DL, Region::US, "Ratchet: Deadlocked"}, // us original
	{"scus_974.85", Game::DL, Region::US, "Ratchet: Deadlocked"}, // us demo
	{"scus_974.87", Game::DL, Region::US, "Ratchet: Deadlocked"} // us public beta
};

Release identify_release(const IsoDirectory& root) {
	Release result;
	for(const IsoFileRecord& file : root.files) {
		for(const Release& release : RELEASES) {
			if(strcmp(release.elf_name, file.name.c_str()) == 0) {
				result = release;
				break;
			}
		}
	}
	return result;
}
