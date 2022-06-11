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
#include <gui/gui_state.h>

class Mod {
public:
	Mod(gui::StateNode& node) : _node(node) {}
	
	std::string& path() { return _node.string("path"); }
	
	std::string& name() { return _node.string("name"); }
	std::string& author() { return _node.string("author"); }
	std::string& description() { return _node.string("description"); }
	std::string& version() { return _node.string("version"); }
	std::vector<std::string>& images() { return _node.strings("images"); }
	
	bool& enabled() { return _node.boolean("enabled"); }
private:
	gui::StateNode& _node;
};

void mod_list(const std::string& filter);
void load_mod_list(const std::vector<std::string>& mods_folders);
void free_mod_list();

#endif
