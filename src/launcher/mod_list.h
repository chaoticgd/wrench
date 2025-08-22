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

struct Mod
{
	std::string path;
	GameInfo info;
	bool enabled = false;
};

struct ModImage
{
	ModImage(GlTexture t, s32 w, s32 h, std::string p)
		: texture(std::move(t)), width(w), height(h), path(std::move(p)) {}
	GlTexture texture;
	s32 width;
	s32 height;
	std::string path;
};

Mod* mod_list(const std::string& filter);
void load_mod_list(const std::vector<std::string>& mods_folders);
void free_mod_list();
std::vector<std::string> enabled_mods();
bool any_mods_enabled();

extern std::vector<ModImage> g_mod_images;
extern std::vector<std::string> g_mod_builds;

#endif
