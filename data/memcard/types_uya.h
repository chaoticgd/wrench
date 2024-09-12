#pragma wrench parser on

// mc_gamedata

enum Level
{
	VELDIN = 1,
	FLORANA = 2,
	STARSHIP_PHOENIX = 3,
	MARCADIA = 4,
	DAXX = 5,
	STARSHIP_PHOENIX_UNDER_ATTACK = 6,
	ANNIHILATION_NATION = 7,
	AQUATOS = 8,
	TYHRRANOSIS = 9,
	ZELDRIN_STARPORT = 10,
	OBANI_MOONS = 11,
	RILGAR = 12,
	HOLOSTAR_STUDIOS_RATCHET = 13,
	KOROS = 14,
	KERWAN = 16,
	CRASH_SITE = 17,
	ARIDIA = 18,
	THRAN_ASTEROID_BELT = 19,
	FINAL_BOSS = 20,
	OBANI_DRACO = 21,
	MYLON = 22,
	HOLOSTAR_STUDIOS_CLANK = 23,
	INSOMNIAC_MUSEUM = 24,
	KERWAN_RANGER_MISSIONS = 26,
	AQUATOS_BASE = 27,
	AQUATOS_SEWERS = 28,
	TYHRRANOSIS_RANGER_MISSIONS = 29,
	VID_COMIC_UNNAMED = 30,
	VID_COMIC_1 = 31,
	VID_COMIC_4 = 32,
	VID_COMIC_2 = 33,
	VID_COMIC_3 = 34,
	VID_COMIC_5 = 35,
	VID_COMIC_1_SPECIAL_EDITION = 36,
	MULTIPLAYER_MENU = 39,
	MULTIPLAYER_BAKISI_ISLES = 40,
	MULTIPLAYER_HOVEN_GORGE = 41,
	MULTIPLAYER_OUTPOST_X12 = 42,
	MULTIPLAYER_KORGON_OUTPOST = 43,
	MULTIPLAYER_METROPOLIS = 44,
	MULTIPLAYER_BLACKWATER_CITY = 45,
	MULTIPLAYER_COMMAND_CENTER = 46,
	MULTIPLAYER_BLACKWATER_DOCKS = 47,
	MULTIPLAYER_AQUATOS_SEWERS = 48,
	MULTIPLAYER_MARCADIA_PALACE = 49,
	MULTIPLAYER_BAKISI_ISLES_SPLIT_SCREEN = 50,
	MULTIPLAYER_HOVEN_GORGE_SPLIT_SCREEN = 51,
	MULTIPLAYER_OUTPOST_X12_SPLIT_SCREEN = 52,
	MULTIPLAYER_KORGON_OUTPOST_SPLIT_SCREEN = 53,
	MULTIPLAYER_METROPOLIS_SPLIT_SCREEN = 54,
	MULTIPLAYER_BLACKWATER_CITY_SPLIT_SCREEN = 55
};

struct HeroSave
{
	int Bolts;
	char Unknown[124];
};

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
typedef int TotalPlayTime;

enum GlobalFlag
{
	
};

typedef bool GlobalFlags[272];

enum Cheat
{

};

typedef char CheatsActivated[19];

enum SkillPoint
{

};

typedef char SkillPoints[32];

