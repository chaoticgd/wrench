/*
	BSD 2-Clause License

	Copyright (c) 2019-2021 chaoticgd
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

	1. Redistributions of source code must retain the above copyright notice, this
	list of conditions and the following disclaimer.

	2. Redistributions in binary form must reproduce the above copyright notice,
	this list of conditions and the following disclaimer in the documentation
	and/or other materials provided with the distribution.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
	AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
	FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
	DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
	CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
	OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <platform/fileio.h>

// Prints out a memory map from an eeMemory.bin file.
// Supports R&C1, R&C2, R&C3 and Deadlocked.

#define GAME_COUNT         4
#define MIN_SEGMENT_COUNT  5
#define MAX_SEGMENT_COUNT  53
#define RAC1_SEGMENT_COUNT 15 // We're lumping multiple segments together for now.
#define RAC2_SEGMENT_COUNT 35
#define RAC3_SEGMENT_COUNT 36
#define DL_SEGMENT_COUNT   53
#define EE_MEMORY_SIZE     (32 * 1024 * 1024)
#define KERNEL_BASE        0x0
#define CODE_SEGMENT_BASE  0x100000

typedef struct {
	const char* name;
	uint32_t pointer;
	uint32_t address;
} memory_segment;

int detect_game(uint8_t* ee_memory);
void build_memory_map_rac1(memory_segment* map, uint8_t* ee_memory);
int find_opcode_pattern(uint32_t* ee_memory, const int8_t* pattern, int instruction_count);
void build_memory_map_rac234(memory_segment* map, uint32_t* ee_memory, int game);
void print_memory_map(memory_segment* map, int segment_count);

// Caution: Deadlocked contains the R&C3 pattern.
static const char* PATTERNS[GAME_COUNT] = {
	"IOPRP243.IMG", "IOPRP255.IMG", "Ratchet and Clank: Up Your Arsenal", "Ratchet: Deadlocked"
};
static const uint32_t SEGMENT_COUNTS[GAME_COUNT] = {
	RAC1_SEGMENT_COUNT, RAC2_SEGMENT_COUNT, RAC3_SEGMENT_COUNT, DL_SEGMENT_COUNT
};
static const char* RAC2_SEGMENT_LABELS[RAC2_SEGMENT_COUNT];
static const char* RAC3_SEGMENT_LABELS[RAC3_SEGMENT_COUNT];
static const char* DL_SEGMENT_LABELS[DL_SEGMENT_COUNT];
static const char** SEGMENT_LABELS[GAME_COUNT] = {
	NULL, RAC2_SEGMENT_LABELS, RAC3_SEGMENT_LABELS, DL_SEGMENT_LABELS
};

int main(int argc, char** argv)
{
	if(argc != 2) {
		fprintf(stderr, "usage: %s path/to/eeMemory.bin\n", argv[0]);
		fprintf(stderr, "Supports R&C1, R&C2, R&C3 and Deadlocked.\n");
		exit(1);
	}
	uint8_t* ee_memory = malloc(EE_MEMORY_SIZE);
	WrenchFileHandle* file = file_open(argv[1], WRENCH_FILE_MODE_READ);
	if(!file) {
		fprintf(stderr, "Failed to open file '%s' for reading (%s).\n", argv[1], FILEIO_ERROR_CONTEXT_STRING);
		return 1;
	}
	if(file_read(ee_memory, EE_MEMORY_SIZE, file) != EE_MEMORY_SIZE) {
		fprintf(stderr, "Failed to read data from file.\n");
		return 1;
	}
	int game = detect_game(ee_memory);
	
	memory_segment map[MAX_SEGMENT_COUNT];
	switch(game) {
		case 0:
			printf("--- Detected R&C1.\n");
			build_memory_map_rac1(map, ee_memory);
			break;
		case 1:
		case 2:
			printf("--- Detected R&C%d.\n", game + 1);
			build_memory_map_rac234(map, (uint32_t*) ee_memory, game);
			break;
		case 3:
			printf("--- Detected DL.\n");
			build_memory_map_rac234(map,(uint32_t*) ee_memory, game);
			break;
		default:
			fprintf(stderr, "Cannot detect game!\n");
			exit(1);
	}
	print_memory_map(map, SEGMENT_COUNTS[game]);
	free(ee_memory);
	file_close(file);
	return 0;
}

int detect_game(uint8_t* ee_memory)
{
	int i, j, k;
	for(i = GAME_COUNT - 1; i >= 0; i--) {
		int pattern_size = strlen(PATTERNS[i]);
		for(j = CODE_SEGMENT_BASE; j < EE_MEMORY_SIZE - pattern_size; j++) {
			if(memcmp(&ee_memory[j], PATTERNS[i], pattern_size) == 0) {
				return i;
			}
		}
	}
	return -1;
}

#define OPCODE_LUI   0b001111
#define OPCODE_COP1  0b10001
#define OPCODE_JAL   0b000011
#define OPCODE_ADDIU 0b001001
#define MASK_OPCODE  0b11111100000000000000000000000000
#define MASK_IMMED   0b00000000000000001111111111111111
// Extract the immediate operand from insn.
#define GET_IMMED(insn) ((*(int16_t*) &(insn)) & MASK_IMMED)
// Reconstruct a pointer from seperate high and low parts stored as seperate immediates.
#define GET_POINTER(hi, lo) \
	(((int16_t) GET_IMMED(ee_memory[res + hi])) << 16) + \
	((int16_t) GET_IMMED(ee_memory[res + lo]))
// Build a memory_segment object from its name and the addresses of the high and low instructions.
#define RAC1_SEGMENT(str, hi, lo) \
	tmp.name = str; \
	tmp.pointer = GET_POINTER(hi, lo); \
	if(tmp.pointer > EE_MEMORY_SIZE - 4) { \
		fprintf(stderr, "error: Invalid memory segment pointer address.\n"); \
		exit(1); \
	} \
	tmp.address = *(uint32_t*) &ee_memory[tmp.pointer]; \
	if(tmp.address > EE_MEMORY_SIZE) { \
		fprintf(stderr, "error: Invalid memory segment address: %x.\n", tmp.address); \
		exit(1); \
	} \
	map[i++] = tmp
// Convenience macro as sometimes lo = hi + 4.
#define RAC1_SEGMENT_SEQ(str, hi) RAC1_SEGMENT(str, hi, hi + 4)
static const int8_t RAC1_MEMMAP_OPCODES[] = {
		OPCODE_LUI,
		OPCODE_LUI,
		OPCODE_COP1, // mtc1
		OPCODE_JAL, // printf("\n*** MEMORY MAP ***\n\n")
		OPCODE_ADDIU,
		OPCODE_LUI,
		OPCODE_LUI,
		OPCODE_JAL,
		OPCODE_ADDIU,
		OPCODE_JAL
};

void build_memory_map_rac1(memory_segment* map, uint8_t* ee_memory)
{
	// Find the bit in the code where the sizes of all the memory segments get
	// printed out.
	int res = find_opcode_pattern((uint32_t*) ee_memory, RAC1_MEMMAP_OPCODES, sizeof(RAC1_MEMMAP_OPCODES));
	if(res == -1) {
		fprintf(stderr, "error: Unable to find memory map code!\n");
		exit(1);
	}
	
	// Used by the macros below.
	memory_segment tmp;
	int i = 0;
	
	// Build the memory map by determining the addresses of pointers to memory
	// segments from the aforementioned code that references them.
	map[i].name = "OS";
	map[i].pointer = 0;
	map[i].address = 0;
	i++;
	map[i].name = ""; // code, dead space, vu chain bufs, hud, gadget buffer
	map[i].pointer = 0;
	map[i].address = 0x100000;
	i++;
	//RAC1_SEGMENT_SEQ("Tfrag Geomtry + Occlusion", 0x138);
	RAC1_SEGMENT_SEQ("Sky", 0x200);
	RAC1_SEGMENT("Collision", 0x180, 0x230);
	RAC1_SEGMENT_SEQ("Shared VRAM + Particle VRAM", 0x278);
	//RAC1_SEGMENT("Particle VRAM", 0x274, 0x2b0);
	RAC1_SEGMENT_SEQ("Effects VRAM", 0x2ec);
	RAC1_SEGMENT("Mobies", 0x24c, 0x31c);
	RAC1_SEGMENT("Ties", 0x258, 0x314);
	RAC1_SEGMENT_SEQ("Shrubs + Ratchet Seqs", 0x3bc);
	//RAC1_SEGMENT("Ratchet Seqs", 0x0, 0x408);
	RAC1_SEGMENT_SEQ("Tie Instances",   0x444);
	RAC1_SEGMENT_SEQ("Shrub Instances", 0x478);
	RAC1_SEGMENT_SEQ("Moby Instances",  0x4ac);
	RAC1_SEGMENT_SEQ("Moby Pvars",      0x4e0);
	RAC1_SEGMENT_SEQ("Paths",           0x514);
	RAC1_SEGMENT_SEQ("Part Instances + Stack", 0x560);
}

int find_opcode_pattern(uint32_t* ee_memory, const int8_t* pattern, int instruction_count)
{
	int32_t i, j;
	for(i = 0; i < EE_MEMORY_SIZE / 4 - instruction_count; i++) {
		int match = 1;
		for(j = 0; j < instruction_count; j++) {
			if((ee_memory[i + j] & MASK_OPCODE) != (pattern[j] << 26)) {
				match = 0;
			}
		}
		if(match) {
			return i * 4;
		}
	}
	return -1;
}

void build_memory_map_rac234(memory_segment* map, uint32_t* ee_memory, int game)
{
	int32_t i, j;
	for(i = CODE_SEGMENT_BASE / 0x4; i < EE_MEMORY_SIZE / 4 - SEGMENT_COUNTS[game]; i++) {
		uint32_t* ptr = ee_memory + i;
		
		// The PS2 kernel and code segments are always at the same addresses.
		if(ptr[0] != KERNEL_BASE || ptr[1] != CODE_SEGMENT_BASE) {
			continue;
		}
		
		// The addresses must be in ascending order.
		int should_skip = 0;
		for(j = 0; j < MIN_SEGMENT_COUNT; j++) {
			if(ptr[j] > ptr[j + 1] || ptr[j] > EE_MEMORY_SIZE) {
				should_skip = 1;
			}
		}
		if(should_skip) {
			continue;
		}
		
		for(j = 0; j < SEGMENT_COUNTS[game]; j++) {
			memory_segment seg = {SEGMENT_LABELS[game][j], (i + j) * 4, ptr[j]};
			map[j] = seg;
		}
		return;
	}
	fprintf(stderr, "Failed to find memory map.\n");
	exit(1);
}

void print_memory_map(memory_segment* map, int segment_count)
{
	int i, j;
	for(i = 0; i < segment_count; i++) {
		int32_t size = INT32_MAX;
		for(j = 0; j < segment_count; j++) {
			if(i != j && map[i].address == map[j].address) {
				// We cannot determine the size of the segment, as there are
				// multiple segments with the same address.
				size = -1; 
			}
		}
		if(size != -1) {
			// Calculate the size of the current segment by trying to find the
			// address of the next segment and subtracting the address of the
			// current segment from that.
			for(j = 0; j < segment_count; j++) {
				int32_t possible_size = map[j].address - map[i].address;
				if(map[j].address > map[i].address && possible_size < size) {
					size = possible_size;
				}
			}
			if(size == INT32_MAX) {
				// It's the last segment, assume it takes up the rest of memory.
				size = EE_MEMORY_SIZE - map[i].address;
			}
		}
		printf("%08x %-32s%8x", map[i].pointer, map[i].name, map[i].address);
		if(size == -1) {
			printf("     ??? k\n");
		} else {
			printf("%8d k\n", size / 1024);
		}
	}
}

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
	"os",
	"code",
	"base",
	"vu1_chain_1",
	"vu1_chain_2",
	"tie_cache",
	"moby_joint_cache",
	"joint_cache_entry_list",
	"level_base",
	"level_nav",
	"level_tfrag",
	"level_occl",
	"level_sky",
	"level_coll",
	"level_vram",
	"level_part_vram",
	"level_fx_vram",
	"level_mobys",
	"level_ties",
	"level_shrubs",
	"level_ratchet",
	"level_gameplay",
	"level_global_nav_data",
	"level_mission_load_buffer",
	"level_mission_pvar_buffer",
	"level_mission_class_buffer_1",
	"level_mission_class_buffer_2",
	"level_mission_moby_insts",
	"level_mission_moby_pvars",
	"level_mission_moby_groups",
	"level_mission_moby_shared",
	"level_art",
	"level_help",
	"level_tie_insts",
	"level_shrub_insts",
	"level_moby_insts",
	"level_moby_insts_backup",
	"level_moby_pvars",
	"level_moby_pvars_backup",
	"level_misc_insts",
	"level_part_insts",
	"level_moby_sound_remap",
	"level_end",
	"perm_base",
	"perm_armor",
	"perm_armor2",
	"perm_skin",
	"perm_patch",
	"hud",
	"gui",
	"net_overlay",
	"heap",
	"stack"
};
