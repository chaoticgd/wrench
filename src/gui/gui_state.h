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

#ifndef WRENCHGUI_GUI_STATE_H
#define WRENCHGUI_GUI_STATE_H

#include <any>
#include <core/util.h>

namespace gui {

class StateNode {
public:
	s32& integer(const char* tag);
	std::vector<s32>& integers(const char* tag);
	
	bool& boolean(const char* tag);
	std::vector<bool>& booleans(const char* tag);
	
	std::string& string(const char* tag);
	std::vector<std::string>& strings(const char* tag);
	
	StateNode& subnode(const char* tag);
	std::vector<StateNode>& subnodes(const char* tag);
	
private:
	std::map<std::string, std::any> _attributes;
};

}

extern gui::StateNode g_gui;

#endif
