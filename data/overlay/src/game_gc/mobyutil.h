#pragma wrench parser on

struct SubVars {
	/* 0x00 */ void* targetVars;
	/* 0x04 */ void* unknown_04;
	/* 0x08 */ void* unknown_08;
	/* 0x0c */ void* unknown_0c;
	/* 0x10 */ void* reactVars;
	/* 0x14 */ void* unknown_14;
	/* 0x18 */ void* unknown_18;
	/* 0x1c */ void* moveV2Vars;
};

struct TargetVars {
	/* 0x00 */ float hitPoints;
	/* 0x04 */ short maxHitPoints; /* int? */
	/* 0x06 */ char field2_0x6;
	/* 0x07 */ char field3_0x7;
	/* 0x08 */ unsigned char attackDamage[6];
	/* 0x0e */ unsigned short hitCount;
	/* 0x10 */ int flags;
	/* 0x14 */ char field7_0x14;
	/* 0x15 */ char field8_0x15;
	/* 0x16 */ char field9_0x16;
	/* 0x17 */ unsigned char weaponThatHurtMeLast;
	/* 0x18 */ MobyInstance* mobyThatHurtMeLast;
	/* 0x1c */ unsigned short damageCounter;
	/* 0x1e */ char field13_0x1e;
	/* 0x1f */ char field14_0x1f;
	/* 0x20 */ int unknown_20;
	/* 0x24 */ int unknown_24;
	/* 0x28 */ int unknown_28;
	/* 0x2c */ int unknown_2c;
};

struct GcVars04 {
	/* 0x00 */ int unknown_0;
};

struct GcVars08 {
	/* 0x00 */ int unknown_0;
	/* 0x04 */ int unknown_4;
	/* 0x08 */ int unknown_8;
	/* 0x0c */ int unknown_c;
	/* 0x10 */ int unknown_10;
	/* 0x14 */ int unknown_14;
	/* 0x18 */ int unknown_18;
	/* 0x1c */ int unknown_1c;
	/* 0x20 */ int unknown_20;
	/* 0x24 */ int unknown_24;
	/* 0x28 */ int unknown_28;
	/* 0x2c */ int unknown_2c;
	/* 0x30 */ int unknown_30;
	/* 0x34 */ int unknown_34;
	/* 0x38 */ int unknown_38;
	/* 0x3c */ int unknown_3c;
};

struct GcVars0c {
	/* 0x00 */ int unknown_0;
	/* 0x04 */ int unknown_4;
	/* 0x08 */ int unknown_8;
	/* 0x0c */ int unknown_c;
	/* 0x10 */ int unknown_10;
	/* 0x14 */ int unknown_14;
	/* 0x18 */ int unknown_18;
	/* 0x1c */ int unknown_1c;
};

struct ReactVars {
	/* 0x00 */ int unknown_0;
	/* 0x04 */ int unknown_4;
	/* 0x08 */ int unknown_8;
	/* 0x0c */ int unknown_c;
	/* 0x10 */ int unknown_10;
	/* 0x14 */ int unknown_14;
	/* 0x18 */ int unknown_18;
	/* 0x1c */ int unknown_1c;
	/* 0x20 */ int unknown_20;
	/* 0x24 */ int unknown_24;
	/* 0x28 */ int unknown_28;
	/* 0x2c */ int unknown_2c;
	/* 0x30 */ int unknown_30;
	/* 0x34 */ int unknown_34;
	/* 0x38 */ int unknown_38;
	/* 0x3c */ int unknown_3c;
	/* 0x40 */ int unknown_40;
	/* 0x44 */ int unknown_44;
	/* 0x48 */ int unknown_48;
	/* 0x4c */ int unknown_4c;
	/* 0x50 */ int unknown_50;
	/* 0x54 */ int unknown_54;
	/* 0x58 */ int unknown_58;
	/* 0x5c */ int unknown_5c;
	/* 0x60 */ int unknown_60;
	/* 0x64 */ int unknown_64;
	/* 0x68 */ int unknown_68;
	/* 0x6c */ int unknown_6c;
	/* 0x70 */ int unknown_70;
	/* 0x74 */ int unknown_74;
	/* 0x78 */ int unknown_78;
	/* 0x7c */ int unknown_7c;
	/* 0x80 */ int unknown_80;
	/* 0x84 */ int unknown_84;
	/* 0x88 */ int unknown_88;
	/* 0x8c */ int unknown_8c;
	/* 0x90 */ int unknown_90;
	/* 0x94 */ int unknown_94;
	/* 0x98 */ int unknown_98;
	/* 0x9c */ int unknown_9c;
	/* 0xa0 */ int unknown_a0;
	/* 0xa4 */ int unknown_a4;
	/* 0xa8 */ int unknown_a8;
	/* 0xac */ int unknown_ac;
};

