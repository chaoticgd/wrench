#pragma wrench parser on

struct update3709 { // 0x2e0
	/* 0x000 */ SubVars subVars;
	/* 0x050 */ TargetVars tVars;
	/* 0x0e0 */ MoveVars_V2 mVars;
	/* 0x290 */ FlashVars fVars;
	/* 0x2a0 */ moby *pMorphed;
	/* 0x2a4 */ int lifetime;
	/* 0x2a8 */ float origScale;
	/* 0x2ac */ float morphScale;
	/* 0x2b0 */ int death;
	/* 0x2b4 */ int formerPlayerIdx;
	/* 0x2b8 */ int iSoundLoopHandle;
	/* 0x2bc */ int flameOn;
	/* 0x2c0 */ moby *pTargetMoby;
	/* 0x2c4 */ int pad[2];
	/* 0x2d0 */ vec4 target;
};
