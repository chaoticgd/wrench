#pragma wrench parser on

// mc_gamedata

typedef int Level;
typedef int BoltCount;

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

enum GlobalFlagsEnum {
	ZOOMERATOR = 0,
	RARITANIUM = 1,
	CODEBOT = 2,
	PREMIUM_NANOTECH = 3,
	ULTRA_NANOTECH = 4,
	MAPOMATIC = 33
};

#pragma wrench elementnames GlobalFlagsEnum
typedef bool GlobalFlags[128];

struct equipped_gadgets {
	int hand;
	int foot;
	int head;
	int backpack;
	int unknown[3];
};

typedef char CheatsActivated[12];
typedef int ammo_purchased[37];
typedef char SkillPoints[12];
typedef int ammo[37];
typedef char unlocks[37];
typedef char items[37];
typedef int quick_select[8];
typedef int galactic_map[20];

struct HelpDatum {
	short unsigned int timesUsed;
	short unsigned int counter;
	unsigned int unknown;
};

struct map_landmark {
	float pos_x;
	float pos_y;
	float rot_z;
	int flags;
};

typedef HelpDatum HelpDataMessages[148];
typedef HelpDatum HelpDataGadgets[36];
typedef HelpDatum HelpDataMisc[37];
typedef char CheatsEverActivated[12];
typedef short unsigned int HelpLog[75];

// mc_leveldata

typedef char map_mask[2048];
typedef char gold_bolts[4];
typedef char SegmentsCompleted[16];

struct unknown_3006_item {
	short moby_uid;
	short data;
};

typedef unknown_3006_item unknown_3006[64];