struct GcVars14 {
	/* 0x00 */ int unknown_0;
	/* 0x04 */ int unknown_4;
	/* 0x08 */ int unknown_8;
	/* 0x0c */ int unknown_c;
	/* 0x10 */ int unknown_10;
	/* 0x14 */ int unknown_14;
	/* 0x18 */ int unknown_18;
	/* 0x1c */ int unknown_1c;
	/* 0x20 */ int unknown_20;
	/* 0x24 */ int unknown_24;
	/* 0x28 */ int unknown_28;
	/* 0x2c */ int unknown_2c;
	/* 0x30 */ int unknown_30;
	/* 0x34 */ int unknown_34;
	/* 0x38 */ int unknown_38;
	/* 0x3c */ int unknown_3c;
	/* 0x40 */ int unknown_40;
	/* 0x44 */ int unknown_44;
	/* 0x48 */ int unknown_48;
	/* 0x4c */ int unknown_4c;
	/* 0x50 */ int unknown_50;
	/* 0x54 */ int unknown_54;
	/* 0x58 */ int unknown_58;
	/* 0x5c */ int unknown_5c;
	/* 0x60 */ int unknown_60;
	/* 0x64 */ int unknown_64;
	/* 0x68 */ int unknown_68;
	/* 0x6c */ int unknown_6c;
	/* 0x70 */ int unknown_70;
	/* 0x74 */ int unknown_74;
	/* 0x78 */ int unknown_78;
	/* 0x7c */ int unknown_7c;
	/* 0x80 */ int unknown_80;
	/* 0x84 */ int unknown_84;
	/* 0x88 */ int unknown_88;
	/* 0x8c */ int unknown_8c;
	/* 0x90 */ int unknown_90;
	/* 0x94 */ int unknown_94;
	/* 0x98 */ int unknown_98;
	/* 0x9c */ int unknown_9c;
	/* 0xa0 */ int unknown_a0;
	/* 0xa4 */ int unknown_a4;
	/* 0xa8 */ int unknown_a8;
	/* 0xac */ int unknown_ac;
	/* 0xb0 */ int unknown_b0;
	/* 0xb4 */ int unknown_b4;
	/* 0xb8 */ int unknown_b8;
	/* 0xbc */ int unknown_bc;
	/* 0xc0 */ int unknown_c0;
	/* 0xc4 */ int unknown_c4;
	/* 0xc8 */ int unknown_c8;
	/* 0xcc */ int unknown_cc;
	/* 0xd0 */ int unknown_d0;
	/* 0xd4 */ int unknown_d4;
	/* 0xd8 */ int unknown_d8;
	/* 0xdc */ int unknown_dc;
	/* 0xe0 */ int unknown_e0;
	/* 0xe4 */ int unknown_e4;
	/* 0xe8 */ int unknown_e8;
	/* 0xec */ int unknown_ec;
	/* 0xf0 */ int unknown_f0;
	/* 0xf4 */ int unknown_f4;
	/* 0xf8 */ int unknown_f8;
	/* 0xfc */ int unknown_fc;
	/* 0x100 */ int unknown_100;
	/* 0x104 */ int unknown_104;
	/* 0x108 */ int unknown_108;
	/* 0x10c */ int unknown_10c;
	/* 0x110 */ int unknown_110;
	/* 0x114 */ int unknown_114;
	/* 0x118 */ int unknown_118;
	/* 0x11c */ int unknown_11c;
	/* 0x120 */ int unknown_120;
	/* 0x124 */ int unknown_124;
	/* 0x128 */ int unknown_128;
	/* 0x12c */ int unknown_12c;
	/* 0x130 */ int unknown_130;
	/* 0x134 */ int unknown_134;
	/* 0x138 */ int unknown_138;
	/* 0x13c */ int unknown_13c;
	/* 0x140 */ int unknown_140;
	/* 0x144 */ int unknown_144;
	/* 0x148 */ int unknown_148;
	/* 0x14c */ int unknown_14c;
	/* 0x150 */ int unknown_150;
	/* 0x154 */ int unknown_154;
	/* 0x158 */ int unknown_158;
	/* 0x15c */ int unknown_15c;
};

