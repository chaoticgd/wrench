#pragma wrench parser on

// mc_gamedata

enum Level
{
	ARANOS_TUTORIAL = 0,
	OOZLA = 1,
	MAKTAR_NEBULA = 2,
	ENDAKO = 3,
	BARLOW = 4,
	FELTZIN_SYSTEM = 5,
	NOTAK = 6,
	SIBERIUS = 7,
	TABORA = 8,
	DOBBO = 9,
	HRUGIS_CLOUD = 10,
	JOBA = 11,
	TODANO = 12,
	BOLDAN = 13,
	ARANOS_PRISON = 14,
	GORN = 15,
	SNIVELAK = 16,
	SMOLG = 17,
	DAMOSEL = 18,
	GRELBIN = 19,
	YEEDIL = 20,
	DOBBO_ORBIT = 22,
	DAMOSEL_ORBIT = 23,
	SHIP_SHACK = 24,
	WUPASH_NEBULA = 25,
	JAMMING_ARRAY = 26,
	INSOMNIAC_MUSEUM = 30
};

struct HeroSave
{
	int Bolts;
	char Unknown[60];
};

typedef int BoltCount;
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
	
};

typedef bool GlobalFlags[205];

enum Cheat
{

};

typedef char CheatsActivated[12];

enum SkillPoint
{

};

typedef char SkillPoints[32];

// Gadget list from: https://creepnt.stream/rc/rccombo.html
enum Gadget
{
	
};

typedef int Ammo[56];
typedef char Unlocks[56];
typedef char Items[56];

struct Unknown40
{
	char data[224];
};

#pragma wrench enum Gadget
typedef char PurchasableVendorItems[32];

typedef Gadget QuickSelect[8];
typedef bool VisitedPlanets[28];
typedef Level GalacticMap[28];

struct MapLandmark
{
	float pos_x;
	float pos_y;
	float rot_z;
	int flags;
};

typedef MapLandmark MapLandmarks[114];

typedef char HelpDataMessages[1932];
typedef char HelpDataMisc[516];
typedef char HelpDataGadgets[672];

typedef char Unknown38[40];

#pragma wrench enum Gadget
typedef char GadgetLookup[80];

struct EquippedGadgets
{
	int hand;
	int foot;
	int head;
	int backpack;
	int unknown[3];
};

typedef char CheatsEverActivated[12];

typedef char Unknown41[76];
typedef char Unknown42[56];
typedef char Unknown45[56];
typedef char Unknown43[256];
typedef int Unknown44;
typedef char Unknown46[40];
typedef char Unknown47[256];
typedef int TotalDeaths;
typedef unsigned short HelpLog[85];
typedef int HelpLogPos;

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
typedef char GoldBolts[4];
typedef char SegmentsCompleted[16];

typedef char Unknown3005[1024];
typedef char Unknown3007[12];
typedef int Unknown4002;
typedef char Unknown4005[64];
typedef int Unknown4007;
typedef int Unknown4008;
