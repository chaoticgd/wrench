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

typedef u128 vec4;
typedef vec4 vec4f;
typedef u128 quat;

struct mtx3 {
	u128 value[3];
};

struct mtx4 {
	u128 value[4];
};
