/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019 chaoticgd

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

#ifndef MENU_H
#define MENU_H

#include <vector>
#include <functional>
#include <initializer_list>

#include "app.h"

class menu {
public:
	menu(const char* name, std::function<void(app&)> callback);
	menu(const char* name, std::initializer_list<menu> children);

	void render(app& a) const;

private:
	const char* _name;
	std::function<void(app&)> _callback;
	std::vector<menu> _children;
};

#endif
