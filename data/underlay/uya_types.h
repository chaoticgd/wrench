// wrench parser on

struct M2871_Ocean {
	/* 0x00 */ int overlaySpriteId;
	/* 0x04 */ int underlaySpriteId;
	/* 0x08 */ float waveSpeed;
	/* 0x0c */ float waveCrest;
	/* 0x10 */ float waveSurge;
	/* 0x14 */ float waveRippleSize; // floats < 1 makes the ripples very choppy
	/* 0x18 */ float waveDirection;
	/* 0x1c */ float waveDirectionVariation;
	/* 0x20 */ float waveShimmerIntensity; // setting this really high causes bright spots then the surge goes high enough
	/* 0x24 */ float overlayTiling; // larger to stretch the tiling
	/* 0x28 */ float overlayDirection;
	/* 0x2c */ float overlaySpeed;
	/* 0x30 */ int underlayColorABGR;
	/* 0x34 */ int overlayColorABGR;
	/* 0x38 */ unsigned char fogColorGreen; // broke these out because GRB is a weird order, 0x3C isn't opacity
	/* 0x39 */ unsigned char fogColorRed;
	/* 0x3a */ unsigned char fogColorBlue;
	/* 0x3b */ unsigned char pad;
	/* 0x3c */ unsigned char unk_3c; // more padding?
	/* 0x3d */ unsigned char fogIntensityFar; // Anything above 0x64 (100) causes jagged edges
	/* 0x3e */ unsigned char fogIntensityNear; // Value higher than far intensity is ineffective
	/* 0x3f */ unsigned char blueFilter; // Applies extra blue to the fog color for whatever reason
	/* 0x40 */ float fogDistNear;
	/* 0x44 */ float fogDistFar;
	/* 0x48 */ float waterShimmerThreshold; // supposed to simulate sun reflecting off the water?
	/* 0x4c */ float posZ; // Z axis the plane renders on
	/* 0x50 */ int activeChunkId;
	/* 0x54 */ int unk_54; // controls rendering but unsure of data source
	/* 0x58 */ int hiddenCuboidId; // hides the plane when hero is inside this cuboid
	/* 0x5c */ int unk_5C; // controls rendering but unsure of data source
	/* 0x60 */ int unk_60;
	/* 0x64 */ int unk_64;
	/* 0x68 */ short unk_68;
	/* 0x6a */ short unk_6a;
	/* 0x6c */ int unk_6c;
};
