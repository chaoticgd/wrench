#pragma wrench parser on

// mc_gamedata

enum Level {
	VELDIN_TUTORIAL = 0,
	NOVALIS = 1,
	ARIDIA = 2,
	KERWAN = 3,
	EUDORA = 4,
	RILGAR = 5,
	NEBULA_G34 = 6,
	UMBRIS = 7,
	BATALIA = 8,
	GASPAR = 9,
	ORXON = 10,
	POKITARU = 11,
	HOVEN = 12,
	OLTANIS_ORBIT = 13,
	OLTANIS = 14,
	QUARTU = 15,
	KALEBO_III = 16,
	VELDIN_ORBIT = 17,
	VELDIN_FINALE = 18
};

typedef int BoltCount;
typedef int Completes;
typedef int ElapsedTime;

struct sceCdCLOCK {
	unsigned char stat;
	unsigned char second;
	unsigned char minute;
	unsigned char hour;
	unsigned char pad;
	unsigned char day;
	unsigned char month;
	unsigned char year;
};

typedef sceCdCLOCK LastSaveTime;

enum GlobalFlag {
	ZOOMERATOR = 0,
	RARITANIUM = 1,
	CODEBOT = 2,
	PREMIUM_NANOTECH = 3,
	ULTRA_NANOTECH = 4,
	MAPOMATIC = 33
};

typedef bool GlobalFlags[128];

typedef char CheatsActivated[12];
typedef char SkillPoints[12];

// Gadget list from: https://creepnt.stream/rc/rccombo.html
enum Gadget {
	UNKNOWN_0 = 0,
	UNKNOWN_1 = 1,
	HELI_PACK = 2,
	THRUSTER_PACK = 3,
	HYDRO_PACK = 4,
	SONIC_SUMMONER = 5,
	O2_MASK = 6,
	PILOTS_HELMET = 7,
	WRENCH = 8,
	SUCK_CANNON = 9,
	BOMB_GLOVE = 10,
	DEVASTATOR = 11,
	SWINGSHOT = 12,
	VISIBOMB_GUN = 13,
	TAUNTER = 14,
	BLASTER = 15,
	PYROCITOR = 16,
	MINE_GLOVE = 17,
	WALLOPER = 18,
	TESLA_CLAW = 19,
	GLOVE_OF_DOOM = 20,
	MORPH_O_RAY = 21,
	HYDRODISPLACER = 22,
	RYNO = 23,
	DRONE_DEVICE = 24,
	DECOY_GLOVE = 25,
	TRESPASSER = 26,
	METAL_DETECTOR = 27,
	MAGNEBOOTS = 28,
	GRINDBOOTS = 29,
	HOVERBOARD = 30,
	HOLOGUISE = 31,
	GADGETRON_PDA = 32,
	MAP_O_MATIC = 33,
	BOLT_GRABBER = 34,
	PERSUADER = 35,
	UNKNOWN_36 = 36
};

typedef int Ammo[37];
typedef char Unlocks[37];
typedef char Items[37];

#pragma wrench enum Gadget
typedef char PurchasableVendorItems[12];

typedef int QuickSelect[8];
typedef int MaxHitPoints;
typedef bool VisitedPlanets[20];
typedef Level GalacticMap[20];

struct MapLandmark {
	float pos_x;
	float pos_y;
	float rot_z;
	int flags;
};

typedef MapLandmark MapLandmarks[121];

struct HelpDatum {
	short unsigned int timesUsed;
	short unsigned int counter;
	unsigned int unknown;
};

typedef HelpDatum HelpDataMessages[148];
typedef HelpDatum HelpDataGadgets[36];
typedef HelpDatum HelpDataMisc[37];

typedef int LastEquippedGadget;
typedef int Unknown22;
typedef int Unknown23;
typedef int Unknown24;
typedef int CameraElevationDir;
typedef int CameraAzimuthDir;
typedef int CameraRotateSpeed;
typedef bool HelpVoiceOn;
typedef bool HelpTextOn;
typedef bool GoldGadgets[40];
typedef int Unknown31;

struct EquippedGadgets {
	int hand;
	int foot;
	int head;
	int backpack;
	int unknown[3];
};

typedef bool SubtitlesActive;
typedef int Stereo;
typedef int MusicVolume;
typedef int EffectsVolume;
typedef char CheatsEverActivated[12];
typedef int AmmoPurchased[37];
typedef int Unknown1001[37];
typedef int Unknown1002[37];
typedef int TotalPlayTime;
typedef int TotalHitsTaken;
typedef int TotalDeaths;
typedef int Unknown1008;
typedef int TotalBoltsCollected;
typedef unsigned short HelpLog[75];
typedef int HelpLogPos;

// mc_leveldata

enum VisitedEnum {
	UNVISITED = 0,
	VISITED = 1,
	COMPLETED = 2
};

#pragma wrench enum VisitedEnum
typedef char Visited;

typedef char MapMask[2048];
typedef char GoldBolts[4];
typedef char SegmentsCompleted[16];

struct Unknown3006Entry {
	short moby_uid;
	short data;
};

typedef Unknown3006Entry Unknown3006[64];
typedef long TimePerLevel;
typedef char MetalDetector[16];
typedef int BoltsCollectedPerLevel;
typedef int HitsTakenPerLevel;
typedef int DeathsPerLevel;
