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

void check_error(wrench_result result) {
	if(result != wrench_result::OKAY) {
		throw std::runtime_error(
			"translate_command tried to act on an invalid object. "
			"The history stack has been corrupted!");
	}
}

translate_command::translate_command(
		level* lvl,
		object_list objects,
		glm::vec3 displacement)
	: _lvl(lvl),
	  _displacement(displacement),
	  _objects(objects) {
	auto result = _lvl->world.for_each_point_object_in(_objects, [=](object_id id, auto& object) {
		_prev_positions[id] = glm::vec3(object.mat()[3]);
	});
	check_error(result);
}

void translate_command::apply(wrench_project* project) {
	auto result = _lvl->world.for_each_point_object_in(_objects, [=](object_id id, auto& object) {
		object.translate(_displacement);
	});
	check_error(result);
}

void translate_command::undo(wrench_project* project) {
	auto result = _lvl->world.for_each_point_object_in(_objects, [=](object_id id, auto& object) {
		object.set_translation(_prev_positions.at(id));
	});
	check_error(result);
}
