/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2022 chaoticgd

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

#include "extract_iso.h"

#include <editor/level_file_types.h>
#include <iso/iso_filesystem.h>

static void extract_non_wads_recursive(FILE* iso, const fs::path& out, iso_directory& in, const char* row_format);

void extract_iso(std::string iso_path, fs::path output_dir, const char* row_format) {
	fs::path global_dir = output_dir/"global";
	std::map<level_file_type, fs::path> level_dirs = {
		{level_file_type::LEVEL, output_dir/"levels"},
		{level_file_type::AUDIO, output_dir/"audio"},
		{level_file_type::SCENE, output_dir/"scenes"}
	};
	if(fs::is_directory(iso_path)) {
		fprintf(stderr, "error: Input path is a directory!\n");
		exit(1);
	}
	if(!fs::is_directory(output_dir)) {
		fprintf(stderr, "error: The output directory does not exist!\n");
		exit(1);
	}
	if(fs::exists(global_dir) && !fs::is_directory(global_dir)) {
		fprintf(stderr, "error: Existing files are cluttering up the output directory!\n");
		exit(1);
	}
	if(!fs::exists(global_dir)) {
		fs::create_directory(global_dir);
	}
	for(auto& [_, dir] : level_dirs) {
		if(fs::exists(dir) && !fs::is_directory(dir)) {
			fprintf(stderr, "error: Existing files are cluttering up the output directory!\n");
			exit(1);
		}
		if(!fs::exists(dir)) {
			fs::create_directory(dir);
		}
	}
	
	FILE* iso = fopen(iso_path.c_str(), "rb");
	verify(iso, "Failed to open ISO file.");
	defer([&]() { fclose(iso); });
	
	printf("LBA             Size (bytes)    Filename\n");
	printf("---             ------------    --------\n");
	
	// Extract SYSTEM.CNF, the boot ELF, etc.
	std::vector<u8> filesystem_buf = read_file(iso, 0, MAX_FILESYSTEM_SIZE_BYTES);
	iso_directory root_dir;
	std::string volume_id;
	if(!read_iso_filesystem(root_dir, volume_id, Buffer(filesystem_buf))) {
		fprintf(stderr, "error: Missing or invalid ISO filesystem!\n");
		exit(1);
	}
	extract_non_wads_recursive(iso, output_dir, root_dir, row_format);
	
	// Extract levels and other asset files.
	table_of_contents toc;
	if(volume_id == RAC1_VOLUME_ID) {
		toc = read_table_of_contents_rac1(iso);
	} else {
		toc = read_table_of_contents_rac234(iso);
		if(toc.levels.size() == 0) {
			fprintf(stderr, "error: Unable to locate level table!\n");
			exit(1);
		}
	}
	for(toc_table& table : toc.tables) {
		auto name = std::to_string(table.index) + ".wad";
		auto path = global_dir/name;
		
		size_t start_of_file = table.sector.bytes();
		size_t end_of_file = SIZE_MAX;
		// Assume the beginning of the next file after this one is also the end
		// of this file.
		for(toc_table& other_table : toc.tables) {
			Sector32 lba = other_table.sector;
			if(lba.bytes() > start_of_file) {
				end_of_file = std::min(end_of_file, (size_t) lba.bytes());
			}
		}
		for(toc_level& other_level : toc.levels) {
			for(std::optional<toc_level_part>& part : other_level.parts) {
				if(part && part->file_lba.bytes() > start_of_file) {
					end_of_file = std::min(end_of_file, (size_t) part->file_lba.bytes());
				}
			}
		}
		assert(end_of_file != SIZE_MAX);
		assert(end_of_file >= start_of_file);
		size_t file_size = end_of_file - start_of_file;
		printf(row_format, (size_t) table.sector.sectors, (size_t) file_size, name.c_str());
		
		FILE* dest_file = fopen(path.string().c_str(), "wb");
		verify(dest_file, "Failed to open '%s' for writing.", path.string().c_str());
		extract_file(path, dest_file, iso, table.sector.bytes(), file_size);
		fclose(dest_file);
	}
	for(toc_level& level : toc.levels) {
		for(std::optional<toc_level_part>& part : level.parts) {
			if(!part) {
				continue;
			}
			
			auto num = std::to_string(level.level_table_index);
			if(num.size() == 1) num = "0" + num;
			auto name = part->info.prefix + num + ".wad";
			auto path = level_dirs.at(part->info.type)/name;
			printf(row_format, (size_t) part->file_lba.sectors, part->file_size.bytes(), name.c_str());
			
			FILE* dest_file = fopen(path.string().c_str(), "wb");
			verify(dest_file, "Failed to open '%s' for writing.", path.string().c_str());
			if(part->prepend_header) {
				std::vector<u8> padded_header;
				OutBuffer(padded_header).write_multiple(part->header);
				OutBuffer(padded_header).pad(SECTOR_SIZE, 0);
				verify(fwrite(padded_header.data(), padded_header.size(), 1, dest_file) == 1,
					"Failed to write header to '%s'.", path.string().c_str());
			}
			extract_file(path, dest_file, iso, part->file_lba.bytes(), part->file_size.bytes());
			fclose(dest_file);
		}
	}
}

static void extract_non_wads_recursive(FILE* iso, const fs::path& out, iso_directory& in, const char* row_format) {
	for(iso_file_record& file : in.files) {
		fs::path file_path = out/file.name.substr(0, file.name.size() - 2);
		if(file_path.string().find(".wad") == std::string::npos) {
			print_file_record(file, row_format);
			FILE* dest_file = fopen(file_path.string().c_str(), "wb");
			verify(dest_file, "Failed to open '%s' for writing.", file_path.string().c_str());
			extract_file(file_path, dest_file, iso, file.lba.bytes(), file.size);
			fclose(dest_file);
		}
	}
	for(iso_directory& subdir : in.subdirs) {
		auto dir_path = out/subdir.name;
		fs::create_directory(dir_path);
		extract_non_wads_recursive(iso, out/subdir.name, subdir, row_format);
	}
}
