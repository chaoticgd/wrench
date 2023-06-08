#pragma wrench parser on

struct update8299 { // 0xc0
	/* 0x00 */ SubVars sVars;
	/* 0x50 */ CommandVars cVars;
	/* 0x64 */ grindlink grindPath;
	/* 0x68 */ int target;
	/* 0x6c */ GrindPath *pGrindPath;
	/* 0x70 */ moby *pAttachMoby;
	/* 0x74 */ int attachJoint;
	/* 0x78 */ float animFrame;
	/* 0x7c */ moby *pSegment[8];
	/* 0xa0 */ vec4 lastAttachPos;
	/* 0xb0 */ Hero *pAttachedHero;
	/* 0xb4 */ int pad[3];
};
