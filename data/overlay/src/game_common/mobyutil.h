#pragma wrench parser on

struct Tweaker { // 0x80
	/* 0x00 */ Manipulator manip;
	/* 0x40 */ vec4f rot;
	/* 0x50 */ vec4f speed;
	/* 0x60 */ vec4f target;
	/* 0x70 */ float scale;
	/* 0x74 */ int joint;
	/* 0x78 */ moby *pMoby;
	/* 0x7c */ int pad[1];
};
