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

struct vec4 {
	long long value;
};

typedef vec4 vec4f;

struct quat {
	long long value;
};

struct mtx3 {
	long long value[3];
};

struct mtx4 {
	long long value[4];
};
