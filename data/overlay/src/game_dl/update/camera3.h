#pragma wrench parser on

struct camera3 { // 0x40
	/* 0x00 */ cameraShared s;
	/* 0x20 */ int camPath;
	/* 0x24 */ grindlink indexPath;
	/* 0x28 */ int camRefPath;
	/* 0x2c */ int indexRefPath;
	/* 0x30 */ int followType;
	/* 0x34 */ short int inited;
	/* 0x36 */ short int dir;
	/* 0x38 */ float closestOfs;
	/* 0x3c */ char grindZip;
	/* 0x3d */ char pad[3];
};
