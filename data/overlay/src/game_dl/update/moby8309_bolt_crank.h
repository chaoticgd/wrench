#pragma wrench parser on

struct update8309 { // 0x300
	/* 0x000 */ float open;
	/* 0x004 */ int cameraPos;
	/* 0x008 */ float numRotationsToOpen;
	/* 0x00c */ moby *pBoltCrank;
	/* 0x010 */ BaseCrankBoltInterface_t *pCrankInterface;
	/* 0x014 */ float colorPulse;
	/* 0x018 */ float fVehiclePadColorPulse;
	/* 0x01c */ int helpTimer;
	/* 0x020 */ moby *pLight;
	/* 0x024 */ moby *pGlass;
	/* 0x028 */ moby *pTurrets[2];
	/* 0x030 */ Tweaker bigCogTweak;
	/* 0x0b0 */ Tweaker smallCogTweak[4];
	/* 0x2b0 */ float turretColorPulse[2];
	/* 0x2b8 */ int vehiclePad;
	/* 0x2bc */ int turretLink;
	/* 0x2c0 */ moby *pUpgradeTurrets[2];
	/* 0x2c8 */ float upgradeTurretPulseColors[2];
	/* 0x2d0 */ int healthPad;
	/* 0x2d4 */ int ammoPad;
	/* 0x2d8 */ unsigned int boltUID;
	/* 0x2dc */ char spawnedUpgrades;
	/* 0x2dd */ char locked;
	/* 0x2de */ char midMissionSave;
	/* 0x2df */ char disabled;
	/* 0x2e0 */ int masterState;
	/* 0x2e4 */ char team;
	/* 0x2e5 */ char toTeam;
	/* 0x2e6 */ char prevTeam;
	/* 0x2e7 */ char homeNode;
	/* 0x2e8 */ char covered;
	/* 0x2e9 */ char light;
	/* 0x2ea */ char registerOnRadar;
	/* 0x2eb */ char opened;
	/* 0x2ec */ int teleporterPad;
	/* 0x2f0 */ int resurrectionPt;
	/* 0x2f4 */ int upgradeController;
	/* 0x2f8 */ int vehiclePad2;
	/* 0x2fc */ int pad3[1];
};
