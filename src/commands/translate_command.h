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

#ifndef TRANSLATE_COMMAND_H
#define TRANSLATE_COMMAND_H

#include <map>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "../command.h"
#include "../formats/level_impl.h"

# /*
#	Undo/redo command for translating a set of objects.
# */

class translate_command : public command {
public:
	translate_command(
			level* lvl,
			object_list objects,
			glm::vec3 displacement);

	void apply(wrench_project* project) override;
	void undo(wrench_project* project) override;
	
private:
	level* _lvl;
	glm::vec3 _displacement;
	object_list _objects;
	std::map<object_id, glm::vec3> _prev_positions;
};

#endif
