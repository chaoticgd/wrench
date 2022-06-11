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

#ifndef LAUNCHER_MOD_LIST_H
#define LAUNCHER_MOD_LIST_H

#include <core/filesystem.h>
#include <assetmgr/game_info.h>
#include <gui/gui.h>

struct Mod {
	GameInfo game_info;
	fs::path absolute_path;
	bool selected = false;
};

struct ModResult {
	Mod* current;
	std::vector<GlTexture>* images;
	std::vector<Mod>* mods;
	const char* build;
};

ModResult mod_list(const std::string& filter);
void load_mod_list(const std::vector<std::string>& mods_folders);
void free_mod_list();

#endif
