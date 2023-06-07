// wrench parser on

struct TargetVars;
struct npcVars;
struct TrackVars;
struct SuckVars;
struct ReactVars;
struct ArmorVars;
struct MoveVars;
struct BogeyVars;
struct ScriptVars;
struct TransportVars;
struct EffectorVars;
struct CommandVars;
struct RoleVars;
struct FlashVars;
struct MoveVars_V2;
struct NavigationVars;
struct ObjectiveVars;

typedef TargetVars *tVarPtr;
typedef npcVars *nVarPtr;
typedef TrackVars *trVarPtr;
typedef SuckVars *sVarPtr;
typedef ReactVars *rVarPtr;
typedef ArmorVars *aVarPtr;
typedef MoveVars *mVarPtr;
typedef BogeyVars *bVarPtr;
typedef ScriptVars *scVarPtr;
typedef TransportVars *transVarPtr;
typedef EffectorVars *eVarPtr;
typedef CommandVars *cmdVarPtr;
typedef RoleVars *roleVarPtr;
typedef FlashVars *flashVarPtr;
typedef MoveVars_V2 *m2VarPtr;
typedef NavigationVars *navVarPtr;
typedef ObjectiveVars *oVarPtr;

struct SubVars { // 0x50
	/* 0x00 */ tVarPtr tVars;
	/* 0x04 */ nVarPtr nVars;
	/* 0x08 */ trVarPtr trVars;
	/* 0x0c */ bVarPtr bVars;
	/* 0x10 */ rVarPtr rVars;
	/* 0x14 */ scVarPtr scVars;
	/* 0x18 */ mVarPtr mVars;
	/* 0x1c */ m2VarPtr m2Vars;
	/* 0x20 */ aVarPtr aVars;
	/* 0x24 */ transVarPtr transVars;
	/* 0x28 */ eVarPtr eVars;
	/* 0x2c */ cmdVarPtr cmdVars;
	/* 0x30 */ roleVarPtr roleVars;
	/* 0x34 */ flashVarPtr flashVars;
	/* 0x38 */ sVarPtr sVars;
	/* 0x3c */ navVarPtr navVars;
	/* 0x40 */ oVarPtr oVars;
	/* 0x44 */ int pad[3];
};

struct TargetVars { // 0x90
	/* 0x00 */ float hitPoints;
	/* 0x04 */ int maxHitPoints;
	/* 0x08 */ unsigned char attackDamage[6];
	/* 0x0e */ short int hitCount;
	/* 0x10 */ int flags;
	/* 0x14 */ float targetHeight;
	/* 0x18 */ moby *mobyThatHurtMeLast;
	/* 0x1c */ float camPushDist;
	/* 0x20 */ float camPushHeight;
	/* 0x24 */ short int damageCounter;
	/* 0x26 */ short int empTimer;
	/* 0x28 */ short int infectedTimer;
	/* 0x2a */ short int invincTimer;
	/* 0x2c */ short int bogeyType;
	/* 0x2e */ short int team;
	/* 0x30 */ char lookAtMeDist;
	/* 0x31 */ char lookAtMePriority;
	/* 0x32 */ char lookAtMeZOfsIn8ths;
	/* 0x33 */ char lookAtMeJoint;
	/* 0x34 */ char lookAtMeExpression;
	/* 0x35 */ char lockOnPriority;
	/* 0x36 */ char soundType;
	/* 0x37 */ char targetRadiusIn8ths;
	/* 0x38 */ char noAutoTrack;
	/* 0x39 */ char trackSpeedInMps;
	/* 0x3a */ char camModOverride;
	/* 0x3b */ char destroyMe;
	/* 0x3c */ char morphoraySpecial;
	/* 0x3d */ char headJoint;
	/* 0x3e */ char hitByContinuous;
	/* 0x3f */ char infected;
	/* 0x40 */ char empFxTimer;
	/* 0x41 */ char weaponTargetedOnMe;
	/* 0x42 */ char isOrganic;
	/* 0x43 */ signed char bundleIndex;
	/* 0x44 */ char bundleDamage;
	/* 0x45 */ char firedAt;
	/* 0x46 */ char weaponThatHurtMeLast;
	/* 0x47 */ char invalidTarget;
	/* 0x48 */ int maxDifficultySlotted;
	/* 0x4c */ int curDifficultySlotted;
	/* 0x50 */ moby *pTargettedByBogeys[8];
	/* 0x70 */ moby *mobyThatFiredAtMe;
	/* 0x74 */ int targetShadowMask;
	/* 0x78 */ int damageTypes;
	/* 0x7c */ int padA;
	/* 0x80 */ float morphDamage;
	/* 0x84 */ float freezeDamage;
	/* 0x88 */ float infectDamage;
	/* 0x8c */ float lastDamage;
};

