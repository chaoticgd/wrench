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
#include "../reflection/refolder.h"

template <typename T>
class property_changed_command : public command {
public:
	property_changed_command(rf::property<T> property, T new_value);

	void apply() override;
	void undo() override;
private:
	rf::property<T> _property;
	T _old_value;
	T _new_value;
};

template <typename T>
property_changed_command<T>::property_changed_command(rf::property<T> property, T new_value)
	: _property(property),
	  _old_value(property.get()),
	  _new_value(new_value) {}

template <typename T>
void property_changed_command<T>::apply() {
	_property.set(_new_value);
}

template <typename T>
void property_changed_command<T>::undo() {
	_property.set(_old_value);
}

#endif
