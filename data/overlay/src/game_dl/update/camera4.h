#pragma wrench parser on

struct camera4 { // 0x40
	/* 0x00 */ cameraShared s;
	/* 0x20 */ short int swTimer[4];
	/* 0x28 */ short int camRequested[4];
	/* 0x30 */ short int centerTimer[4];
	/* 0x38 */ char overridePad[4];
	/* 0x3c */ char fpsInit[4];
};
