#ifndef FORMATS_LEVEL_DATA_H
#define FORMATS_LEVEL_DATA_H

#include <memory>
#include <stdint.h>

#include "../level.h"
#include "../stream.h"
#include "../vec.h"
#include "wad.h"

struct level_data_header;
struct level_data_moby_table;
struct level_data_moby;

packed_struct(level_data_header,
	uint8_t unknown1[0x4c];
	file_ptr<level_data_moby_table> mobies;
)

packed_struct(level_data_moby_table,
	uint32_t num_mobies;
	uint32_t unknown[3];
)

packed_struct(level_data_moby,
	uint32_t size;           // 0x0
	uint32_t unknown1[0x3];  // 0x4
	uint32_t uid;            // 0x10
	uint32_t unknown2[0xb];  // 0x14
	vec3f position;          // 0x40
	vec3f rotation;          // 0x4c
	uint32_t unknown3[0x2d]; // 0x58
)

std::unique_ptr<level_impl> import_level(stream& level_file);

uint32_t locate_main_level_segment(stream& level_file);

#endif
