#pragma wrench parser on

// mc_gamedata

struct global_flags {
	bool has_zoomerator;
	bool has_raritanium;
	bool has_codebot;
	bool has_premium_nanotech;
	bool has_ultra_nanotech;
	bool unknown[128 - 5];
};

typedef char CheatsActivated[12];
typedef char SkillPoints[12];
typedef int ammo[37];
typedef char unlocks[37];
typedef char items[37];
typedef int galactic_map[20];

struct HelpDatum {
	short unsigned int timesUsed;
	short unsigned int counter;
	unsigned int unknown;
};

typedef HelpDatum HelpDataMessages[148];
typedef HelpDatum HelpDataGadgets[36];
typedef HelpDatum HelpDataMisc[37];
typedef char CheatsEverActivated[12];
typedef short unsigned int HelpLog[75];

// mc_leveldata

typedef char map_mask[2048];
typedef char gold_bolts[4];
typedef char segments_completed[16];
