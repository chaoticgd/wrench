#pragma wrench parser on

struct update3950 { // 0x90
	/* 0x00 */ SubVars sVars;
	/* 0x50 */ ObjectiveVars oVars;
	/* 0x6c */ int index;
	/* 0x70 */ char enabled;
	/* 0x71 */ char allowReplay;
	/* 0x72 */ signed char save;
	/* 0x73 */ signed char srMessage;
	/* 0x74 */ int heroFinalPosCuboid;
	/* 0x78 */ int nextCutscene;
	/* 0x7c */ char replayUntilMissionCompleted;
	/* 0x7d */ char occlusionOff;
	/* 0x7e */ char moviePlayed;
	/* 0x7f */ char movieActivated;
	/* 0x80 */ char movieQueued;
	/* 0x84 */ moby *pNextCutscene;
	/* 0x88 */ missionlink objective;
	/* 0x8c */ int missionRequired;
};
