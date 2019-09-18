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

#ifndef PROPERTY_CHANGED_COMMAND_H
#define PROPERTY_CHANGED_COMMAND_H

#include "../command.h"

# /*
#	Undo/redo command for modifying object properties.
# */

template <typename T_owner, typename T>
struct property {
	typedef T    (T_owner::*getter)() const;
	typedef void (T_owner::*setter)(T);
	getter get;
	setter set;
};

template <typename T_owner, typename T>
class property_changed_command : public command {
public:
	property_changed_command(
			T_owner owner,
			property<T_owner, T> property_,
			T new_value);

	void apply(wrench_project* project) override;
	void undo(wrench_project* project) override;
	
private:
	T_owner _owner;
	property<T_owner, T> _property;
	T _old_value;
	T _new_value;
};

template <typename T_owner, typename T>
property_changed_command<T_owner, T>::property_changed_command(
		T_owner owner,
		property<T_owner, T> property_,
		T new_value)
	: _owner(owner),
	  _property(property_),
	  _old_value((_owner.*property_.get)()),
	  _new_value(new_value) {}

template <typename T_owner, typename T>
void property_changed_command<T_owner, T>::apply(wrench_project* project) {
	(_owner.*_property.set)(_new_value);
}

template <typename T_owner, typename T>
void property_changed_command<T_owner, T>::undo(wrench_project* project) {
	(_owner.*_property.set)(_old_value);
}

#endif
