#pragma wrench parser on

enum M2716_RollResult {
	ROLL_LOSS = 0,
	ROLL_BOLTS = 1,
	ROLL_SELFDESTRUCT = 2,
	ROLL_AMMO = 3,
	ROLL_HEALTH = 4,
	ROLL_JACKPOT = 5
};

struct update2716 {
	char field0_0x0[32];
	float unk20; /* 3.0 */
	int unk24; /* 3 */
	char field3_0x28[4];
	int unk2C; /* 0x7F000000 */
	float field5_0x30; /* 1.0 */
	char field6_0x34[20];
	int unk48; /* 0xFF070000 */
	char field8_0x4c[4];
	int unk50; /* 0x3C (60) */
	int unk54;
	int unk58; /* 0x32 (50) */
	M2716_RollResult rollResult; /* Result of the roll, between 0 and 5 */
	float slot1DisplayState; /* A float that determines which part of the band is shown */
	float slot2DisplayState;
	float slot3DisplayState;
	unsigned int slot1Timer; /* Slots spins while their timer is non-0 */
	unsigned int slot2Timer;
	unsigned int slot3Timer;
	unsigned int slot1SpinState; /* 0 = stopped, 1/2 = moving */
	unsigned int slot2SpinState;
	unsigned int slot3SpinState;
	float slot1SpinSpeed;
	float slot2SpinSpeed;
	float slot3SpinSpeed;
	M2716_RollResult slot1Result;
	M2716_RollResult slot2Result;
	M2716_RollResult slot3Result;
	char field28_0x9c[20]; /* Padding? */
};
