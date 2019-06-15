#ifndef VEC_H
#define VEC_H

#include "stream.h"

packed_struct(vec3f,

	vec3f() {
		x = 0;
		y = 0;
		z = 0;
	}
	
	vec3f(float x_, float y_, float z_) {
		x = x_;
		y = y_;
		z = z_;
	}

	float components[0];
	float x;
	float y;
	float z;
)

#endif