struct GcVars18 {
	/* 0x00 */ int unknown_0;
	/* 0x04 */ int unknown_4;
	/* 0x08 */ int unknown_8;
	/* 0x0c */ int unknown_c;
	/* 0x10 */ int unknown_10;
	/* 0x14 */ int unknown_14;
	/* 0x18 */ int unknown_18;
	/* 0x1c */ int unknown_1c;
	/* 0x20 */ int unknown_20;
	/* 0x24 */ int unknown_24;
	/* 0x28 */ int unknown_28;
	/* 0x2c */ int unknown_2c;
	/* 0x30 */ int unknown_30;
	/* 0x34 */ int unknown_34;
	/* 0x38 */ int unknown_38;
	/* 0x3c */ int unknown_3c;
	/* 0x40 */ int unknown_40;
	/* 0x44 */ int unknown_44;
	/* 0x48 */ int unknown_48;
	/* 0x4c */ int unknown_4c;
	/* 0x50 */ int unknown_50;
	/* 0x54 */ int unknown_54;
	/* 0x58 */ int unknown_58;
	/* 0x5c */ int unknown_5c;
	/* 0x60 */ int unknown_60;
	/* 0x64 */ int unknown_64;
	/* 0x68 */ int unknown_68;
	/* 0x6c */ int unknown_6c;
	/* 0x70 */ int unknown_70;
	/* 0x74 */ int unknown_74;
	/* 0x78 */ int unknown_78;
	/* 0x7c */ int unknown_7c;
	/* 0x80 */ int unknown_80;
	/* 0x84 */ int unknown_84;
	/* 0x88 */ int unknown_88;
	/* 0x8c */ int unknown_8c;
	/* 0x90 */ int unknown_90;
	/* 0x94 */ int unknown_94;
	/* 0x98 */ int unknown_98;
	/* 0x9c */ int unknown_9c;
	/* 0xa0 */ int unknown_a0;
	/* 0xa4 */ int unknown_a4;
	/* 0xa8 */ int unknown_a8;
	/* 0xac */ int unknown_ac;
	/* 0xb0 */ int unknown_b0;
	/* 0xb4 */ int unknown_b4;
	/* 0xb8 */ int unknown_b8;
	/* 0xbc */ int unknown_bc;
	/* 0xc0 */ int unknown_c0;
	/* 0xc4 */ int unknown_c4;
	/* 0xc8 */ int unknown_c8;
	/* 0xcc */ int unknown_cc;
	/* 0xd0 */ int unknown_d0;
	/* 0xd4 */ int unknown_d4;
	/* 0xd8 */ int unknown_d8;
	/* 0xdc */ int unknown_dc;
	/* 0xe0 */ int unknown_e0;
	/* 0xe4 */ int unknown_e4;
	/* 0xe8 */ int unknown_e8;
	/* 0xec */ int unknown_ec;
};

struct MoveVars_V2 {
	/* 0x00 */ int unknown_0;
	/* 0x04 */ int unknown_4;
	/* 0x08 */ int unknown_8;
	/* 0x0c */ int unknown_c;
	/* 0x10 */ int unknown_10;
	/* 0x14 */ int unknown_14;
	/* 0x18 */ int unknown_18;
	/* 0x1c */ int unknown_1c;
};
