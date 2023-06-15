#pragma wrench parser on

struct MissionData {
	/* 0x00 */ missionlink link;
	/* 0x04 */ cameralink introCam;
	/* 0x08 */ int introCutscene;
	/* 0x0c */ cameralink successCam;
	/* 0x10 */ int successCutscene;
	/* 0x14 */ music track;
	/* 0x18 */ char chunk;
	/* 0x19 */ char heroStartPrevPos;
	/* 0x1a */ char missionResults;
	/* 0x1b */ char fillAmmo;
	/* 0x1c */ char pad[4];
};

struct update8355 {
	/* 0x000 */ MissionData missions[8];
	/* 0x100 */ MissionData extraMissions[56];
	/* 0x800 */ missionlink empty;
	/* 0x804 */ int counter;
	/* 0x808 */ char success;
	/* 0x809 */ char hubCampaignCompleted;
	/* 0x80a */ char lossGranted;
};
