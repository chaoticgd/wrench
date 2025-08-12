#pragma wrench parser on

// mc_gamedata

// Note: The Insomniac Museum is at a different position compared to the normal
// level numbers.
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
	INSOMNIAC_MUSEUM = 21,
	DOBBO_ORBIT = 22,
	DAMOSEL_ORBIT = 23,
	SHIP_SHACK = 24,
	WUPASH_NEBULA = 25,
	JAMMING_ARRAY = 26
};

// Gadget list from: https://creepnt.stream/rc/rccombo.html
enum Gadget
{
	// UNKNOWN_0 = 0,
	// UNKNOWN_1 = 1,
	HELI_PACK = 2,
	THRUSTER_PACK = 3,
	HYDRO_PACK = 4,
	MAPPER = 5,
	COMMANDO_SUIT = 6,
	BOLT_GRABBER = 7,
	LEVITATOR = 8,
	CLANK_ZAPPER = 9,
	WRENCH = 10,
	// UNKNOWN_11 = 11,
	BOMB_GLOVE = 12,
	SWINGSHOT = 13,
	VISIBOMB = 14,
	// UNKNOWN_15 = 15,
	SHEEPINATOR = 16,
	DECOY_GLOVE = 17,
	TESLA_CLAW = 18,
	GRAVITY_BOOTS = 19,
	GRINDBOOTS = 20,
	GLIDER = 21,
	CHOPPER = 22,
	PULSE_RIFLE = 23,
	SEEKER_GUN = 24,
	HOVERBOMB_GUN = 25,
	BLITZ_GUN = 26,
	MINIROCKET_TUBE = 27,
	PLASMA_COIL = 28,
	LAVA_GUN = 29,
	LANCER = 30,
	SYNTHENOID = 31,
	SPIDERBOT = 32,
	// UNKNOWN_33 = 33,
	// UNKNOWN_34 = 34,
	// UNKNOWN_35 = 35,
	DYNAMO = 36,
	BOUNCER = 37,
	ELECTROLYZER = 38,
	THERMANATOR = 39,
	// UNKNOWN_40 = 40,
	MINITURRET_GLOVE = 41,
	GRAVITY_BOMB = 42,
	ZODIAC = 43,
	RYNO = 44,
	SHIELD_CHARGER = 45,
	TRACTOR_BEAM = 46,
	// UNKNOWN_47 = 47,
	BIKER_HELMET = 48,
	QWARK_STATUETTE = 49,
	BOX_BREAKER = 50,
	INFILTRATOR = 51,
	// UNKNOWN_52 = 52,
	WALLOPER = 53,
	CHARGE_BOOTS = 54,
	HYPNOMATIC = 55
};

struct HeroSave
{
	int Bolts;
	int Raritanium;
	int Health;
	Gadget HandGadget;
	short FootGadget;
	short F_12;
	int XP;
	int F_18;
	int F_1C;
	short PlatinumBoltsFound;
	short CrystalsFound;
	int MoonstonesFound;
	int F_28;
	int F_2C;
	char Unknown[16];
};

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
typedef int TotalPlayTime;

enum GlobalFlag
{
	
};

typedef bool GlobalFlags[205];

enum Cheat
{
	ACTORS_HAVE_OVERSIZED_CRANIUMS = 0,
	RATCHET_HAS_A_BIG_HEAD = 1,
	SNOW_DUDE = 2,
	CLANK_HAS_A_LARGE_NOGGIN = 3,
	LEVELS_ARE_MIRRORED = 4,
	// UNKNOWN_5 = 5,
	// UNKNOWN_6 = 6,
	ENEMIES_HAVE_MASSIVE_DOMES = 7,
	// UNKNOWN_8 = 8,
	RATCHET_SHOWS_HIS_FUNNY_SIDE = 9,
	RATCHET_IN_A_TUX = 10,
	BEACH_BOY = 11
};

typedef bool CheatsActivated[12];

