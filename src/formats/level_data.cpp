/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019 chaoticgd

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
#include "level_data.h"

const uint32_t BARLOW_OUTER_HEADER_OFFSET = 0x12e800;

std::unique_ptr<level_impl> level_data::import_level(stream& level_file) {

	auto lvl = std::make_unique<level_impl>();

	auto master_hdr = level_file.read<master_header>(0);

	uint32_t moby_wad_offset = locate_moby_wad(level_file);
	{
		array_stream moby_wad_data;
		proxy_stream wad_segment(&level_file, moby_wad_offset);
		decompress_wad(moby_wad_data, wad_segment);
		import_moby_wad(*lvl.get(), moby_wad_data);
	}

	uint32_t secondary_header_delta =
		(master_hdr.secondary_moby_offset_part * 0x800 + 0xfff) & 0xfffffffffffff000;
	uint32_t secondary_header_offset = moby_wad_offset - secondary_header_delta;
	
	auto secondary_hdr =
		level_file.read<secondary_header>(secondary_header_offset);

	{
		uint32_t ram_image_wad_offset = secondary_hdr.ram_image_wad.value;
		array_stream ram_image_data;
		proxy_stream ram_image_wad_segment(&level_file,
			secondary_header_offset + ram_image_wad_offset);
		decompress_wad(ram_image_data, ram_image_wad_segment);
		import_ram_image_wad(*lvl.get(), ram_image_data);
	}

	return lvl;
}

uint32_t level_data::locate_moby_wad(stream& level_file) {
	
	// For now just find the largest 0x100 byte-aligned WAD segment.
	// This should work for most levels.

	uint32_t result_offset = 1;
	long result_size = -1;
	for(uint32_t offset = 0; offset < level_file.size() - sizeof(wad_header); offset += 0x100) {
		wad_header header = level_file.read<wad_header>(offset);
		if(validate_wad(header.magic) && header.total_size > result_size) {
			result_offset = offset;
			result_size = header.total_size;
		}
	}

	if(result_offset == 1) {
		throw stream_format_error("File does not contain a valid WAD segment.");
	}
	
	return result_offset;
}

void level_data::import_moby_wad(level_impl& lvl, stream& moby_wad) {
	auto header = moby_wad.read<moby_wad::header>(0);

	auto moby_table = moby_wad.read<moby_wad::moby_table>(header.mobies.value);

	auto moby_ptr = header.mobies.next<moby_wad::moby>().value;
	for(uint32_t i = 0; i < moby_table.num_mobies; i++) {
		auto moby_data = moby_wad.read<moby_wad::moby>(moby_ptr);
		uint32_t uid = moby_data.uid;

		auto current = std::make_unique<::moby>(uid);
		current->name = std::to_string(moby_ptr);
		current->class_num = moby_data.class_num;
		current->set_position(
			glm::vec3(moby_data.position.x, moby_data.position.y, moby_data.position.z));
		lvl.add_moby(uid, std::move(current));

		moby_ptr += moby_data.size;
	}
}

void level_data::import_ram_image_wad(level_impl& lvl, stream& wad) {

	// Convert to PS2 system memory address space.
	proxy_stream image(&wad, -ram_image::BASE_OFFSET);
}
