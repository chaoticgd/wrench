#ifndef MOBY_H
#define MOBY_H

#include <glm/glm.hpp>

#include "reflection/refolder.h"

class moby {
public:
	moby(uint32_t uid);

	glm::vec3 position() const;
	void set_position(glm::vec3 position);

	glm::vec3 rotation() const;
	void set_rotation(glm::vec3 rotation);

	std::string name;
	bool selected;

private:
	uint32_t _uid;

	glm::vec3 _position;
	glm::vec3 _rotation;

public:
	template <typename... T>
	void reflect(T... callbacks) {
		rf::reflector r(this, callbacks...);
		r.visit_f("UID",      [=]() { return _uid; }, [](uint32_t) {});
		r.visit_r("Name",     name);
		r.visit_m("Position", &moby::position, &moby::set_position);
		r.visit_m("Rotation", &moby::rotation, &moby::set_rotation);
	}
};

#endif
