#pragma wrench parser on

struct Manipulator { // 0x40
	/* 0x00 */ char animJoint;
	/* 0x01 */ char state;
	/* 0x02 */ signed char scaleOn;
	/* 0x03 */ char absolute;
	/* 0x04 */ int jointId;
	/* 0x08 */ Manipulator *pChain;
	/* 0x0c */ float interp;
	/* 0x10 */ quat q;
	/* 0x20 */ vec4 scale;
	/* 0x30 */ vec4 trans;
};
