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

#include "tool.h"

tool::tool() {
	static int s_id = 0;
	_id = ++s_id;
}

int tool::id() {
	return _id;
}

void tool::close(app& a) {
	auto iter = std::find_if(a.tools.begin(), a.tools.end(),
		[=](auto& ptr) { return ptr.get() == this; });
	a.tools.erase(iter);
}
