#pragma wrench parser on

struct npcstring { // 0x14
	/* 0x00 */ char *text[5];
};

struct npcStep { // 0x1c
	/* 0x00 */ int offer;
	/* 0x04 */ short int scene;
	/* 0x06 */ short int dest;
	/* 0x08 */ short int cond_type;
	/* 0x0a */ short int cond_val;
	/* 0x0c */ short int true_dest;
	/* 0x0e */ short int false_dest;
	/* 0x10 */ short int flags;
	/* 0x12 */ short int pad[5];
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
