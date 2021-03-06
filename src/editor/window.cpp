/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2020 chaoticgd

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

#include "window.h"

#include "app.h"

window::window() {
	static int s_id = 0;
	_id = ++s_id;
}

int window::id() {
	return _id;
}

bool window::is_unique() const {
	return true;
}

bool window::has_padding() const {
	return true;
}

void window::close(app& a) {
	auto iter = std::find_if(a.windows.begin(), a.windows.end(),
		[&](auto& ptr) { return ptr.get() == this; });
	a.windows.erase(iter);
}
