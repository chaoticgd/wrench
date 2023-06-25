#pragma wrench parser on

struct vec2 {
	float x;
	float y;
};

struct vec3 {
	float x;
	float y;
	float z;
};

struct alignas(16) vec4 {
	float x;
	float y;
	float z;
	float w;
};

typedef vec4 vec4f;
typedef vec4 quat;

struct mtx3 {
	vec4 value[3];
};

struct mtx4 {
	vec4 value[4];
};