struct npcstring { // 0x14
	/* 0x00 */ char *text[5];
};

struct npcVars { // 0x40
	/* 0x00 */ short int type;
	/* 0x02 */ short int msg;
	/* 0x04 */ short int prevStep;
	/* 0x06 */ short int prevMsg;
	/* 0x08 */ char autoTalk;
	/* 0x09 */ char init;
	/* 0x0a */ short int pad;
	/* 0x0c */ float talkRange;
	/* 0x10 */ float hero_dist;
	/* 0x14 */ float cam_init_height;
	/* 0x18 */ float cam_init_dist;
	/* 0x1c */ float cam_init_rot;
	/* 0x20 */ npcstring name;
	/* 0x34 */ short int scene;
	/* 0x36 */ short int step;
	/* 0x38 */ int step_time;
	/* 0x3c */ npcStep *pStep;
};

struct TrackVars { // 0x40
	/* 0x00 */ vec4f rot;
	/* 0x10 */ vec4 disp;
	/* 0x20 */ vec4f oldTrackRot;
	/* 0x30 */ mtx3 *oldMtx;
	/* 0x34 */ int pad2[2];
	/* 0x3c */ int flags;
};

struct BogeyVars { // 0x90
	/* 0x00 */ vec4 targetPos;
	/* 0x10 */ moby *pTarget;
	/* 0x14 */ short int targetType;
	/* 0x16 */ short int targetStatus;
	/* 0x18 */ short int targetTimer;
	/* 0x1a */ short int lookForTypes;
	/* 0x1c */ short int flags;
	/* 0x1e */ short int targetOverrideTimer;
	/* 0x20 */ float shotSpeed;
	/* 0x24 */ int bestPotentialIndex;
	/* 0x28 */ int lastTargetTime;
	/* 0x2c */ short int allAwareTimer;
	/* 0x2e */ short int padA;
	/* 0x30 */ arealink alertArea;
	/* 0x34 */ float alertRadius;
	/* 0x38 */ float alertZDiff;
	/* 0x3c */ char curLookIndex;
	/* 0x3d */ char curTargetIndex;
	/* 0x3e */ char lookFrames;
	/* 0x3f */ char shotIsClear;
	/* 0x40 */ int difficultyRating;
	/* 0x44 */ float lookHeight;
	/* 0x48 */ float lookAcquireThreshold;
	/* 0x4c */ float lookLoseThreshold;
	/* 0x50 */ moby *pPotentialTargets[8];
	/* 0x70 */ float potentialLookAccum[8];
};

