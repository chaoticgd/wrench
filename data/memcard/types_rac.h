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
	#pragma wrench bcd
	unsigned char second;
	#pragma wrench bcd
	unsigned char minute;
	#pragma wrench bcd
	unsigned char hour;
	unsigned char pad;
	#pragma wrench bcd
	unsigned char day;
	#pragma wrench bcd
	unsigned char month;
	#pragma wrench bcd
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
	ACTORS_HAVE_OVERSIZED_CRANIUMS = 0,
	RATCHET_HAS_A_BIG_HEAD = 1,
	TRIPPY_CONTRAINS = 2,
	CLANK_HAS_A_LARGE_NOGGIN = 3,
	LEVELS_ARE_MIRRORED = 4,
	// UNKNOWN_5 = 5,
	HEALTH_GIVES_INVINCIBILITY_AT_MAX = 6,
	ENEMIES_HAVE_MASSIVE_DOMES = 7,
	// UNKNOWN_8 = 8,
	// UNKNOWN_9 = 9,
	// UNKNOWN_10 = 10,
	// UNKNOWN_11 = 11
};

typedef bool CheatsActivated[12];

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

typedef bool SkillPoints[32];

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
typedef bool Unlocks[37];
typedef bool Items[37];

#pragma wrench enum Gadget
typedef char PurchasableVendorItems[12];

typedef Gadget QuickSelect[8];
typedef int MaxHitPoints;
typedef bool VisitedPlanets[20];
typedef Level GalacticMap[20];

struct MapLandmark
{
	float PosX;
	float PosY;
	float RotZ;
	int Flags;
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
typedef bool WrenchEquipped;
typedef int ThrusterPackEquippedLast;
typedef int Unused24;
typedef int CameraElevationDir;
typedef int CameraAzimuthDir;
typedef int CameraRotateSpeed;
typedef bool HelpVoiceOn;
typedef bool HelpTextOn;
typedef bool GoldGadgets[40];
typedef int Unknown31;

struct EquippedGadgets
{
	Gadget Hand;
	Gadget Foot;
	Gadget Head;
	Gadget Back;
	Gadget Unknown[3];
};

typedef bool SubtitlesActive;
typedef int Stereo;
typedef int MusicVolume;
typedef int EffectsVolume;
typedef bool CheatsEverActivated[12];
typedef int AmmoPurchased[37];
typedef int AmmoPickedUp[37];
typedef int AmmoUsed[37];
typedef int TotalPlayTime;
typedef int TotalHitsTaken;
typedef int TotalDeaths;
typedef int Unknown1008;
typedef int TotalBoltsCollected;
typedef unsigned char HelpLog[150];
typedef int HelpLogPos;

// mc_leveldata

enum SaveLevel
{
	_VELDIN_TUTORIAL = 0,
	_NOVALIS = 1,
	_ARIDIA = 2,
	_KERWAN = 3,
	_EUDORA = 4,
	_RILGAR = 5,
	_NEBULA_G34 = 6,
	_UMBRIS = 7,
	_BATALIA = 8,
	_GASPAR = 9,
	_ORXON = 10,
	_POKITARU = 11,
	_HOVEN = 12,
	_OLTANIS_ORBIT = 13,
	_OLTANIS = 14,
	_QUARTU = 15,
	_KALEBO_III = 16,
	_VELDIN_ORBIT = 17,
	_VELDIN_FINALE = 18
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
