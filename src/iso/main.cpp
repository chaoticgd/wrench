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

#include "../util.h"
#include "../stream.h"
#include "../fs_includes.h"
#include "../command_line.h"
#include "iso_filesystem.h"
#include "table_of_contents.h"

// This is true for R&C2, R&C3 and Deadlocked.
static const uint32_t TABLE_OF_CONTENTS_LBA = 0x3e9;

void ls(std::string iso_path);
void extract(std::string iso_path, fs::path output_dir);
void build(std::string iso_path, fs::path input_dir);
void enumerate_files_recursive(std::vector<fs::path>& files, fs::path dir, int depth);

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
	for(toc_level& level : toc.levels) {
		size_t offsets[3] = {0, 0, 0};
		size_t sizes[3] = {0, 0, 0};
		
		for(std::optional<toc_level_part>& part : level.parts) {
			if(!part) {
				continue;
			}
			size_t part_column = 0;
			switch(part->info.type) {
				case level_file_type::LEVEL: part_column = 0; break;
				case level_file_type::AUDIO: part_column = 1; break;
				case level_file_type::SCENE: part_column = 2; break;
			}
			if(offsets[part_column] != 0) {
				fprintf(stderr, "error: Level table entry references multiple files of same type.\n");
				exit(1);
			}
			offsets[part_column] = part->file_lba.bytes();
			sizes[part_column] = part->file_size.bytes();
		}
		
		printf("| %03ld   |", level.level_table_index);
		for(int i = 0; i < 3; i++) {
			if(offsets[i] != 0) {
				printf(" %010lx  %010lx |", offsets[i], sizes[i]);
			} else {
				printf(" N/A         N/A        |");
			}
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
		fs::path file_path = output_dir/file.name.substr(0, file.name.size() - 2);
		file_stream output_file(file_path.string(), std::ios::out);
		iso.seek(file.lba.bytes());
		stream::copy_n(output_file, iso, file.size);
	}
	
	// Extract levels and other asset files.
	table_of_contents toc = read_table_of_contents(iso, TABLE_OF_CONTENTS_LBA * SECTOR_SIZE);
	if(toc.levels.size() == 0) {
		fprintf(stderr, "error: Unable to locate level table!\n");
		exit(1);
	}
	for(toc_table& table : toc.tables) {
		auto name = std::to_string(table.index) + ".wad";
		auto path = global_dir/name;
		
		size_t start_of_file = table.header.base_offset.bytes();
		size_t end_of_file = SIZE_MAX;
		// Assume the beginning of the next file after this one is also the end
		// of this file.
		for(toc_table& other_table : toc.tables) {
			sector32 lba = other_table.header.base_offset;
			if(lba.bytes() > start_of_file) {
				end_of_file = std::min(end_of_file, lba.bytes());
			}
		}
		for(toc_level& other_level : toc.levels) {
			for(std::optional<toc_level_part>& part : other_level.parts) {
				if(part && part->file_lba.bytes() > start_of_file) {
					end_of_file = std::min(end_of_file, part->file_lba.bytes());
				}
			}
		}
		assert(end_of_file != SIZE_MAX);
		assert(end_of_file >= start_of_file);
		size_t file_size = end_of_file - start_of_file;
		
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
		for(std::optional<toc_level_part>& part : level.parts) {
			if(!part) {
				continue;
			}
			auto name = part->info.prefix + std::to_string(level.level_table_index) + ".wad";
			auto path = levels_dir/name;
			file_stream output_file(path.string(), std::ios::out);
			
			output_file.write<uint32_t>(part->magic);
			output_file.write<uint32_t>(part->info.header_size_sectors); // Same as above.
			output_file.write_v(part->lumps);
			iso.seek(part->file_lba.bytes());
			stream::copy_n(output_file, iso, part->file_size.bytes());
		}
	}
}

struct global_file {
	fs::path path;
	sector32 lba;
	bool operator<(global_file& rhs) { return path < rhs.path; }
};

const size_t LEVEL_PART = 0;
const size_t AUDIO_PART = 1;
const size_t SCENE_PART = 2;
struct level_parts {
	std::optional<fs::path> parts[3];
	sector32 data_offsets[3];
	sector32 data_lbas[3]; // For writing out the table of contents.
	sector32 sizes[3]; // For writing out the table of contents.
};