struct ReactVars { // 0x80
	/* 0x00 */ int flags;
	/* 0x04 */ int lastReactFrame;
	/* 0x08 */ moby *pInfectionMoby;
	/* 0x0c */ float acidDamage;
	/* 0x10 */ char eternalDeathCount;
	/* 0x11 */ char doHotSpotChecks;
	/* 0x12 */ char unchainable;
	/* 0x13 */ char isShielded;
	/* 0x14 */ char state;
	/* 0x15 */ char deathState;
	/* 0x16 */ char deathEffectState;
	/* 0x17 */ char deathStateType;
	/* 0x18 */ signed char knockbackRes;
	/* 0x19 */ char padC;
	/* 0x1a */ char padD;
	/* 0x1b */ char padE;
	/* 0x1c */ char padF;
	/* 0x1d */ char padG;
	/* 0x1e */ char padH;
	/* 0x1f */ char padI;
	/* 0x20 */ float minorReactPercentage;
	/* 0x24 */ float majorReactPercentage;
	/* 0x28 */ float deathHeight;
	/* 0x2c */ float bounceDamp;
	/* 0x30 */ int deadlyHotSpots;
	/* 0x34 */ float curUpGravity;
	/* 0x38 */ float curDownGravity;
	/* 0x3c */ float shieldDamageReduction;
	/* 0x40 */ float damageReductionSameOClass;
	/* 0x44 */ int deathCorn;
	/* 0x48 */ int deathType;
	/* 0x4c */ short int deathSound;
	/* 0x4e */ short int deathSound2;
	/* 0x50 */ float peakFrame;
	/* 0x54 */ float landFrame;
	/* 0x58 */ float drag;
	/* 0x5c */ short unsigned int effectStates;
	/* 0x5e */ short unsigned int effectPrimMask;
	/* 0x60 */ short unsigned int effectTimers[16];
};

struct ScriptVars {
	// unknown
};

typedef float fSpeed_mps;
typedef float fAccel_mps;
typedef float fSpeed_dps;
typedef float fAccel_dps;

struct MoveVars { // 0xf0
	/* 0x00 */ float collRadius;
	/* 0x04 */ float kneeHeight;
	/* 0x08 */ float maxStepUp;
	/* 0x0c */ float maxStepDown;
	/* 0x10 */ float slopeLimit;
	/* 0x14 */ fAccel_dps turnAccel;
	/* 0x18 */ fAccel_dps turnDeccel;
	/* 0x1c */ fSpeed_dps turnLimit;
	/* 0x20 */ fAccel_mps accel;
	/* 0x24 */ fAccel_mps deccel;
	/* 0x28 */ fSpeed_mps maxSpeed;
	/* 0x2c */ fAccel_mps gravity;
	/* 0x30 */ float rotThresh;
	/* 0x34 */ float distThresh;
	/* 0x38 */ int flags;
	/* 0x3c */ pathlink boundPath;
	/* 0x40 */ int *nearbyOClasses;
	/* 0x44 */ int numInNearbyOClasses;
	/* 0x48 */ Tweaker *pHeadTweak;
	/* 0x4c */ Tweaker *pTorsoTweak;
	/* 0x50 */ vec4 floorNormal;
	/* 0x60 */ vec4 bumpPoint;
	/* 0x70 */ vec4 push;
	/* 0x80 */ int arrestedTimer;
	/* 0x84 */ moby *pBumpMoby;
	/* 0x88 */ moby *pGroundMoby;
	/* 0x8c */ int poly;
	/* 0x90 */ float groundZ;
	/* 0x94 */ int resultFlags;
	/* 0x98 */ int prevResultFlags;
	/* 0x9c */ int internalFlags;
	/* 0xa0 */ vec4 vel;
	/* 0xb0 */ float realSpeed;
	/* 0xb4 */ float turnSpeed;
	/* 0xb8 */ int bumpTimer;
	/* 0xbc */ int lastUpdateFrame;
	/* 0xc0 */ short int onGood;
	/* 0xc2 */ short int offGood;
	/* 0xc4 */ Path *lastFollowPath;
	/* 0xc8 */ short int curNode;
	/* 0xca */ short int destNode;
	/* 0xcc */ float distToPoint;
	/* 0xd0 */ float actualAnimSpeed;
	/* 0xd4 */ float animAdjustLimit;
	/* 0xd8 */ int hotSpot;
	/* 0xdc */ float swarmOfsAmp;
	/* 0xe0 */ short int swarmOfsTimer;
	/* 0xe2 */ short int swarmOfsMinTime;
	/* 0xe4 */ short int swarmOfsMaxTime;
	/* 0xe6 */ char turnLeftAnim;
	/* 0xe7 */ char turnRightAnim;
	/* 0xe8 */ char walkAnim;
	/* 0xe9 */ char runAnim;
	/* 0xea */ char cachedAnim;
	/* 0xeb */ char padSec3[1];
	/* 0xec */ float fwdOfs;
};

