#ifndef MOBY_H
#define MOBY_H

#include "reflection/refolder.h"
#include "vec.h"

class moby {
public:
	moby(uint32_t uid);

	vec3f position() const;
	void  set_position(vec3f position);

	vec3f rotation() const;
	void  set_rotation(vec3f rotation);

	std::string name;
	bool selected;

private:
	uint32_t _uid;

	vec3f _position;
	vec3f _rotation;

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
