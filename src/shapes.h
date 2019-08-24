#ifndef SHAPES_H
#define SHAPES_H

#include "model.h"

class cube_model : public model {
public:
	cube_model();

	vertex_array triangles() const override;
};

#endif
