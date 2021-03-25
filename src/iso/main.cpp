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

#include "../stream.h"
#include "../fs_includes.h"
#include "../command_line.h"
#include "iso_filesystem.h"
#include "table_of_contents.h"

// This is true for R&C2, R&C3 and Deadlocked.
static const size_t TABLE_OF_CONTENTS_LBA = 0x3e9;

void ls(std::string iso_path);
void extract(std::string iso_path, fs::path output_dir);
void build(std::string iso_path, fs::path input_directory);

int main(int argc, char** argv) {
	cxxopts::Options options(argv[0],
		"Extract files from and rebuild Ratchet & Clank ISO images. The games\n"
		"use raw disk I/O and a custom table of contents file to access assets\n"
		"so just writing a standard ISO filesystem won't work.");
	options.positional_help("ls|extract|rebuild <iso file> [<input/output directory>]");
	options.add_options()
		("c,command", "The operation to perform. Possible values are: ls, extract, build.",
			cxxopts::value<std::string>())
		("i,iso", "The ISO file.",
			cxxopts::value<std::string>())
		("d,directory", "The input/output directory.",
			cxxopts::value<std::string>());

	options.parse_positional({
		"command", "iso", "directory"
	});

	auto args = parse_command_line_args(argc, argv, options);
	std::string command = cli_get(args, "command");
	std::string iso_path = cli_get(args, "iso");
	std::string directory_path = cli_get_or(args, "directory", "");
	
	if(command == "ls") {
		ls(iso_path);
	} else if(command == "extract") {
		extract(iso_path, directory_path);
	} else if(command == "build") {
		build(iso_path, directory_path);
	} else {
		fprintf(stderr, "Invalid command: %s\n",  command.c_str());
		fprintf(stderr, "Available commands are: ls, extract, build\n");
		exit(1);
	}
}

// Fun fact: This used to be its own command line tool called "toc". Now, it's
// been reduced to a humble subcommand within a greater tool. Pity it.
void ls(std::string iso_path) {
	file_stream iso(iso_path);
	table_of_contents toc = read_table_of_contents(iso, TABLE_OF_CONTENTS_LBA * SECTOR_SIZE);
	
	printf("+-[Non-level Sections]--+-------------+-------------+\n");
	printf("| Index | Offset in ToC | Size in ToC | Data Offset |\n");
	printf("| ----- | ------------- | ----------- | ----------- |\n");
	for(size_t i = 0; i < toc.tables.size(); i++) {
		toc_table& table = toc.tables[i];
		size_t base_offset = table.header.base_offset.bytes();
	printf("| %02ld    | %08x      | %08x    | %08lx    |\n",
		i, table.offset_in_toc, table.header.header_size, base_offset);
	}
	printf("+-------+---------------+-------------+-------------+\n");
	
	printf("+-[Level Table]------------------+------------------------+------------------------+\n");
	printf("|       | LEVELn.WAD             | AUDIOn.WAD             | SCENEn.WAD             |\n");
	printf("|       | ----------             | ----------             | ----------             |\n");
	printf("| Index | Offset      Size       | Offset      Size       | Offset      Size       |\n");
	printf("| ----- | ------      ----       | ------      ----       | ------      ----       |\n");
	for(size_t i = 0; i < toc.levels.size(); i++) {
		toc_level& lvl = toc.levels[i];
		size_t main_part_base = iso.read<sector32>(lvl.main_part.bytes() + 4).bytes();
		printf("| %02ld    | %010lx  %010lx |",
			i, main_part_base, lvl.main_part_size.bytes());
		if(lvl.audio_part.sectors != 0) {
			size_t audio_part_base = iso.read<sector32>(lvl.audio_part.bytes() + 4).bytes();
			printf(" %010lx  %010lx |", audio_part_base, lvl.audio_part_size.bytes());
		} else {
			printf(" N/A         N/A        |");
		}
		if(lvl.scene_part.sectors != 0) {
			size_t scene_part_base = iso.read<sector32>(lvl.scene_part.bytes() + 4).bytes();
			printf(" %010lx  %010lx |", scene_part_base, lvl.scene_part_size.bytes());
		} else {
			printf(" N/A         N/A        |");
		}
		printf("\n");
	}
	printf("+-------+------------------------+------------------------+------------------------+\n");
}

