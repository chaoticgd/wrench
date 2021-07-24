/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2021 chaoticgd

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef FORMATS_LEVEL_TYPES_H
#define FORMATS_LEVEL_TYPES_H

#include <variant>
#include <optional>
#include <stdint.h>
#include <glm/glm.hpp>

#include "../stream.h"

# /*
# 	Defines the types that make up the level format, including game objects.
# */

struct level_primary_header;
struct level_asset_header;
struct level_texture_entry;

// *****************************************************************************
// Outer level structures
// *****************************************************************************

packed_struct(level_primary_header_rac23,
	byte_range code_segment;   // 0x0
	byte_range asset_header;   // 0x8
	byte_range small_textures; // 0x10
	byte_range hud_header;     // 0x18
	byte_range hud_bank_0;     // 0x20
	byte_range hud_bank_1;     // 0x28
	byte_range hud_bank_2;     // 0x30
	byte_range hud_bank_3;     // 0x38
	byte_range hud_bank_4;     // 0x40
	byte_range asset_wad;      // 0x48
	byte_range loading_screen_textures; // 0x50
)

packed_struct(level_primary_header_rac4,
	byte_range unknown_0;      // 0x0
	byte_range code_segment;   // 0x8
	byte_range asset_header;   // 0x10
	byte_range small_textures; // 0x18
	byte_range hud_header;     // 0x20
	byte_range hud_bank_0;     // 0x28
	byte_range hud_bank_1;     // 0x30
	byte_range hud_bank_2;     // 0x38
	byte_range hud_bank_3;     // 0x40
	byte_range hud_bank_4;     // 0x48
	byte_range asset_wad;      // 0x50
	byte_range instances_wad;  // 0x58
	byte_range world_wad;      // 0x60
)

packed_struct(level_code_segment_header,
	uint32_t base_address; // 0x0 Where to load it in RAM.
	uint32_t unknown_4;
	uint32_t unknown_8;
	uint32_t entry_offset; // 0xc The address of the main_loop function, relative to base_address.
	// Code segment immediately follows.
)

// Barlow 0x418200
packed_struct(level_asset_header,
	uint32_t mipmap_count;           // 0x0
	uint32_t mipmap_offset;          // 0x4
	uint32_t tfrag_geomtry;          // 0x8
	uint32_t occlusion;              // 0xc
	uint32_t sky;                    // 0x10
	uint32_t collision;              // 0x14
	uint32_t moby_model_count;       // 0x18
	uint32_t moby_model_offset;      // 0x1c
	uint32_t tie_model_count;        // 0x20
	uint32_t tie_model_offset;       // 0x24
	uint32_t shrub_model_count;      // 0x28
	uint32_t shrub_model_offset;     // 0x2c
	uint32_t tfrag_texture_count;    // 0x30
	uint32_t tfrag_texture_offset;   // 0x34 Relative to asset header.
	uint32_t moby_texture_count;     // 0x38
	uint32_t moby_texture_offset;    // 0x3c
	uint32_t tie_texture_count;      // 0x40
	uint32_t tie_texture_offset;     // 0x44
	uint32_t shrub_texture_count;    // 0x48
	uint32_t shrub_texture_offset;   // 0x4c
	uint32_t some2_texture_count;    // 0x50
	uint32_t some2_texture_offset;   // 0x54
	uint32_t sprite_texture_count;   // 0x58
	uint32_t sprite_texture_offset;  // 0x5c
	uint32_t tex_data_in_asset_wad;  // 0x60
	uint32_t ptr_into_asset_wad_64;  // 0x64
	uint32_t ptr_into_asset_wad_68;  // 0x68
	uint32_t rel_to_asset_header_6c; // 0x6c
	uint32_t rel_to_asset_header_70; // 0x70
	uint32_t unknown_74; // 0x74
	uint32_t rel_to_asset_header_78; // 0x78
	uint32_t unknown_7c; // 0x7c
	uint32_t index_into_some1_texs; // 0x80
	uint32_t unknown_84; // 0x84
	uint32_t unknown_88; // 0x88
	uint32_t unknown_8c; // 0x8c
	uint32_t unknown_90; // 0x90
	uint32_t unknown_94; // 0x94
	uint32_t unknown_98; // 0x98
	uint32_t unknown_9c; // 0x9c
	uint32_t unknown_a0; // 0xa0
	uint32_t ptr_into_asset_wad_a4; // 0xa4
	uint32_t unknown_a8; // 0xa8
	uint32_t unknown_ac; // 0xac
	uint32_t ptr_into_asset_wad_b0; // 0xb0
)

packed_struct(level_moby_model_entry,
	uint32_t offset_in_asset_wad;
	uint32_t o_class;
	uint32_t unknown_8;
	uint32_t unknown_c;
	uint8_t textures[16];
)

packed_struct(level_shrub_model_entry,
	uint32_t offset_in_asset_wad;
	uint32_t o_class;
	uint32_t unknown_8;
	uint32_t unknown_c;
	uint8_t textures[16];
	uint8_t unknown_20[16];
)

packed_struct(level_mipmap_descriptor,
	uint32_t unknown_0; // Type?
	uint16_t width;
	uint16_t height;
	uint32_t offset_1;
	uint32_t offset_2; // Duplicate of offset_1?
)

packed_struct(level_texture_descriptor,
	uint32_t ptr;
	uint16_t width;
	uint16_t height;
	uint16_t unknown_8;
	uint16_t palette;
	uint32_t field_c;
)

packed_struct(level_hud_header,
	uint32_t unknown_0;  // 0x0
	uint32_t unknown_4;  // 0x4
	uint32_t unknown_8;  // 0x8
	uint32_t unknown_c;  // 0xc
	uint32_t unknown_10; // 0x10
	uint32_t unknown_14; // 0x14
	uint32_t unknown_18; // 0x18
	uint32_t unknown_1c; // 0x1c
	uint32_t unknown_20; // 0x20
	uint32_t unknown_24; // 0x24
	uint32_t unknown_28; // 0x28
	uint32_t unknown_2c; // 0x2c
	uint32_t unknown_30; // 0x30
	uint32_t unknown_34; // 0x34
	uint32_t unknown_38; // 0x38
	uint32_t unknown_3c; // 0x3c
	uint32_t unknown_40; // 0x40
	uint32_t unknown_44; // 0x44
	uint32_t unknown_48; // 0x48
	uint32_t unknown_4c; // 0x4c
	uint32_t unknown_50; // 0x50
	uint32_t bank_0;     // 0x54
	uint32_t unknown_58; // 0x58
	uint32_t bank_2;     // 0x5c
	uint32_t bank_3;     // 0x60
	uint32_t bank_4;     // 0x64
)

#endif
