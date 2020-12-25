#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// Prints out a memory map from an eeMemory.bin file.
// Supports R&C2, R&C3 and Deadlocked.

#define GAME_COUNT         4
#define MIN_SEGMENT_COUNT  10
#define RAC1_SEGMENT_COUNT 40 // Not sure.
#define RAC2_SEGMENT_COUNT 35
#define RAC3_SEGMENT_COUNT 36
#define DL_SEGMENT_COUNT   53
#define EE_MEMORY_SIZE     (32 * 1024 * 1024)
#define KERNEL_BASE        0x0
#define CODE_SEGMENT_BASE  0x100000

int detect_game(uint8_t* ee_memory);

// Caution: Deadlocked contains the R&C3 pattern.
static const char* PATTERNS[GAME_COUNT] = {
	"IOPRP243.IMG", "IOPRP255.IMG", "Ratchet and Clank: Up Your Arsenal", "Ratchet: Deadlocked"
};
static const uint32_t SEGMENT_COUNTS[GAME_COUNT] = {
	RAC1_SEGMENT_COUNT, RAC2_SEGMENT_COUNT, RAC3_SEGMENT_COUNT, DL_SEGMENT_COUNT
};
static const char* RAC1_SEGMENT_LABELS[RAC1_SEGMENT_COUNT];
static const char* RAC2_SEGMENT_LABELS[RAC2_SEGMENT_COUNT];
static const char* RAC3_SEGMENT_LABELS[RAC3_SEGMENT_COUNT];
static const char* DL_SEGMENT_LABELS[DL_SEGMENT_COUNT];
static const char** SEGMENT_LABELS[GAME_COUNT] = {
	RAC1_SEGMENT_LABELS, RAC2_SEGMENT_LABELS, RAC3_SEGMENT_LABELS, DL_SEGMENT_LABELS
};

int main(int argc, char** argv) {
	if(argc != 2) {
		fprintf(stderr, "usage: %s path/to/eeMemory.bin\n", argv[0]);
		fprintf(stderr, "Supports R&C2, R&C3 and Deadlocked.\n");
		exit(1);
	}
	uint32_t* ee_memory = malloc(EE_MEMORY_SIZE);
	FILE* file = fopen(argv[1], "rb");
	if(!file) {
		fprintf(stderr, "Failed to open file.\n");
		return 1;
	}
	if(fread(ee_memory, EE_MEMORY_SIZE, 1, file) != 1) {
		fprintf(stderr, "Failed to read data from file.\n");
		return 1;
	}
	int game = detect_game((uint8_t*) ee_memory);
	switch(game) {
		case 0: printf("--- Detected R&C1. Game not supported!\n"); exit(1);
		case 1: printf("--- Detected R&C2.\n"); break;
		case 2: printf("--- Detected R&C3.\n"); break;
		case 3: printf("--- Detected DL. Segment sizes may be inaccurate.\n"); break;
		default: fprintf(stderr, "Cannot detect game!\n"); exit(1);
	}
	uint32_t i, j;
	for(i = CODE_SEGMENT_BASE / 0x4; i < EE_MEMORY_SIZE / 4 - SEGMENT_COUNTS[game]; i++) {
		uint32_t* ptr = ee_memory + i;
		
		// The PS2 kernel and code segments are always at the same addresses.
		if(ptr[0] != KERNEL_BASE || ptr[1] != CODE_SEGMENT_BASE) {
			continue;
		}
		
		// The addresses must be in ascending order.
		int should_skip = 0;
		for(j = 0; j < 5; j++) {
			if(ptr[j] > ptr[j + 1] || ptr[j] > EE_MEMORY_SIZE) {
				should_skip = 1;
			}
		}
		if(should_skip) {
			continue;
		}
		
		for(j = 0; j < SEGMENT_COUNTS[game]; j++) {
			int32_t size;
			if(ptr[j] == 0 || ptr[j + 1] < ptr[j]) {
				size = -1;
			} else if(j == SEGMENT_COUNTS[game] - 1) {
				size = EE_MEMORY_SIZE - ptr[j];
			} else {
				size = ptr[j + 1] - ptr[j];
			}
			printf("%08x %-16s%8x", (i + j) * 4, SEGMENT_LABELS[game][j], ptr[j]);
			if(size == -1) {
				printf("     ??? k\n");
			} else {
				printf("%8d k\n", size / 1024);
			}
		}
		return 0;
	}
	fprintf(stderr, "Failed to find memory map.\n");
	return 1;
}

int detect_game(uint8_t* ee_memory) {
	int i, j, k;
	for(i = GAME_COUNT - 1; i >= 0; i--) {
		int game_matches = 0;
		int pattern_size = strlen(PATTERNS[i]);
		for(j = CODE_SEGMENT_BASE; j < EE_MEMORY_SIZE - 0x1000; j++) {
			int address_matches = 1;
			for(k = 0; k < pattern_size; k++) {
				if(ee_memory[j + k] != PATTERNS[i][k]) {
					address_matches = 0;
					break;
				}
			}
			if(address_matches) {
				game_matches = 1;
			}
		}
		if(game_matches) {
			return i;
		}
	}
	return -1;
}

static const char* RAC1_SEGMENT_LABELS[RAC1_SEGMENT_COUNT] = {
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
	"", "", "", "",
	"", "", "", ""
};
	
static const char* RAC2_SEGMENT_LABELS[RAC2_SEGMENT_COUNT] = {
	"OS",
	"Code",
	"",
	"",
	"",
	"",
	"",
	"Tfrag Geometry",
	"Occlusion",
	"Sky",
	"Collision",
	"Shared VRAM",
	"Particle VRAM",
	"Effects VRAM",
	"Mobies",
	"Ties",
	"Shrubs",
	"Ratchet Seqs",
	"",
	"Help Messages",
	"Tie Instances",
	"Shrub Instances",
	"Moby Instances",
	"Moby Pvars",
	"Misc Instances",
	"",
	"",
	"",
	"",
	"",
	"",
	"HUD",
	"GUI",
	"",
	""
};

static const char* RAC3_SEGMENT_LABELS[RAC3_SEGMENT_COUNT] = {
	"OS",
	"Code",
	"",
	"",
	"",
	"",
	"",
	"Tfrag Geometry",
	"Occlusion",
	"Sky",
	"Collision",
	"Shared VRAM",
	"Particle VRAM",
	"Effects VRAM",
	"Mobies",
	"Ties",
	"Shrubs",
	"Ratchet Seqs",
	"",
	"Help Messages",
	"Tie Instances",
	"Shrub Instances",
	"Moby Instances",
	"Moby Pvars",
	"Misc Instances",
	"",
	"",
	"",
	"",
	"",
	"",
	"", // R&C2 doesn't have this.
	"HUD",
	"GUI",
	"",
	""
};

static const char* DL_SEGMENT_LABELS[DL_SEGMENT_COUNT] = {
	"OS",
	"Code",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"Tfrag Geometry",
	"Occlusion",
	"Sky",
	"Collision",
	"Shared VRAM",
	"Particle VRAM",
	"Effects VRAM",
	"Mobies",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"Help Messages",
	"Tie Instances",
	"",
	"Moby Instances",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"HUD",
	"",
	"",
	"",
	""
};
