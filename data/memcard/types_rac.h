#pragma wrench parser on

// mc_gamedata

enum Level
{
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

struct sceCdCLOCK
{
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

enum GlobalFlag
{
	ZOOMERATOR = 0,
	RARITANIUM = 1,
	CODEBOT = 2,
	PREMIUM_NANOTECH = 4,
	ULTRA_NANOTECH = 5,
	MAPOMATIC = 33
};

typedef bool GlobalFlags[128];

enum Cheat
{
	BIG_HEAD_NPCS = 0,
	BIG_HEAD_RATCHET = 1,
	TRIPPY_CONTRAINS = 2,
	BIG_HEAD_CLANK = 3,
	MIRRORED_LEVELS = 4,
	// UNKNOWN_5 = 5,
	INVINCIBILITY = 6,
	BIG_HEAD_ENEMIES = 7,
	// UNKNOWN_8 = 8,
	// UNKNOWN_9 = 9,
	// UNKNOWN_10 = 10,
	// UNKNOWN_11 = 11
};

typedef char CheatsActivated[12];

enum SkillPoint
{
	TAKE_AIM = 0,
	SWING_IT = 1,
	TRANSPORTED = 2,
	STRIKE_A_POSE = 3,
	BLIMPY = 4,
	QWARKTASTIC = 5,
	ANY_TEN = 6,
	TRICKY = 7,
	CLUCK_CLUCK = 8,
	SPEEDY = 9,
	GIRL_TROUBLE = 10,
	JUMPER = 11,
	ACCURACY_COUNTS = 12,
	EAT_LEAD = 13,
	DESTROYED = 14,
	GUNNER = 15,
	SNIPER = 16,
	HEY_OVER_HERE = 17,
	ALIEN_INVASION = 18,
	BURIED_TREASURE = 19,
	PEST_CONTROL = 20,
	WHIRLYBIRDS = 21,
	SITTING_DUCKS = 22,
	SHATTERED_GLASS = 23,
	BLAST_EM = 24,
	HEAVY_TRAFFIC = 25,
	MAGICIAN = 26,
	SNEAKY = 27,
	CAREFUL_CRUISE = 28,
	GOING_COMMANDO = 29
};

typedef char SkillPoints[32];

// Gadget list from: https://creepnt.stream/rc/rccombo.html
enum Gadget
{
	// UNKNOWN_0 = 0,
	// UNKNOWN_1 = 1,
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
	// UNKNOWN_36 = 36
};

typedef int Ammo[37];
typedef char Unlocks[37];
typedef char Items[37];

#pragma wrench enum Gadget
typedef char PurchasableVendorItems[12];

typedef Gadget QuickSelect[8];
typedef int MaxHitPoints;
typedef bool VisitedPlanets[20];
typedef Level GalacticMap[20];

struct MapLandmark
{
	float pos_x;
	float pos_y;
	float rot_z;
	int flags;
};

typedef MapLandmark MapLandmarks[121];

struct HelpDatum
{
	short unsigned int timesUsed;
	short unsigned int counter;
	unsigned int unknown;
};

typedef HelpDatum HelpDataMessages[148];
typedef HelpDatum HelpDataMisc[36];
typedef HelpDatum HelpDataGadgets[37];

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

struct EquippedGadgets
{
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

enum SaveLevel
{
	VELDIN_TUTORIAL_ = 0,
	NOVALIS_ = 1,
	ARIDIA_ = 2,
	KERWAN_ = 3,
	EUDORA_ = 4,
	RILGAR_ = 5,
	NEBULA_G34_ = 6,
	UMBRIS_ = 7,
	BATALIA_ = 8,
	GASPAR_ = 9,
	ORXON_ = 10,
	POKITARU_ = 11,
	HOVEN_ = 12,
	OLTANIS_ORBIT_ = 13,
	OLTANIS_ = 14,
	QUARTU_ = 15,
	KALEBO_III_ = 16,
	VELDIN_ORBIT_ = 17,
	VELDIN_FINALE_ = 18
};

enum VisitedEnum
{
	UNVISITED = 0,
	VISITED = 1,
	COMPLETED = 2
};

#pragma wrench enum VisitedEnum
typedef char Visited;

typedef char MapMask[2048];
typedef char GoldBolts[4];
typedef char SegmentsCompleted[16];

struct Unknown3006Entry
{
	short moby_uid;
	short data;
};

typedef Unknown3006Entry Unknown3006[64];
typedef long TimePerLevel;
typedef char MetalDetector[16];
typedef int BoltsCollectedPerLevel;
typedef int HitsTakenPerLevel;
typedef int DeathsPerLevel;