void build(std::string iso_path, fs::path input_dir) {
	std::vector<fs::path> input_files;
	enumerate_files_recursive(input_files, input_dir, 0);
	
	auto str_to_lower = [](std::string str) {
		for(char& c : str) {
			c = tolower(c);
		}
		return str;
	};
	
	// Seperate asset (*.WAD) files from other files (e.g. SYSTEM.CNF).
	std::vector<fs::path> wad_files;
	std::vector<fs::path> other_files;
	for(fs::path& path : input_files) {
		auto name = str_to_lower(path.filename().string());
		if(name.find(".wad") != std::string::npos) {
			wad_files.push_back(path);
		} else {
			other_files.push_back(path);
		}
	}
	
	// Seperate global files (ARMOR.WAD, etc) from level files (LEVEL0.WAD, AUDIO0.WAD, etc).
	std::vector<global_file> global_files;
	std::vector<level_parts> level_files;
	for(fs::path& path : wad_files) {
		auto name = str_to_lower(path.filename().string());
		bool is_global = true;
		for(int part = 0; part < 3; part++) {
			static const char* LEVEL_PART_NAMES[3] = { "level", "audio", "scene" };
			if(name.find(LEVEL_PART_NAMES[part]) == 0) {
				assert(name.size() >= 9);
				int level_index;
				try {
					level_index = std::stoi(name.substr(5, name.size() - 9).c_str());
				} catch(std::invalid_argument&) {
					break;
				}
				if(level_index < 0 || level_index > 100) {
					fprintf(stderr, "error: Level index is out of range.\n");
					exit(1);
				}
				if(level_files.size() <= (size_t) level_index) {
					level_files.resize(level_index + 1);
				}
				level_files[level_index].parts[part] = path;
				is_global = false;
			}
		}
		if(is_global) {
			global_files.push_back({path});
		}
	}
	
	// HACK: Assume that global files are numbered 0.wad, 1.wad, etc.
	// This is usually only true for files extracted using this tool!
	std::sort(global_files.begin(), global_files.end());
	
	// Sanity check: Make sure that if there's a AUDIOn.WAD file or a SCENEn.WAD
	// file that there's also a LEVELn.WAD file.
	for(level_parts& level : level_files) {
		if(level.parts[AUDIO_PART] && !level.parts[LEVEL_PART]) {
			fprintf(stderr, "error: An audio file is missing an associated level file!\n");
			exit(1);
		}
		if(level.parts[SCENE_PART] && !level.parts[LEVEL_PART]) {
			fprintf(stderr, "error: A scene file is missing an associated level file!\n");
			exit(1);
		}
	}
	
	// Calculate the size of the table of contents file so we can determine the
	// LBAs of all the files that come after it.
	size_t global_toc_size_bytes = 0;
	for(global_file& global : global_files) {
		file_stream file(global.path);
		uint32_t size = file.read<uint32_t>();
		if(size > 0xffff) {
			fprintf(stderr, "error: File '%s' has a header size > 0xffff bytes.\n", global.path.filename().c_str());
			exit(1);
		}
		global_toc_size_bytes += size;
	}
	global_toc_size_bytes += level_files.size() * sizeof(sector_range) * 3;
	sector32 global_toc_size = sector32::size_from_bytes(global_toc_size_bytes);
	assert(global_toc_size.sectors <= 0xb); // This size is hardcoded in the boot ELF for R&C2.
	sector32 total_toc_size = global_toc_size;
	for(auto& level : level_files) {
		// Assume all of these headers are a single sector.
		total_toc_size.sectors += !!level.parts[LEVEL_PART];
		total_toc_size.sectors += !!level.parts[AUDIO_PART];
		total_toc_size.sectors += !!level.parts[SCENE_PART];
	}
	
	// Determine the LBAs of files on disc and build the root directory.
	std::vector<iso_file_record> root_dir;
	sector32 lba {TABLE_OF_CONTENTS_LBA + total_toc_size.sectors};
	for(fs::path& path : other_files) {
		auto name = str_to_lower(path.filename().string());
		if(name == "rc2.hdr") {
			// We're writing out a new table of contents, so this one
			// isn't needed.
			continue;
		}
		iso_file_record record;
		record.name = name + ";1";
		record.lba = lba;
		record.size = file_stream(path).size();
		record.source = path;
		root_dir.push_back(record);
		lba.sectors += sector32::size_from_bytes(record.size).sectors;
	}
	for(global_file& global : global_files) {
		global.lba = lba;
		
		file_stream file(global.path);
		sector32 data_offset = file.read<sector32>(0x4);
		
		iso_file_record record;
		record.name = global.path.filename().string() + ";1";
		record.lba = global.lba;
		record.size = file.size() - data_offset.bytes();
		record.source = global.path;
		root_dir.push_back(record);
		lba.sectors += sector32::size_from_bytes(record.size).sectors;
	}
	// Note: This matches how the levels are laid out for R&C2 (AoS), but for
	// R&C3 and DL they're laid out on disc SoA, audio first.
	for(level_parts& level : level_files) {
		for(int i = 0; i < 3; i++) {
			if(level.parts[i]) {
				file_stream file(*level.parts[i]);
				level.data_offsets[i] = file.read<sector32>(0x4);
				level.data_lbas[i] = lba;
				level.sizes[i] = sector32::size_from_bytes(file.size() - level.data_offsets[i].bytes());
				
				iso_file_record record;
				record.name = level.parts[i]->filename().string() + ";1";
				record.lba = lba;
				record.size = level.sizes[i].bytes();
				record.source = *level.parts[i];
				root_dir.push_back(record);
				lba.sectors += level.sizes[i].sectors;
			}
		}
	}
	
	file_stream iso(iso_path, std::ios::out);
	
	printf("Writing ISO filesystem\n");
	write_iso_filesystem(iso, root_dir);
	
	iso.pad(SECTOR_SIZE, 0);
	static const uint8_t zeroed_sector[SECTOR_SIZE] = {0};
	while(iso.tell() < TABLE_OF_CONTENTS_LBA * SECTOR_SIZE) {
		iso.write_n((char*) zeroed_sector, SECTOR_SIZE);
	}
	
	// Write out the table of contents.
	printf("Writing table of contents at LBA %x\n", TABLE_OF_CONTENTS_LBA);
	for(global_file& global : global_files) {
		file_stream file(global.path);
		uint32_t size = file.read<uint32_t>();
		iso.write<uint32_t>(size);
		iso.write<sector32>(global.lba);
		file.seek(8);
		stream::copy_n(iso, file, size - 8);
	}
	std::vector<sector_range> level_table(level_files.size() * 3);
	size_t level_table_pos = iso.tell();
	defer([&]() {
		iso.seek(level_table_pos);
		iso.write_v(level_table);
	});
	iso.seek(iso.tell() + level_table.size() + sizeof(sector_range));
	for(size_t i = 0; i < level_files.size(); i++) {
		auto& level = level_files[i];
		for(int part = 0; part < 3; part++) {
			if(level.parts[part]) {
				uint32_t header[SECTOR_SIZE / 4]; // Assume the headers are all a single sector.
				file_stream file(*level.parts[part]);
				file.read_n((char*) header, SECTOR_SIZE);
				header[1] = level.data_lbas[part].sectors;
				
				iso.pad(SECTOR_SIZE, 0);
				level_table[i * 3 + part].offset.sectors = iso.tell() / SECTOR_SIZE;
				level_table[i * 3 + part].size = level.sizes[part];
				iso.write_n((char*) header, SECTOR_SIZE);
			}
		}
	}
	
	// Write out the rest of the files.
	for(const iso_file_record& record : root_dir) {
		file_stream file(record.source);
		uint32_t data_offset = 0;
		std::string name = str_to_lower(record.name);
		if(name.find(".wad") != std::string::npos) {
			// For the .WAD files the ToC header is added to the beginning of
			// the file.
			data_offset = file.read<sector32>(0x4).bytes();
		}
		file.seek(data_offset);
		iso.pad(SECTOR_SIZE, 0);
		assert(iso.tell() == record.lba.bytes());
		printf("Writing %s at LBA 0x%x\n", record.name.c_str(), record.lba.sectors);
		stream::copy_n(iso, file, file.size() - data_offset);
	}
	
	// Make sure we've written out the right amount of data.
	assert(iso.tell() == lba.bytes());
	
	// Write the volume size into the primary volume descriptor.
	iso.write<uint32_t>(0x8050, lba.sectors);
}

void enumerate_files_recursive(std::vector<fs::path>& files, fs::path dir, int depth) {
	if(depth > 10) {
		fprintf(stderr, "error: Directory depth limit (10 levels) reached!\n");
		exit(1);
	}
	for(const fs::directory_entry& file : fs::directory_iterator(dir)) {
		if(file.is_regular_file()) {
			if(files.size() > 1000) {
				fprintf(stderr, "error: File count limit (1000) reached!\n");
				exit(1);
			}
			files.push_back(file.path());
		} else if(file.is_directory()) {
			enumerate_files_recursive(files, file.path(), depth + 1);
		}
	}
}
