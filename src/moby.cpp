#include "moby.h"

moby::moby(uint32_t uid)
	: selected(false),
	  _uid(uid) {}

glm::vec3 moby::position() const {
	return _position;
}

void moby::set_position(glm::vec3 position) {
	_position = position;
}

glm::vec3 moby::rotation() const {
	return _rotation;
}

void moby::set_rotation(glm::vec3 rotation) {
	_rotation = rotation;
}
