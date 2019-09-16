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

#include "level.h"

#include "util.h"
#include "formats/level_impl.h"

base_level::base_level() {}

const texture_provider* base_level::get_texture_provider() const {
	return const_cast<base_level*>(this)->get_texture_provider();
}

void base_level::for_each_game_object_const(std::function<void(const game_object*)> callback) const {
	return const_cast<base_level*>(this)->for_each_game_object
		(static_cast<std::function<void(game_object*)>>(callback));
}

bool base_level::is_selected(const game_object* obj) const {
	return std::find(selection.begin(), selection.end(), obj->base()) != selection.end();
}

std::string game_object::base_string() const {
	return int_to_hex(base());
}
