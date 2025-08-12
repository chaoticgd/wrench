#pragma wrench parser on

// mc_gamedata

enum Level
{
	MULTIPLAYER_MENU = 0,
	DREADZONE_STATION = 1,
	CATACROM_GRAVEYARD = 2,
	SARATHOS_SWAMP = 4,
	DARK_CATHEDRAL = 5,
	TEMPLE_OF_SHAAR = 6,
	VALIX_LIGHTHOUSE = 7,
	MINING_FACILITY = 8,
	TORVAL_RUINS = 10,
	TEMPUS_STATION = 11,
	MARAXUS_PRISON = 13,
	GHOST_STATION = 14,
	CONTROL_LEVEL = 15,
	DREADZONE_STATION_SPLIT_SCREEN = 21,
	CATACROM_GRAVEYARD_SPLIT_SCREEN = 22,
	SARATHOS_SWAMP_SPLIT_SCREEN = 24,
	DARK_CATHEDRAL_SPLIT_SCREEN = 25,
	TEMPLE_OF_SHAAR_SPLIT_SCREEN = 26,
	VALIX_LIGHTHOUSE_SPLIT_SCREEN = 27,
	MINING_FACILITY_SPLIT_SCREEN = 28,
	TORVAL_RUINS_SPLIT_SCREEN = 30,
	TEMPUS_STATION_SPLIT_SCREEN = 31,
	MARAXUS_PRISON_SPLIT_SCREEN = 33,
	GHOST_STATION_SPLIT_SCREEN = 34,
	CONTROL_LEVEL_SPLIT_SCREEN = 35,
	MULTIPLAYER_BATTLEDOME_TOWER = 41,
	MULTIPLAYER_CATACROM_GRAVEYARD = 42,
	MULTIPLAYER_SARATHOS_SWAMP = 44,
	MULTIPLAYER_DARK_CATHEDRAL = 45,
	MULTIPLAYER_TEMPLE_OF_SHAAR = 46,
	MULTIPLAYER_VALIX_LIGHTHOUSE = 47,
	MULTIPLAYER_MINING_FACILITY = 48,
	MULTIPLAYER_TORVAL_RUINS = 50,
	MULTIPLAYER_TEMPUS_STATION = 51,
	MULTIPLAYER_MARAXUS_PRISON = 53,
	MULTIPLAYER_GHOST_STATION = 54,
	MULTIPLAYER_BATTLEDOME_TOWER_SPLIT_SCREEN = 61,
	MULTIPLAYER_CATACROM_GRAVEYARD_SPLIT_SCREEN = 62,
	MULTIPLAYER_SARATHOS_SWAMP_SPLIT_SCREEN = 64,
	MULTIPLAYER_DARK_CATHEDRAL_SPLIT_SCREEN = 65,
	MULTIPLAYER_TEMPLE_OF_SHAAR_SPLIT_SCREEN = 66,
	MULTIPLAYER_VALIX_LIGHTHOUSE_SPLIT_SCREEN = 67,
	MULTIPLAYER_MINING_FACILITY_SPLIT_SCREEN = 68,
	MULTIPLAYER_TORVAL_RUINS_SPLIT_SCREEN = 70,
	MULTIPLAYER_TEMPUS_STATION_SPLIT_SCREEN = 71,
	MULTIPLAYER_MARAXUS_PRISON_SPLIT_SCREEN = 73,
	MULTIPLAYER_GHOST_STATION_SPLIT_SCREEN = 74
};

