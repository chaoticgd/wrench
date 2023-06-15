#pragma wrench parser on

struct cameraShared { // 0x20
	/* 0x00 */ float leadStrength;
	/* 0x04 */ void *pCam;
	/* 0x08 */ spherelink sphereVolume;
	/* 0x0c */ int cuboidVolume;
	/* 0x10 */ cylinderlink cylinderVolume;
	/* 0x14 */ int pathLink;
	/* 0x18 */ float focusHeight;
	/* 0x1c */ unsigned char priority;
	/* 0x1d */ unsigned char blendTypeIn;
	/* 0x1e */ unsigned char blendTypeOut;
	/* 0x1f */ unsigned char activationType;
};