enum SkillPoint
{
	THATS_IMPOSSIBLE = 0,
	WRENCH_NINJA_BLADE_TO_BLADE = 1,
	SPEED_DEMON = 2,
	HOW_FAST_WAS_THAT = 3,
	NO_SHOCKING_DEVELOPMENTS = 4,
	HEAL_YOUR_CHI = 5,
	BE_A_MOON_CHILD = 6,
	MIDTOWN_INSANITY = 7,
	DUKES_UP = 8,
	NOTHING_TO_SEE_HERE = 9,
	YOURE_MY_HERO = 10,
	MOVING_VIOLATION = 11,
	OLD_SKOOL = 12,
	PREHISTORIC_RAMPAGE = 13,
	VANDALIZE = 14,
	SMASH_AND_GRAB = 15,
	YOU_CAN_BREAK_A_SNOW_DAN = 16,
	PLANET_BUSTER = 17,
	WRENCH_NINJA_II_MASSACRE = 18,
	_2B_OR_NOT_2B_HIT = 19,
	BYE_BYE_BIRDIES = 20,
	DESTROY_ALL_BREAKABLES = 21,
	TRY_TO_SLEEP = 22,
	NANO_TO_THE_MAX = 23,
	ROBO_RAMPAGE = 24,
	CLANK_NEEDS_A_NEW_PAIR_OF_SHOES = 25,
	WEAPON_ENVY = 26,
	SAFETY_DEPOSIT = 27,
	OPERATE_HEAVY_MACHINERY = 28,
	NICE_RIDE = 29
};

typedef bool SkillPoints[32];

typedef int Ammo[56];
typedef bool Unlocks[56];
typedef bool Items[56];
typedef int GadgetXP[56];

#pragma wrench enum Gadget
typedef char PurchasableVendorItems[32];

typedef Gadget QuickSelect[8];

#pragma wrench elementnames Level
typedef bool VisitedPlanets[28];
typedef Level GalacticMap[28];

struct MapLandmark
{
	float PosX;
	float PosY;
	float RotZ;
	int Flags;
};

typedef MapLandmark MapLandmarks[114];

typedef char HelpDataMessages[1932];
typedef char HelpDataMisc[516];
typedef char HelpDataGadgets[672];

struct Settings
{
	char unknown[40];
};

#pragma wrench enum Gadget
typedef char GadgetLookup[80];

struct EquippedGadgets
{
	Gadget Hand;
	Gadget Foot;
	Gadget Head;
	Gadget Back;
	Gadget Unknown[3];
};

typedef bool CheatsEverActivated[12];
typedef int EnemiesKilled[19];
typedef int ChallengeCompletes[19];
typedef char GadgetMods[56];
typedef char Gadget45[56];

enum Movie
{
	
};

typedef char MoviesPlayedRecord[256];

enum ShipNose
{
	STANDARD_NOSE = 0,
	SPLIT_NOSE = 1,
	SCOOP_NOSE = 2
};

enum ShipWings
{
	STANDARD_WINGS = 0,
	HIGH_LIFT_WINGS = 1,
	HEAVY_ORDINANCE_WINGS = 2
};

enum ShipUpgrades
{
	TRIPLE_BOOST_ACCELERATION_ENGINE = 1,
	FUSION_LASER_CANNONS = 4,
	ELECTRO_MINE_LAUNCHER = 8,
	MEGA_MINE_LAUNCHER = 16,
	FAST_LOCK_MISSILE_LAUNCHER = 32,
	TORPEDO_LAUNCHER = 64,
	MULTI_TORPEDO_LAUNCHER = 128,
	NUCLEAR_DETONATION_DEVICE = 256,
	HYPERSPACE_WARP_SYSTEM = 512,
	ADVANCED_SHIELDING_SYSTEM = 1024
};

enum ShipPaintJob
{
	BLARGIAN_RED = 0,
	ORXON_GREEN = 1,
	BOGON_BLUE = 2,
	INSOMNIAC_SPECIAL = 3,
	DARK_NEBULA = 4,
	DREKS_BLACK_HEART = 5,
	SPACE_STORM = 6,
	LUNAR_ECLIPSE = 7,
	PLAIDTASTIC = 8,
	SUPERNOVA = 9,
	SOLAR_WIND = 10,
	CLOWNER = 11,
	SILENT_STRIKE = 12,
	PREPSTER = 13,
	LOMBAX_ORANGE = 14,
	NEUTRON_STAR = 15,
	STAR_TRAVELLER = 16
};

struct ShipMods
{
	#pragma wrench enum ShipNose
	unsigned short Nose : 2;
	#pragma wrench enum ShipWings
	unsigned short Wings : 2;
	#pragma wrench bitflags ShipUpgrades
	unsigned short Upgrades : 12;
	#pragma wrench enum ShipPaintJob
	unsigned short PaintJob;
};

typedef char Unknown46[40];
typedef char Unknown47[256];
typedef int TotalDeaths;
typedef unsigned char HelpLog[170];
typedef int HelpLogPos;

// mc_leveldata

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
typedef int DeathsPerLevel;
typedef char Unknown4005[64];
typedef int Unknown4007;
typedef int Unknown4008;
