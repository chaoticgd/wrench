#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

// Prints out a memory map from an eeMemory.bin file.
// Currently only works for R&C2.

#define SEGMENT_COUNT     35
#define EE_MEMORY_SIZE    (32 * 1024 * 1024)
#define KERNEL_BASE       0x0
#define CODE_SEGMENT_BASE 0x100000

static const char* SEGMENT_LABELS[SEGMENT_COUNT];

int main(int argc, char** argv) {
	if(argc != 2) {
		fprintf(stderr, "usage: %s path/to/eeMemory.bin\n", argv[0]);
		fprintf(stderr, "Currently only works for R&C2.\n");
		exit(1);
	}
	uint32_t* ee_memory = malloc(EE_MEMORY_SIZE);
	FILE* file = fopen(argv[1], "rb");
	if(!file) {
		fprintf(stderr, "Failed to open file.\n");
		exit(1);
	}
	if(fread(ee_memory, EE_MEMORY_SIZE, 1, file) != 1) {
		fprintf(stderr, "Failed to read data from file.\n");
		exit(1);
	}
	uint32_t i, j;
	for(i = CODE_SEGMENT_BASE / 0x4; i < EE_MEMORY_SIZE / 4 - SEGMENT_COUNT; i++) {
		uint32_t* ptr = ee_memory + i;
		// Look for the start of the memory map/segment table.
		if(*ptr == KERNEL_BASE && *(ptr + 1) == CODE_SEGMENT_BASE) {
			for(j = 0; j < SEGMENT_COUNT; j++) {
				uint32_t size;
				if(j == SEGMENT_COUNT - 1) {
					size = EE_MEMORY_SIZE - ptr[j];
				} else {
					size = ptr[j + 1] - ptr[j];
				}
				printf("%08x %-16s%8x%8d k\n", (i + j) * 4, SEGMENT_LABELS[j], ptr[j], size / 1024);
			}
			exit(1);
		}
	}
	fprintf(stderr, "Failed to find memory map.\n");
	exit(1);
}

static const char* SEGMENT_LABELS[SEGMENT_COUNT] = {
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
