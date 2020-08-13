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

#include "translate_command.h"

translate_command::translate_command(
		level* lvl,
		glm::vec3 displacement)
	: _lvl(lvl),
	  _displacement(displacement) {
	_lvl->for_each<matrix_entity>([&](matrix_entity& ent) {
		if(ent.selected) {
			_prev_positions[ent.id] = ent.local_to_world[3];
		}
	});
	_lvl->for_each<euler_entity>([&](euler_entity& ent) {
		if(ent.selected) {
			_prev_positions[ent.id] = ent.position;
		}
	});
}

void translate_command::apply(wrench_project* project) {
	_lvl->for_each<matrix_entity>([&](matrix_entity& ent) {
		if(map_contains(_prev_positions, ent.id)) {
			ent.local_to_world[3].x += _displacement.x;
			ent.local_to_world[3].y += _displacement.y;
			ent.local_to_world[3].z += _displacement.z;
		}
	});
	_lvl->for_each<euler_entity>([&](euler_entity& ent) {
		if(map_contains(_prev_positions, ent.id)) {
			ent.position += _displacement;
		}
	});
}

void translate_command::undo(wrench_project* project) {
	_lvl->for_each<matrix_entity>([&](matrix_entity& ent) {
		if(map_contains(_prev_positions, ent.id)) {
			glm::vec3& pos = _prev_positions.at(ent.id);
			ent.local_to_world[3].x = pos.x;
			ent.local_to_world[3].y = pos.y;
			ent.local_to_world[3].z = pos.z;
		}
	});
	_lvl->for_each<euler_entity>([&](euler_entity& ent) {
		if(map_contains(_prev_positions, ent.id)) {
			ent.position = _prev_positions.at(ent.id);
		}
	});
}
