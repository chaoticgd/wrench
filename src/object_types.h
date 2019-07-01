#ifndef OBJECT_TYPES_H
#define OBJECT_TYPES_H

#include <string>
#include <glm/glm.hpp>

class point_object {
public:
	virtual std::string label() const = 0;
	virtual glm::vec3 position() const = 0;
};

#endif