struct HeroSave
{
	int bolts;
	int boltDeficit;
	int xp;
	int points;
	short int HeroMaxHp;
	short int ArmorLevel;
	float limitBreak;
	int purchasedSkins;
	short int spentDiffStars;
	char boltMultLevel;
	char boltMultSubLevel;
	char OldGameSaveData;
	char blueBadges;
	char redBadges;
	char greenBadges;
	char goldBadges;
	char blackBadges;
	char Completes;
	char lastEquippedGadget[2];
	char tempWeapons[4];
	int currentMaxLimitBreak;
	short int ArmorLevel2;
	short int progressionArmorLevel;
	int startLimitBreakDiff;
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
typedef unsigned int TotalPlayTime;
typedef int TN_savedDifficultyLevel;

enum GlobalFlag
{
	
};

typedef char GlobalFlags[12];

enum Cheat
{
	
};

typedef bool CheatsActivated[14];
typedef bool CheatsEverActivated[14];

enum SkillPoint
{
	
};

typedef int SkillPoints[15];

struct vec3
{
	float x;
	float y;
	float z;
};

struct alignas(16) vec4
{
	float x;
	float y;
	float z;
	float w;
};

struct tNW_GadgetEventMessage
{
	short int gadgetId;
	char playerIndex;
	char gadgetEventType;
	char extraData;
	int activeTime;
	unsigned int targetUID;
	vec3 firingLoc;
	vec3 targetDir;
};

struct GadgetEvent
{
	unsigned char gadgetID;
	unsigned char cPlayerIndex;
	char cGadgetType;
	char gadgetEventType;
	int iActiveTime;
	unsigned int targetUID;
	vec4 targetOffsetQuat;
	GadgetEvent *pNextGadgetEvent;
	tNW_GadgetEventMessage gadgetEventMsg;
};

enum eModBasicType
{
	MOD_BSC_UNDEFINED = 0,
	MOD_BSC_SPEED = 1,
	MOD_BSC_AMMO = 2,
	MOD_BSC_AIMING = 3,
	MOD_BSC_IMPACT = 4,
	MOD_BSC_AREA = 5,
	MOD_BSC_XP = 6,
	MOD_BSC_JACKPOT = 7,
	MOD_BSC_NANOLEECH = 8,
	TOTAL_BASIC_MODS = 9,
	TOTAL_MOD_BSC_DEFS_SIZE = 9
};

enum eModPostFXType
{
	MOD_PFX_UNDEFINED = 0,
	MOD_PFX_NAPALM = 1,
	MOD_PFX_ELECTRICDOOM = 2,
	MOD_PFX_FREEZING = 3,
	MOD_PFX_BOMBLETS = 4,
	MOD_PFX_MORPHING = 5,
	MOD_PFX_INFECTION = 6,
	MOD_PFX_PLAGUE = 7,
	MOD_PFX_SHOCK = 8,
	TOTAL_POSTFX_MODS = 9,
	TOTAL_MOD_PFX_DEFS_SIZE = 9
};

enum eModWeaponType
{
	MOD_WPN_UNDEFINED = 0,
	MOD_WPN_ROCKET_GUIDANCE = 1,
	MOD_WPN_SHOTGUN_WIDTH = 2,
	MOD_WPN_SHOTGUN_LENGTH = 3,
	MOD_WPN_GRENADE_MANUALDET = 4,
	MOD_WPN_MACHGUN_BEAM = 5,
	MOD_WPN_SNIPER_PIERCING = 6,
	TOTAL_WPN_MODS = 7,
	TOTAL_MOD_WPN_DEFS_SIZE = 7
};

struct ModBasicEntry
{
	eModBasicType ID;
};

struct ModPostFXEntry
{
	eModPostFXType ID;
};

struct ModWeaponEntry
{
	eModWeaponType ID;
};

struct GadgetEntry
{
	short int level;
	short int sAmmo;
	unsigned int sXP;
	int iActionFrame;
	ModPostFXEntry modActivePostFX;
	ModWeaponEntry modActiveWeapon;
	ModBasicEntry modActiveBasic[10];
	ModWeaponEntry modWeapon[2];
};

struct GadgetBox
{
	char initialized;
	char level;
	char bButtonDown[10];
	short int sButtonUpFrames[10];
	char cNumGadgetEvents;
	char modBasic[8];
	short int modPostFX;
	GadgetEvent *pNextGadgetEvent;
	GadgetEvent gadgetEventSlots[32];
	GadgetEntry gadgets[32];
};

typedef GadgetBox g_HeroGadgetBox[10];

struct HelpDatum
{
	short unsigned int timesUsed;
	short unsigned int counter;
	unsigned int lastTime;
	unsigned int level_die;
};

typedef HelpDatum HelpDataMessages[2088];
typedef HelpDatum HelpDataGadgets[20];
typedef HelpDatum HelpDataMisc[16];

struct GameSettings
{
	int PalMode;
	char HelpVoiceOn;
	char HelpTextOn;
	char SubtitlesActive;
	int Stereo;
	int MusicVolume;
	int EffectsVolume;
	int VoVolume;
	int CameraElevationDir[3][4];
	int CameraAzimuthDir[3][4];
	int CameraRotateSpeed[3][4];
	unsigned char FirstPersonModeOn[10];
	char _was_NTSCProgessive;
	char Wide;
	char ControllerVibrationOn[8];
	char QuickSelectPauseOn;
	char Language;
	char AuxSetting2;
	char AuxSetting3;
	char AuxSetting4;
	char AutoSaveOn;
};

typedef GameSettings Settings;

typedef int FirstPersonDesiredMode[10];

enum Movie
{
	
};

typedef unsigned int MoviesPlayedRecord[64];
typedef int TotalDeaths;
typedef short unsigned int HelpLog[2100];
typedef int HelpLogPos;
typedef unsigned char g_PurchaseableGadgets[20];
typedef unsigned char g_PurchaseableBotUpgrades[17];
typedef unsigned char g_PurchaseableWrenchLevel;
typedef unsigned char g_PurchaseablePostFXMods[9];

struct BotSave
{
	char botUpgrades[17];
	char botPaintjobs[11];
	char botHeads[8];
	char curBotPaintJob[2];
	char curBotHead[2];
};

struct ST_PlayerData
{
	unsigned int healthReceived;
	unsigned int damageReceived;
	unsigned int ammoReceived;
	unsigned int timeChargeBooting;
	unsigned int numDeaths;
	unsigned int weaponKills[20];
	float weaponKillPercentage[20];
	unsigned int ammoUsed[20];
	unsigned int shotsThatHit[20];
	unsigned int shotsThatMiss[20];
	float shotAccuracy[20];
	unsigned int funcModKills[9];
	unsigned int funcModUsed[9];
	unsigned int timeSpentInVehicles[4];
	unsigned int killsWithVehicleWeaps[4];
	unsigned int killsFromVehicleSquashing[4];
	unsigned int killsWhileInVehicle;
	unsigned int vehicleShotsThatHit[4];
	unsigned int vehicleShotsThatMiss[4];
	float vehicleShotAccuracy[4];
};

typedef ST_PlayerData ST_PlayerStatistics[2];
typedef int ST_BattledomeWinsAndLosses[2];

struct ST_EnemyKillInfo
{
	int oClass;
	int kills;
};

typedef ST_EnemyKillInfo ST_EnemyKills[30];
typedef int g_QuickSwitchGadgets[4][3];

// mc_leveldata

struct MF_MissionSave
{
	int xp;
	int bolts;
	unsigned char status;
	unsigned char completes;
	unsigned char difficulty;
};

struct MF_LevelSave
{
	MF_MissionSave mission[64];
	unsigned char status;
	unsigned char jackpot;
};

typedef MF_LevelSave mf_levelSaveData;

// mc_netdata

struct timeDeathMatch
{
	int timeLimit;
	bool vehiclesOn;
	bool friendlyFireOn;
	bool suicideOn;
};

struct fragDeathMatch
{
	int fragLimit;
	bool vechiclesOn;
	bool suicideOn;
	bool friendlyFireOn;
};

struct siegeMatch
{
	int timeLimit;
	bool nodesOn;
	bool aisOn;
	bool vehiclesOn;
	bool friendlyfireOn;
};

struct gameModeStruct
{
	int modeChosen;
	siegeMatch siegeOptions;
	timeDeathMatch timeDeathMatchOptions;
	fragDeathMatch fragDeathMatchOptions;
};

typedef gameModeStruct gameModeOptions;

struct generalStatStruct
{
	int noofGamesPlayed;
	int noofGamesWon;
	int noofGamesLost;
	int noofKills;
	int noofDeaths;
};

struct siegeMatchStatStruct
{
	int noofWins;
	int noofLosses;
	int winsPerLevel[6];
	int lossesPerLevel[6];
	int noofBaseCaptures;
	int noofKills;
	int noofDeaths;
};

struct deadMatchStatStruct
{
	int noofWins;
	int noofLosses;
	int winsPerLevel[6];
	int lossesPerLevel[6];
	int noofkills;
	int noofDeaths;
};

struct cameraMode
{
	bool normalLeftRightMode;
	bool normalUpDownMode;
	int cameraSpeed;
};

struct profileStruct
{
	int skin;
	cameraMode camerOptions[3];
	unsigned char FirstPersonModeOn;
	char name[16];
	char password[16];
	char mapAccess;
	char palServer;
	char helpMsgOff;
	char savePassword;
	char locationIdx;
	generalStatStruct generalStats;
	siegeMatchStatStruct siegeMatchStats;
	deadMatchStatStruct deadMatchStats;
	char active;
	int help_data[32];
	char netEnabled;
	char vibration;
	short int musicVolume;
	int extraDataPadding[31];
};

typedef profileStruct mpProfiles[8];
