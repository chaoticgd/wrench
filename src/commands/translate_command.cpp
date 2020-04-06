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
		std::vector<object_id> objects,
		glm::vec3 displacement)
	: _lvl(lvl),
	  _displacement(displacement) {
    FOR_EACH_MATRIX_OBJECT(_lvl->world, ([=](object_id id, auto& object) {
        if(std::find(objects.begin(), objects.end(), id) != objects.end()) {
            _prev_positions[id] = glm::vec3(object.mat()[3]);
        }
    }));
}

void translate_command::apply(wrench_project* project) {
    FOR_EACH_MATRIX_OBJECT(_lvl->world, ([=](object_id id, auto& object) {
        if(_prev_positions.find(id) != _prev_positions.end()) {
            object.translate(_displacement);
        }
    }));
}

void translate_command::undo(wrench_project* project) {
    FOR_EACH_MATRIX_OBJECT(_lvl->world, ([=](object_id id, auto& object) {
        if(_prev_positions.find(id) != _prev_positions.end()) {
            object.set_translation(_prev_positions.at(id));
        }
    }));
}