typedef vec4 navg_waypoint;
typedef long unsigned int AnimCacheBitField;

struct MoveVars_V2 { // 0x1b0
	/* 0x000 */ int flags;
	/* 0x004 */ int internalFlags;
	/* 0x008 */ int effectorFlags;
	/* 0x00c */ int dirty;
	/* 0x010 */ float maxStepUp;
	/* 0x014 */ float maxStepDown;
	/* 0x018 */ int avoidHotspots;
	/* 0x01c */ int passThruHotspots;
	/* 0x020 */ short int arrestedTimer;
	/* 0x022 */ short int lostTimer;
	/* 0x024 */ float gravity;
	/* 0x028 */ float slopeLimit;
	/* 0x02c */ float maxFlightAngle;
	/* 0x030 */ char elv_state;
	/* 0x031 */ char alert_state;
	/* 0x032 */ char reaction_state;
	/* 0x033 */ char action_state;
	/* 0x034 */ char blend;
	/* 0x035 */ char lockAnim;
	/* 0x036 */ short int numColl;
	/* 0x038 */ MoveVarsAnimCache *pAnimCache;
	/* 0x03c */ MoveVarsAnimCache *pAttachAnimCache;
	/* 0x040 */ moby **effectorOverrideList;
	/* 0x044 */ int effectorOverrideCount;
	/* 0x048 */ arealink boundArea;
	/* 0x04c */ moby *pIgnoreCollMoby;
	/* 0x050 */ moby *pBumpMoby;
	/* 0x054 */ moby *pGroundMoby;
	/* 0x058 */ moby *pIgnoreEffector;
	/* 0x05c */ moby *pAttach;
	/* 0x060 */ int attachJoint;
	/* 0x064 */ float attachMaxRot;
	/* 0x068 */ float actionStartFrame;
	/* 0x06c */ void *pActionCallback;
	/* 0x070 */ int lastUpdateFrame;
	/* 0x074 */ int animGroups;
	/* 0x078 */ float collRadius;
	/* 0x07c */ float gravityVel;
	/* 0x080 */ float swarmOfsAmp;
	/* 0x084 */ int swarmOfsTimer;
	/* 0x088 */ int swarmOfsMinTime;
	/* 0x08c */ int swarmOfsMaxTime;
	/* 0x090 */ float stopDist;
	/* 0x094 */ float walkDist;
	/* 0x098 */ float runDist;
	/* 0x09c */ float walkSpeed;
	/* 0x0a0 */ float runSpeed;
	/* 0x0a4 */ float strafeSpeed;
	/* 0x0a8 */ float backSpeed;
	/* 0x0ac */ float flySpeed;
	/* 0x0b0 */ float linearAccel;
	/* 0x0b4 */ float linearDecel;
	/* 0x0b8 */ float linearLimit;
	/* 0x0bc */ float linearSpeed;
	/* 0x0c0 */ float angularAccel;
	/* 0x0c4 */ float angularDecel;
	/* 0x0c8 */ float angularLimit;
	/* 0x0cc */ float hitGroundSpeed;
	/* 0x0d0 */ float legFacing;
	/* 0x0d4 */ float bodyFacing;
	/* 0x0d8 */ float legAngularSpeed;
	/* 0x0dc */ float bodyAngularSpeed;
	/* 0x0e0 */ float groundSlope;
	/* 0x0e4 */ float groundZ;
	/* 0x0e8 */ int groundHotspot;
	/* 0x0ec */ int groundCheckFrame;
	/* 0x0f0 */ int onGround;
	/* 0x0f4 */ int offGround;
	/* 0x0f8 */ float passThruSurface;
	/* 0x0fc */ int passThruSurfaceType;
	/* 0x100 */ float projectedLandingZ;
	/* 0x104 */ float moveDamper;
	/* 0x108 */ short int moveDamperTimer;
	/* 0x10a */ char curNode;
	/* 0x10b */ char destNode;
	/* 0x10c */ Path *pLastFollowPath;
	/* 0x110 */ float walkTurnFactor;
	/* 0x114 */ float desiredFacing;
	/* 0x120 */ vec4 vel;
	/* 0x130 */ vec4 arrestedPos;
	/* 0x140 */ vec4 groundNormal;
	/* 0x150 */ vec4 jumpVel;
	/* 0x160 */ vec4 target;
	/* 0x170 */ vec4 passThruPoint;
	/* 0x180 */ vec4 passThruNormal;
	/* 0x190 */ navg_waypoint waypoint;
	/* 0x1a0 */ AnimCacheBitField groupCache;
	/* 0x1a8 */ AnimCacheBitField attachGroupCache;
};