void extract(std::string iso_path, fs::path output_dir) {
	fs::path global_dir = output_dir/"global";
	fs::path levels_dir = output_dir/"levels";
	if(!fs::is_directory(output_dir)) {
		fprintf(stderr, "error: The output directory does not exist!\n");
		exit(1);
	}
	if(fs::exists(global_dir) && !fs::is_directory(global_dir)) {
		fprintf(stderr, "error: Existing files are cluttering up the output directory!\n");
		exit(1);
	}
	if(fs::exists(levels_dir) && !fs::is_directory(levels_dir)) {
		fprintf(stderr, "error: Existing files are cluttering up the output directory!\n");
		exit(1);
	}
	if(!fs::exists(global_dir)) {
		fs::create_directory(global_dir);
	}
	if(!fs::exists(levels_dir)) {
		fs::create_directory(levels_dir);
	}
	
	file_stream iso(iso_path);
	
	// Extract SYSTEM.CNF, the boot ELF, etc.
	std::vector<iso_file_record> files;
	if(!read_iso_filesystem(files, iso)) {
		fprintf(stderr, "error: Missing or invalid ISO filesystem!\n");
		exit(1);
	}
	for(iso_file_record& file : files) {
		fs::path file_path = output_dir/file.name;
		file_stream output_file(file_path.string(), std::ios::out);
		iso.seek(file.lba.bytes());
		stream::copy_n(output_file, iso, file.size);
	}
	
	// Extract levels and other asset files.
	table_of_contents toc = read_table_of_contents(iso, TABLE_OF_CONTENTS_LBA * SECTOR_SIZE);
	for(toc_table& table : toc.tables) {
		auto name = std::to_string(table.index) + ".wad";
		auto path = global_dir/name;
		size_t file_size = 0;
		for(sector_range& lump : table.lumps) {
			size_t lump_size;
			// HACK: MPEG.WAD stores some sizes in bytes.
			if(lump.size.sectors > 0xfffff) {
				lump_size = lump.size.sectors;
			} else {
				lump_size = lump.size.bytes();
			}
			
			size_t lump_end = lump.offset.bytes() + lump_size;
			if(lump_end > file_size) {
				file_size = lump_end;
			}
		}
		
		file_stream output_file(path, std::ios::out);
		iso.seek(table.header.base_offset.bytes());
		// This doesn't really matter, but I decided to make the LBA field
		// of the extracted header be equal to the offset of where the data
		// starts in the extracted file in sectors.
		table.header.base_offset = sector32::size_from_bytes(table.header.header_size);
		output_file.write(table.header);
		output_file.write_v(table.lumps);
		output_file.pad(SECTOR_SIZE, 0);
		stream::copy_n(output_file, iso, file_size);
	}
	for(toc_level& level : toc.levels) {
		sector32 header_lbas[3] = { level.main_part, level.audio_part, level.scene_part };
		sector32 file_sizes[3] = { level.main_part_size, level.audio_part_size, level.scene_part_size };
		const char* names[3] = { "level", "audio", "scene" };
		for(int i = 0; i < 3; i++) {
			auto name = names[i] + std::to_string(level.level_table_index) + ".wad";
			auto path = levels_dir/name;
			file_stream output_file(path.string(), std::ios::out);
			
			uint8_t header[SECTOR_SIZE];
			iso.seek(header_lbas[i].bytes());
			iso.read_n((char*) header, SECTOR_SIZE);
			
			sector32* file_lba_ptr = (sector32*) &header[4];
			sector32 file_lba = *file_lba_ptr;
			*file_lba_ptr = sector32{sizeof(header) / SECTOR_SIZE}; // Same as above.
			
			output_file.write_n((char*) header, SECTOR_SIZE);
			iso.seek(file_lba.bytes());
			stream::copy_n(output_file, iso, file_sizes[i].bytes());
		}
	}
}

void build(std::string iso_path, fs::path input_directory) {
	
}
