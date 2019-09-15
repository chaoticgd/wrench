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

bool base_level::is_selected(const game_object* obj) const {
	return std::find(selection.begin(), selection.end(), obj->base()) != selection.end();
}

void base_level::inspect(inspector_callbacks* cb) {
	if(selection.size() == 1) {
		for_each_game_object([=](game_object* object) {
			if(object->base() == selection[0]) {
				object->inspect(cb);
			}
		});
	}
}

std::string game_object::base_string() const {
	return int_to_hex(base());
}

void game_object::inspect(inspector_callbacks* cb) {
	cb->category("Game Object");
	cb->input_string<game_object>("Offset", &game_object::base_string);
}

void point_object::inspect(inspector_callbacks* cb) {
	game_object::inspect(cb);
	cb->category("Point Object");
	cb->input_vector3<point_object>("Position", &point_object::position, &point_object::set_position);
	cb->input_vector3<point_object>("Rotation", &point_object::rotation, &point_object::set_rotation);
}

void base_tie::inspect(inspector_callbacks* cb) {
	point_object::inspect(cb);
	cb->category("Tie");
}

void base_shrub::inspect(inspector_callbacks* cb) {
	point_object::inspect(cb);
	cb->category("Shrub");
}

void base_spline::inspect(inspector_callbacks* cb) {
	game_object::inspect(cb);
	cb->category("Spline");
}

void base_moby::inspect(inspector_callbacks* cb) {
	point_object::inspect(cb);
	cb->category("Moby");
	cb->input_integer<base_moby>("UID",      &base_moby::uid,       &base_moby::set_uid);
	cb->input_uint16 <base_moby>("Class",    &base_moby::class_num, &base_moby::set_class_num);
}