struct ArmorVars { // 0x20
	/* 0x00 */ char bPercents[15];
	/* 0x0f */ char flags;
	/* 0x10 */ float oldHitPoints;
	/* 0x14 */ short int armorBits;
	/* 0x16 */ short int pad[5];
};

struct TransportVars { // 0x10
	/* 0x0 */ int bInitForTransport;
	/* 0x4 */ moby *pTransport;
	/* 0x8 */ float fTransportScale;
	/* 0xc */ int pad;
};

struct EffectorVars { // 0x10
	/* 0x0 */ char effectorMode;
	/* 0x1 */ char bunkerType;
	/* 0x2 */ char padc[2];
	/* 0x4 */ float strength;
	/* 0x8 */ int type;
	/* 0xc */ int pad;
};

struct CommandVars { // 0x14
	/* 0x00 */ FlashVars *pFlashVars;
	/* 0x04 */ moby *pDestroy;
	/* 0x08 */ short int commandType;
	/* 0x0a */ char active;
	/* 0x0b */ char padA;
	/* 0x0c */ cuboidlink spot;
	/* 0x10 */ float parameter;
};

struct RoleVars { // 0x28
	/* 0x00 */ GC_GroupController *tacticalGroup;
	/* 0x04 */ GC_Role *tacticalRole;
	/* 0x08 */ GC_Wave *pWave;
	/* 0x0c */ GC_SpawnReference *pSpawnRef;
	/* 0x10 */ Path *copiedPathInfo;
	/* 0x14 */ moby *copiedTargetInfo;
	/* 0x18 */ float copiedRolePriority;
	/* 0x1c */ int copiedRoleType;
	/* 0x20 */ char roleIsNew;
	/* 0x21 */ char roleCompletionState;
	/* 0x22 */ char roleOverridable;
	/* 0x23 */ char spawnedMethod;
	/* 0x24 */ unsigned int flags;
};

struct FlashVars { // 0x10
	/* 0x0 */ short int timer;
	/* 0x2 */ short int type;
	/* 0x4 */ int destColor;
	/* 0x8 */ int srcColor;
	/* 0xc */ int flags;
};

struct SuckVars { // 0x10
	/* 0x0 */ moby *linkedMoby[4];
};

struct NavigationVars { // 0x50
	/* 0x00 */ vec4 waypointPos;
	/* 0x10 */ vec4 location;
	/* 0x20 */ vec4 targetLoc;
	/* 0x30 */ int command;
	/* 0x34 */ int traversalFlags;
	/* 0x38 */ float toleranceSqr;
	/* 0x3c */ short unsigned int edge;
	/* 0x3e */ short unsigned int nextNode;
	/* 0x40 */ short int pathCurIndex;
	/* 0x42 */ short int pathDestIndex;
	/* 0x44 */ float jumpDistance;
	/* 0x48 */ int pad[2];
};

struct ObjectiveLogic { // 0x8
	/* 0x0 */ mobylink andObjective;
	/* 0x4 */ mobylink orObjective;
};

struct ObjectiveVars { // 0x1c
	/* 0x00 */ ObjectiveLogic successLogic;
	/* 0x08 */ ObjectiveLogic failureLogic;
	/* 0x10 */ char slave;
	/* 0x11 */ char pad[11];
};
