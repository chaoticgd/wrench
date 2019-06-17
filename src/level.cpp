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

void level::apply_command(std::unique_ptr<command> action) {
	action->inject_level_pointer(static_cast<level_impl*>(this));
	action->apply();
	_history.resize(_history_index - 1);
	_history.push_back(std::move(action));
	_history_index++;
}
	
bool level::undo() {
	if(_history_index <= 0) {
		return false;
	}
	_history[--_history_index]->undo();
	return true;
}

bool level::redo() {
	if(_history_index >= _history.size()) {
		return false;
	}
	_history[_history_index++]->apply();
	return true;
}

level_impl::level_impl() {}
