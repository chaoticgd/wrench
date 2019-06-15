#include "moby.h"

moby::moby(uint32_t uid)
	: selected(false),
	  _uid(uid) {}

vec3f moby::position() const {
	return _position;
}

void  moby::set_position(vec3f position) {
	_position = position;
}

vec3f moby::rotation() const {
	return _rotation;
}

void  moby::set_rotation(vec3f rotation) {
	_rotation = rotation;
}
