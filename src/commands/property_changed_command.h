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