// Gadget list from: https://creepnt.stream/rc/rccombo.html
enum Gadget
{
	UNKNOWN_0 = 0,
	UNKNOWN_1 = 1,
	HELI_PACK = 2,
	THRUSTER_PACK = 3,
	HYDRO_PACK = 4,
	MAP_O_MATIC = 5,
	UNKNOWN_6 = 6,
	BOLT_GRABBER_V2 = 7,
	UNKNOWN_8 = 8,
	UNKNOWN_9 = 9,
	UNKNOWN_10 = 10,
	HYPERSHOT = 11,
	UNKNOWN_12 = 12,
	GRAVITY_BOOTS = 13,
	UNKNOWN_14 = 14,
	UNKNOWN_15 = 15,
	PLASMA_COIL = 16,
	LAVA_GUN = 17,
	REFRACTOR = 18,
	BOUNCER = 19,
	THE_HACKER = 20,
	MINITURRET = 21,
	SHIELD_CHARGER = 22,
	UNKNOWN_23 = 23,
	UNKNOWN_24 = 24,
	UNKNOWN_25 = 25,
	BOX_BREAKER = 26,
	UNKNOWN_27 = 27,
	UNKNOWN_28 = 28,
	CHARGE_BOOTS = 29,
	TYHRRAGUISE = 30,
	WARP_PAD = 31,
	NANO_PAK = 32,
	STAR_MAP = 33,
	MASTER_PLAN = 34,
	PDA = 35,
	UNKNOWN_36 = 36,
	UNKNOWN_37 = 37,
	UNKNOWN_38 = 38,
	SHOCK_BLASTER = 39,
	UNKNOWN_40 = 40,
	UNKNOWN_41 = 41,
	UNKNOWN_42 = 42,
	UNKNOWN_43 = 43,
	UNKNOWN_44 = 44,
	UNKNOWN_45 = 45,
	UNKNOWN_46 = 46,
	N60_STORM = 47,
	UNKNOWN_48 = 48,
	UNKNOWN_49 = 49,
	UNKNOWN_50 = 50,
	UNKNOWN_51 = 51,
	UNKNOWN_52 = 52,
	UNKNOWN_53 = 53,
	UNKNOWN_54 = 54,
	INFECTOR = 55,
	UNKNOWN_56 = 56,
	UNKNOWN_57 = 57,
	UNKNOWN_58 = 58,
	UNKNOWN_59 = 59,
	UNKNOWN_60 = 60,
	UNKNOWN_61 = 61,
	UNKNOWN_62 = 62,
	ANNIHILATOR = 63,
	UNKNOWN_64 = 64,
	UNKNOWN_65 = 65,
	UNKNOWN_66 = 66,
	UNKNOWN_67 = 67,
	UNKNOWN_68 = 68,
	UNKNOWN_69 = 69,
	SPITTINGHYDRA = 70,
	UNKNOWN_71 = 71,
	UNKNOWN_72 = 72,
	UNKNOWN_73 = 73,
	UNKNOWN_74 = 74,
	UNKNOWN_75 = 75,
	UNKNOWN_76 = 76,
	UNKNOWN_77 = 77,
	DISC_BLAE_GUN = 78,
	UNKNOWN_79 = 79,
	UNKNOWN_80 = 80,
	UNKNOWN_81 = 81,
	UNKNOWN_82 = 82,
	UNKNOWN_83 = 83,
	UNKNOWN_84 = 84,
	UNKNOWN_85 = 85,
	AGENTS_OF_DOOM = 86,
	UNKNOWN_87 = 87,
	UNKNOWN_88 = 88,
	UNKNOWN_89 = 89,
	UNKNOWN_90 = 90,
	UNKNOWN_91 = 91,
	UNKNOWN_92 = 92,
	UNKNOWN_93 = 93,
	RIFT_INDUCER = 94,
	UNKNOWN_95 = 95,
	UNKNOWN_96 = 96,
	UNKNOWN_97 = 97,
	UNKNOWN_98 = 98,
	UNKNOWN_99 = 99,
	UNKNOWN_100 = 100,
	UNKNOWN_101 = 101,
	HOLOSHIELD = 102,
	UNKNOWN_103 = 103,
	UNKNOWN_104 = 104,
	UNKNOWN_105 = 105,
	UNKNOWN_106 = 106,
	UNKNOWN_107 = 107,
	UNKNOWN_108 = 108,
	UNKNOWN_109 = 109,
	FLUX_RIFLE = 110,
	UNKNOWN_111 = 111,
	UNKNOWN_112 = 112,
	UNKNOWN_113 = 113,
	UNKNOWN_114 = 114,
	UNKNOWN_115 = 115,
	UNKNOWN_116 = 116,
	UNKNOWN_117 = 117,
	NITRO_LAUNCHER = 118,
	UNKNOWN_119 = 119,
	UNKNOWN_120 = 120,
	UNKNOWN_121 = 121,
	UNKNOWN_122 = 122,
	UNKNOWN_123 = 123,
	UNKNOWN_124 = 124,
	UNKNOWN_125 = 125,
	PLASMA_WHIP = 126,
	UNKNOWN_127 = 127,
	UNKNOWN_128 = 128,
	UNKNOWN_129 = 129,
	UNKNOWN_130 = 130,
	UNKNOWN_131 = 131,
	UNKNOWN_132 = 132,
	UNKNOWN_133 = 133,
	SUCK_CANNON = 134,
	UNKNOWN_135 = 135,
	UNKNOWN_136 = 136,
	UNKNOWN_137 = 137,
	UNKNOWN_138 = 138,
	UNKNOWN_139 = 139,
	UNKNOWN_140 = 140,
	UNKNOWN_141 = 141,
	QUACK_O_RAY = 142
};

typedef int Ammo[156];
typedef char Unlocks[156];
typedef char Items[156];
typedef int Unknown40[156];

#pragma wrench enum Gadget
typedef char PurchasableVendorItems[64];

typedef Gadget QuickSelect[8];
typedef bool VisitedPlanets[60];
typedef Level GalacticMap[60];

struct MapLandmark
{
	float pos_x;
	float pos_y;
	float rot_z;
	int flags;
};

typedef MapLandmark MapLandmarks[114];

typedef char HelpDataMessages[4272];
typedef char HelpDataMisc[516];
typedef char HelpDataGadgets[1872];

typedef char Unknown38[188];

#pragma wrench enum Gadget
typedef char GadgetLookup[160];

struct EquippedGadgets
{
	int hand;
	int foot;
	int head;
	int backpack;
	int unknown[3];
};

typedef char CheatsEverActivated[19];

typedef char Unknown41[268];
typedef char Unknown42[156];
typedef char Unknown45[156];
typedef char Unknown43[256];
typedef int Unknown44;
typedef char Unknown46[40];
typedef char Unknown47[256];
typedef int TotalDeaths;
typedef unsigned short HelpLog[400];
typedef int HelpLogPos;
typedef Gadget SecondaryQuickSelect[8];
typedef char Unknown7003[12];
typedef char Unknown7004[6];
typedef char Unknown7005[192];
typedef char Unknown7006[192];
typedef char Unknown7007[192];

// mc_leveldata

enum SaveLevel
{
	
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
typedef char GoldBolts[8];
typedef char SegmentsCompleted[16];

typedef char Unknown3005[1024];
typedef int Unknown4002;
typedef char Unknown4005[64];
typedef int Unknown4007;
typedef int Unknown4008;
