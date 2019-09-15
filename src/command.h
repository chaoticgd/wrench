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

#ifndef COMMAND_H
#define COMMAND_H

#include <stdexcept>

# /*
#	Virtual base class representing an undo/redo command.
# */

class app;
class wrench_project;

class command {
	friend wrench_project;
public:
	virtual ~command() {}

protected:
	command() {}

	// Should only throw command_error.
	virtual void apply(wrench_project* project) = 0;

	// Should only throw command_error.
	virtual void undo(wrench_project* project) = 0;
};

class command_error : public std::runtime_error {
	using std::runtime_error::runtime_error;
};

#endif
