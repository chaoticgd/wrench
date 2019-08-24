#ifndef MODEL_H
#define MODEL_H

#include <array>
#include <vector>
#include <glm/glm.hpp>

using vertex_array = std::vector<std::array<glm::vec3, 3>>;

class model {
public:
	virtual ~model() {}

	virtual vertex_array triangles() const = 0;
};

#endif
