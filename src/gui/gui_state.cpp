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

#include "gui_state.h"

template <typename T>
static T& access_attribute(std::map<std::string, std::any>& attributes, const char* tag, const char* type_name) {
	try {
		std::any& value = attributes[tag];
		if(!value.has_value()) {
			value = T{};
		}
		return std::any_cast<T&>(attributes[tag]);
	} catch(std::bad_any_cast&) {
		verify_not_reached("'%s' is not %s.", tag, type_name);
	}
}

s32& gui::StateNode::integer(const char* tag) {
	return access_attribute<s32>(_attributes, tag, "an integer");
}

std::vector<s32>& gui::StateNode::integers(const char* tag) {
	return access_attribute<std::vector<s32>>(_attributes, tag, "a list of integers");
}

bool& gui::StateNode::boolean(const char* tag) {
	return access_attribute<bool>(_attributes, tag, "an boolean");
}

std::vector<bool>& gui::StateNode::booleans(const char* tag) {
	return access_attribute<std::vector<bool>>(_attributes, tag, "a list of booleans");
}

std::string& gui::StateNode::string(const char* tag) {
	return access_attribute<std::string>(_attributes, tag, "a string");
}

std::vector<std::string>& gui::StateNode::strings(const char* tag) {
	return access_attribute<std::vector<std::string>>(_attributes, tag, "a list of strings");
}

gui::StateNode& gui::StateNode::subnode(const char* tag) {
	return access_attribute<StateNode>(_attributes, tag, "a subnode");
}

std::vector<gui::StateNode>& gui::StateNode::subnodes(const char* tag) {
	return access_attribute<std::vector<StateNode>>(_attributes, tag, "a list of subnodes");
}

gui::StateNode g_gui = {};
