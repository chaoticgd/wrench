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

std::unique_ptr<level_impl> level_data::import_level(stream& level_file, worker_logger& log) {

	auto lvl = std::make_unique<level_impl>();

	auto master_hdr = level_file.read<master_header>(0);

	uint32_t moby_wad_offset = locate_moby_wad(level_file);
	log << "Found moby WAD at 0x" << std::hex << moby_wad_offset << "\n";
	{
		array_stream moby_wad_data;
		proxy_stream wad_segment(&level_file, moby_wad_offset);
		log << "Decompressing moby WAD... ";
		decompress_wad(moby_wad_data, wad_segment);
		log << "DONE!\nImporting moby WAD... ";
		import_moby_wad(*lvl.get(), moby_wad_data);
		log << "DONE!\n";
	}

	uint32_t secondary_header_delta =
		(master_hdr.secondary_moby_offset_part * 0x800 + 0xfff) & 0xfffffffffffff000;
	uint32_t secondary_header_offset = moby_wad_offset - secondary_header_delta;
	
	auto secondary_hdr =
		level_file.read<secondary_header>(secondary_header_offset);
	
	log << "\nLevel imported successfully.\n";

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

	auto ship_data = moby_wad.read<moby_wad::ship_data>(header.ship.value);
	lvl.ship_position = {
		ship_data.position.x, ship_data.position.y, ship_data.position.z
	};

	//
	// Import ingame strings.
	//

	uint32_t
		english_ptr = header.english_strings.value,
		french_ptr  = header.french_strings.value,
		german_ptr  = header.german_strings.value,
		spanish_ptr = header.spanish_strings.value,
		italian_ptr = header.italian_strings.value,
		nolang_ptr  = header.null_strings.value;
	const std::vector<std::pair<std::string, uint32_t>> languages {
		{ "English", english_ptr },
		{ "French",  french_ptr },
		{ "German",  german_ptr },
		{ "Spanish", spanish_ptr },
		{ "Italian", italian_ptr },
		{ "NoLang",  nolang_ptr }
	};
	for(auto language : languages) {
		auto table_header = moby_wad.read<moby_wad::string_table>(language.second);
		std::vector<std::pair<uint32_t, std::string>> strings;
		for(uint32_t i = 0; i < table_header.num_strings; i++) {
			auto table_entry = moby_wad.read<moby_wad::string_table_entry>();

			uint32_t id = table_entry.id;
			uint32_t pos = moby_wad.tell();
			moby_wad.seek(language.second + table_entry.string.value);
			std::string str = moby_wad.read_string();
			moby_wad.seek(pos);
			strings.emplace_back(id, str);
		}
		lvl.strings.emplace_back(language.first, strings);
	}

	//
	// Import mobies (entities).
	//
	
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
