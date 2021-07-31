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

#include "../editor/util.h"
#include "../editor/stream.h"
#include "../editor/fs_includes.h"
#include "../editor/command_line.h"
#include "iso_filesystem.h"
#include "table_of_contents.h"

// This is true for R&C2, R&C3 and Deadlocked.
static const uint32_t SYSTEM_CNF_LBA = 1000;
static const s64 MAX_FILESYSTEM_SIZE_BYTES = RAC1_TABLE_OF_CONTENTS_LBA * SECTOR_SIZE;
static const std::string RAC1_VOLUME_ID =
	"RATCHETANDCLANK                 ";

void ls(std::string iso_path);
void extract(std::string iso_path, fs::path output_dir);
void extract_non_wads_recursive(FILE* iso, fs::path out, iso_directory& in);
void build(std::string input_dir, fs::path iso_path, int single_level_index, bool no_mpegs);
void enumerate_wads_recursive(std::vector<fs::path>& wads, fs::path dir, int depth);
void enumerate_non_wads_recursive(stream& iso, iso_directory& out, fs::path dir, int depth);
void print_file_record(iso_file_record& record);
std::string str_to_lower(std::string str);
void parse_pcsx2_stdout(std::string iso_path);

// C format string for printing out LBA, size, filename lines.
const char* row_format;

int main(int argc, char** argv) {
	cxxopts::Options options(argv[0],
		"Extract files from and rebuild Ratchet & Clank ISO images. The games\n"
		"use raw disk I/O and a custom table of contents file to access assets\n"
		"so just writing a standard ISO filesystem won't work.");
	options.positional_help("ls|extract|build|parse_pcsx2_stdout <input path> [<output path>]");
	options.add_options()
		("c,command", "The operation to perform. Possible values are: ls, extract, build, parse_pcsx2_stdout.",
			cxxopts::value<std::string>())
		("i,input", "The input path.",
			cxxopts::value<std::string>())
		("o,output", "The output path.",
			cxxopts::value<std::string>())
		("d,decimal", "Print out the LBAs and sizes of files in decimal instead of hex.")
		("s,single-level",
			"Write out a single level, then point every other level at it.\n"
			"Much faster. Only applies for rebuilding.",
			cxxopts::value<int>())
		("n,no-mpegs", "Don't write out MPEG cutscenes. Much faster.");

	options.parse_positional({
		"command", "input", "output"
	});

	auto args = parse_command_line_args(argc, argv, options);
	std::string command = cli_get(args, "command");
	std::string input_path = cli_get(args, "input");
	std::string output_path = cli_get_or(args, "output", "");
	int single_level_index = -1; // -1 means write out all levels normally.
	if(args.count("single-level")) {
		single_level_index = args["single-level"].as<int>();
	}
	bool no_mpegs = args.count("no-mpegs");
	
	if(args.count("decimal")) {
		row_format = "%-16ld%-16ld%s\n";
	} else {
		row_format = "0x%-14lx0x%-14lx%s\n";
	}
	
	if(command == "ls") {
		ls(input_path);
	} else if(command == "extract") {
		extract(input_path, output_path);
	} else if(command == "build") {
		build(input_path, output_path, single_level_index, no_mpegs);
	} else if(command == "parse_pcsx2_stdout") {
		parse_pcsx2_stdout(input_path);
	} else {
		fprintf(stderr, "Invalid command: %s\n",  command.c_str());
		fprintf(stderr, "Pass --help for documentation.\n");
		exit(1);
	}
}

// Fun fact: This used to be its own command line tool called "toc". Now, it's
// been reduced to a humble subcommand within a greater tool. Pity it.
void ls(std::string iso_path) {
	FILE* iso = fopen(iso_path.c_str(), "rb");
	verify(iso, "Failed to open ISO file.");
	defer([&]() { fclose(iso); });
	
	table_of_contents toc = read_table_of_contents(iso);
	
	printf("+-[Non-level Sections]--+-------------+-------------+\n");
	printf("| Index | Offset in ToC | Size in ToC | Data Offset |\n");
	printf("| ----- | ------------- | ----------- | ----------- |\n");
	for(size_t i = 0; i < toc.tables.size(); i++) {
		toc_table& table = toc.tables[i];
		size_t base_offset = table.sector.bytes();
	printf("| %02ld    | %08x      | %08x    | %08lx    |\n",
		i, table.offset_in_toc, table.header.size(), base_offset);
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
	extract_non_wads_recursive(iso, output_dir, root_dir);
	
	// Extract levels and other asset files.
	table_of_contents toc;
	if(volume_id == RAC1_VOLUME_ID) {
		toc = read_table_of_contents_rac1(iso);
	} else {
		toc = read_table_of_contents(iso);
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
		
		extract_file(path, iso, table.sector.bytes(), file_size);
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
			
			extract_file(path, iso, part->file_lba.bytes(), part->file_size.bytes());
		}
	}
}

void extract_non_wads_recursive(FILE* iso, fs::path out, iso_directory& in) {
	for(iso_file_record& file : in.files) {
		fs::path file_path = out/file.name.substr(0, file.name.size() - 2);
		if(file_path.string().find(".wad") == std::string::npos) {
			print_file_record(file);
			extract_file(file_path, iso, file.lba.bytes(), file.size);
		}
	}
	for(iso_directory& subdir : in.subdirs) {
		auto dir_path = out/subdir.name;
		fs::create_directory(dir_path);
		extract_non_wads_recursive(iso, out/subdir.name, subdir);
	}
}

struct global_file {
	fs::path path;
	Sector32 lba;
	bool operator<(global_file& rhs) { return path < rhs.path; }
};

// Note: Files aren't necessarily written out in this order. The structure of
// the level table depends on the game!
const size_t LEVEL_PART = 0;
const size_t AUDIO_PART = 1;
const size_t SCENE_PART = 2;
struct level_parts {
	std::optional<fs::path> parts[3];
	uint32_t header_sizes_in_sectors[3];
};

void build(std::string input_dir, fs::path iso_path, int single_level_index, bool no_mpegs) {
	if(!fs::is_directory(input_dir)) {
		fprintf(stderr, "error: Input path is not a directory!\n");
		exit(1);
	}
	if(iso_path == "") {
		fprintf(stderr, "error: No output path specified!\n");
		exit(1);
	}
	if(!fs::is_directory(iso_path.parent_path())) {
		fprintf(stderr, "error: Parent directory of output file does not exist!\n");
		exit(1);
	}
	if(fs::is_directory(iso_path)) {
		fprintf(stderr, "error: Output path is a directory!\n");
		exit(1);
	}
	
	std::vector<fs::path> wad_files;
	enumerate_wads_recursive(wad_files, input_dir, 0);
	
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
					fprintf(stderr, "error: Unable to convert level index to integer.\n");
					exit(1);
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
	std::sort(global_files.begin(), global_files.end(),
		[](global_file& l, global_file& r) { return l.path.filename() < r.path.filename(); });
	
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
	
	int game = GAME_ANY;
	
	// Read the magic identifier from each of the level files and determine the
	// game we're working with. Yes I'm opening each file to read just 4 bytes.
	for(level_parts& level : level_files) {
		for(int i = 0; i < 3; i++) {
			if(!level.parts[i]) {
				continue;
			}
			uint32_t magic = file_stream(level.parts[i]->string()).read<uint32_t>();
			auto info = LEVEL_FILE_TYPES.find(magic);
			if(info == LEVEL_FILE_TYPES.end()) {
				fprintf(stderr, "error: File '%s' has invalid header!\n", level.parts[i]->filename().c_str());
				exit(1);
			}
			level.header_sizes_in_sectors[i] = Sector32::size_from_bytes(magic).sectors;
			game &= info->second.game;
		}
	}
	switch(game) {
		case GAME_RAC1:
			printf("Detected game: Ratchet & Clank 1\n\n");
			break;
		case GAME_RAC2:
			printf("Detected game: Ratchet & Clank 2\n\n");
			break;
		case GAME_RAC3:
			printf("Detected game: Ratchet & Clank 3\n\n");
			break;
		case GAME_RAC4:
			printf("Detected game: Ratchet: Deadlocked\n\n");
			break;
		case GAME_RAC2_OTHER:
			printf("Detected game: Ratchet & Clank 2 Other\n\n");
			break;
		default:
			fprintf(stderr, "warning: Unable to detect game! Assuming Ratchet & Clank 2...\n");
			game = GAME_RAC2;
	}
	
	// Calculate the size of the table of contents file so we can determine the
	// LBAs of all the files that come after it.
	size_t global_toc_size_bytes = 0;
	for(global_file& global : global_files) {
		file_stream file(global.path.string());
		uint32_t size = file.read<uint32_t>();
		if(size > 0xffff) {
			fprintf(stderr, "error: File '%s' has a header size > 0xffff bytes.\n", global.path.filename().c_str());
			exit(1);
		}
		global_toc_size_bytes += size;
	}
	global_toc_size_bytes += level_files.size() * sizeof(sector_range) * 3;
	Sector32 global_toc_size = Sector32::size_from_bytes(global_toc_size_bytes);
	Sector32 total_toc_size = global_toc_size;
	if(single_level_index > -1) {
		if(single_level_index >= (int) level_files.size()) {
			fprintf(stderr, "error: Single level index greater than maximum level index!\n");
			exit(1);
		}
		if(!level_files[single_level_index].parts[LEVEL_PART]) {
			fprintf(stderr, "error: No level file with index %d.\n", single_level_index);
			exit(1);
		}
		for(int part = 0; part < 3; part++) {
			auto& level = level_files[single_level_index];
			if(level.parts[part]) {
				total_toc_size.sectors += level.header_sizes_in_sectors[part] * level_files.size();
			}
		}
	} else {
		for(auto& level : level_files) {
			for(int part = 0; part < 3; part++) {
				if(level.parts[part]) {
					total_toc_size.sectors += level.header_sizes_in_sectors[part];
				}
			}
		}
	}
	
	// After all the other files have been written out, write out an ISO
	// filesystem at the beginning of the image.
	file_stream iso(iso_path.string(), std::ios::out);
	iso_directory root_dir;
	uint32_t volume_size = 0;
	defer([&]() {
		iso.seek(0);
		write_iso_filesystem(iso, &root_dir);
		assert(iso.tell() <= RAC234_TABLE_OF_CONTENTS_LBA * SECTOR_SIZE);
		
		iso.write<uint32_t>(0x8050, volume_size);
	});
	
	// After all the other files have been written out, write out the table of
	// contents file at its hardcoded position.
	std::vector<toc_table> toc_tables;
	std::vector<toc_level> toc_levels;
	defer([&]() {
		iso.seek(RAC234_TABLE_OF_CONTENTS_LBA * SECTOR_SIZE);
		for(toc_table& table : toc_tables) {
			*(Sector32*) &table.header[4] = table.sector;
			iso.write_v(table.header);
			*(Sector32*) &table.header[4] = {0};
		}
		
		size_t level_table_pos = iso.tell();
		std::vector<sector_range> level_table(toc_levels.size() * 3, {{0}, {0}});
		defer([&]() {
			iso.seek(level_table_pos);
			iso.write_v(level_table);
		});
		iso.seek(iso.tell() + level_table.size() * sizeof(sector_range));
		
		size_t toc_start_size_bytes = iso.tell() - RAC234_TABLE_OF_CONTENTS_LBA * SECTOR_SIZE;
		Sector32 toc_start_size = Sector32::size_from_bytes(toc_start_size_bytes);
		
		// Size limits hardcoded in the boot ELF.
		switch(game) {
			case GAME_RAC2: assert(toc_start_size.sectors <= 0xb); break;
			case GAME_RAC3: assert(toc_start_size.sectors <= 0x10); break;
			case GAME_RAC4: assert(toc_start_size.sectors <= 0x1a); break;
		}
		
		for(size_t i = 0; i < toc_levels.size(); i++) {
			toc_level& level = toc_levels[i]; 
			for(std::optional<toc_level_part>& part : level.parts) {
				if(!part) {
					continue;
				}
				
				iso.pad(SECTOR_SIZE, 0);
				uint32_t header_lba = iso.tell() / SECTOR_SIZE;
				*(Sector32*) &part->header[4] = part->file_lba;
				iso.write_v(part->header);
				*(Sector32*) &part->header[4] = {0};
				iso.pad(SECTOR_SIZE, 0);
				
				// The order of fields in the level table entries is different
				// for R&C2 versus R&C3 and Deadlocked.
				size_t field = 0;
				bool is_rac2 = game == GAME_RAC2 || game == GAME_RAC2_OTHER;
				switch(part->info.type) {
					case level_file_type::AUDIO: field = is_rac2; break;
					case level_file_type::LEVEL: field = !is_rac2; break;
					case level_file_type::SCENE: field = 2; break;
				}
				size_t index = i * 3 + field;
				level_table[index].offset.sectors = header_lba;
				level_table[index].size = part->file_size;
			}
		}
		
		assert(iso.tell() <= RAC234_TABLE_OF_CONTENTS_LBA * SECTOR_SIZE + total_toc_size.bytes());
	});
	
	// Write out blank sectors that are to be filled in later.
	iso.pad(SECTOR_SIZE, 0);
	static const uint8_t zeroed_sector[SECTOR_SIZE] = {0};
	while(iso.tell() < SYSTEM_CNF_LBA * SECTOR_SIZE) {
		iso.write_n((char*) zeroed_sector, SECTOR_SIZE);
	}
	
	printf("LBA             Size (bytes)    Filename\n");
	printf("---             ------------    --------\n");
	
	// SYSTEM.CNF must come first at LBA 1000.
	{
		// Find SYSTEM.CNF
		fs::path system_cnf_path;
		for(fs::path path : fs::directory_iterator(input_dir)) {
			if(str_to_lower(path.filename().string()) == "system.cnf") {
				system_cnf_path = path;
			}
		}
		if(system_cnf_path.empty()) {
			fprintf(stderr, "error: No SYSTEM.CNF file in input directory!\n");
			exit(1);
		}
		
		file_stream system_cnf(system_cnf_path.string());
		size_t system_cnf_size = system_cnf.size();
		
		iso_file_record record;
		record.name = "system.cnf;1";
		record.lba = {(s32) (iso.tell() / SECTOR_SIZE)};
		record.size = system_cnf_size;
		root_dir.files.push_back(record);
		
		print_file_record(record);
		
		stream::copy_n(iso, system_cnf, system_cnf_size);
	}
	// Then the table of contents at LBA 1001.
	{
		iso.pad(SECTOR_SIZE, 0);
		iso_file_record toc_record;
		switch(game) {
			case GAME_RAC1: toc_record.name = "rc1.hdr;1"; break;
			case GAME_RAC2: toc_record.name = "rc2.hdr;1"; break;
			case GAME_RAC3: toc_record.name = "rc3.hdr;1"; break;
			case GAME_RAC4: toc_record.name = "rc4.hdr;1"; break;
			case GAME_RAC2_OTHER: toc_record.name = "rc2.hdr;1"; break;
		}
		toc_record.lba = {RAC234_TABLE_OF_CONTENTS_LBA};
		toc_record.size = total_toc_size.bytes();
		root_dir.files.push_back(toc_record);
		print_file_record(toc_record);
	}
	// Write out blank sectors that are to be filled in by the table of contents later.
	iso.pad(SECTOR_SIZE, 0);
	assert(iso.tell() == RAC234_TABLE_OF_CONTENTS_LBA * SECTOR_SIZE);
	while(iso.tell() < RAC234_TABLE_OF_CONTENTS_LBA * SECTOR_SIZE + total_toc_size.bytes()) {
		iso.write_n((char*) zeroed_sector, SECTOR_SIZE);
	}
	// Then various other files e.g. the boot ELF, etc.
	enumerate_non_wads_recursive(iso, root_dir, input_dir, 0);
	// Then the global files e.g. MISC.WAD, MPEG.WAD, ARMOR.WAD, etc.
	iso_directory global_dir {"global"};
	for(global_file& global : global_files) {
		file_stream file(global.path.string());
		iso.pad(SECTOR_SIZE, 0);
		
		toc_table table;
		table.index = 0; // Don't care.
		table.offset_in_toc = 0; // Don't care.
		s64 header_size = file.read<s32>(0);
		table.sector = {(s32) (iso.tell() / SECTOR_SIZE)};
		table.header.resize(header_size);
		file.seek(0);
		file.read_v(table.header);
		
		bool skipped = false;
		if(no_mpegs) {
			// Detect if the current file is the one that contains all the MPEG
			// cutscenes. If it is, null out the header and omit the contents.
			// The MPEG file is the only one that stores some of the lump sizes
			// in bytes instead of sectors, so if the lump sizes are large, we
			// know that it's probably in bytes intead of sectors and hence this
			// file must be the MPEG file.
			size_t lump_sizes_probably_in_bytes = 0;
			size_t lump_count = (table.header.size() - 8) / 8;
			Buffer header_buf(table.header);
			auto lumps = header_buf.read_multiple<sector_range>(8, lump_count, "global header");
			for(sector_range range : lumps) {
				if(range.size.sectors > 0xffff) {
					lump_sizes_probably_in_bytes++;
				}
			}
			// Arbitrary threshold.
			if(lump_sizes_probably_in_bytes > 10) {
				for(size_t i = 8; i < table.header.size(); i++) {
					table.header[i] = 0;
				}
				skipped = true;
			}
		}
		
		toc_tables.push_back(table);
		
		if(!skipped) {
			iso_file_record record;
			record.name = global.path.filename().string() + ";1";
			record.lba = table.sector;
			record.size = file.size();
			global_dir.files.push_back(record);
			
			print_file_record(record);
			
			file.seek(0);
			stream::copy_n(iso, file, record.size);
		}
	}
	// Then the level files.
	root_dir.subdirs.push_back(global_dir);
	auto write_level_part = [&](iso_directory& parent, fs::path& path) {
		file_stream file(path.string());
		iso.pad(SECTOR_SIZE, 0);
		
		size_t file_size = file.size();
		
		toc_level_part part;
		part.header_lba = {0}; // Don't care.
		part.file_size = Sector32::size_from_bytes(file_size);
		part.magic = file.read<uint32_t>(0);
		part.file_lba = {(s32) (iso.tell() / SECTOR_SIZE)};
		auto info = LEVEL_FILE_TYPES.find(part.magic);
		if(info == LEVEL_FILE_TYPES.end()) {
			fprintf(stderr, "error: Level '%s' has an invalid header!\n", path.filename().c_str());
			exit(1);
		}
		part.info = info->second;
		size_t header_size = Sector32::size_from_bytes(part.magic).bytes();
		part.header.resize(header_size);
		file.seek(0);
		file.read_v(part.header);
		
		iso_file_record record;
		record.name = path.filename().string() + ";1";
		record.lba = part.file_lba;
		record.size = file_size;
		parent.files.push_back(record);
		
		print_file_record(record);
		
		iso.pad(SECTOR_SIZE, 0);
		file.seek(0);
		stream::copy_n(iso, file, file_size);
		
		return part;
	};
	// Create directories for the level files.
	iso_directory levels_dir {"levels"};
	iso_directory audio_dir {"audio"};
	iso_directory scenes_dir {"scenes"};
	toc_levels.resize(level_files.size());
	if(single_level_index > -1) {
		// Only write out a single level, and point every level at it.
		auto& p = level_files[single_level_index].parts;
		toc_level_part level_part, audio_part, scene_part;
		assert(p[LEVEL_PART]);
		level_part = write_level_part(levels_dir, *p[LEVEL_PART]);
		if(p[AUDIO_PART]) audio_part = write_level_part(audio_dir, *p[AUDIO_PART]);
		if(p[SCENE_PART]) scene_part = write_level_part(scenes_dir, *p[SCENE_PART]);
		for(size_t i = 0; i < level_files.size(); i++) {
			toc_levels[i].parts[0] = level_part;
			if(p[AUDIO_PART]) toc_levels[i].parts[1] = audio_part;
			if(p[SCENE_PART]) toc_levels[i].parts[2] = scene_part;
		}
	} else if(game == GAME_RAC2 || game == GAME_RAC2_OTHER) {
		// The level files are laid out AoS.
		for(size_t i = 0; i < level_files.size(); i++) {
			level_parts& level = level_files[i];
			auto& p = level.parts;
			if(p[LEVEL_PART]) toc_levels[i].parts[0] = write_level_part(levels_dir, *p[LEVEL_PART]);
			if(p[AUDIO_PART]) toc_levels[i].parts[1] = write_level_part(audio_dir, *p[AUDIO_PART]);
			if(p[SCENE_PART]) toc_levels[i].parts[2] = write_level_part(scenes_dir, *p[SCENE_PART]);
		}
	} else {
		// The level files are laid out SoA, audio files first.
		for(size_t i = 0; i < level_files.size(); i++) {
			level_parts& level = level_files[i];
			if(level.parts[AUDIO_PART]) {
				toc_levels[i].parts[0] = write_level_part(audio_dir, *level.parts[AUDIO_PART]);
			}
		}
		for(size_t i = 0; i < level_files.size(); i++) {
			level_parts& level = level_files[i];
			if(level.parts[LEVEL_PART]) {
				toc_levels[i].parts[1] = write_level_part(levels_dir, *level.parts[LEVEL_PART]);
			}
		}
		for(size_t i = 0; i < level_files.size(); i++) {
			level_parts& level = level_files[i];
			if(level.parts[SCENE_PART]) {
				toc_levels[i].parts[2] = write_level_part(scenes_dir, *level.parts[SCENE_PART]);
			}
		}
	}
	root_dir.subdirs.push_back(levels_dir);
	root_dir.subdirs.push_back(audio_dir);
	root_dir.subdirs.push_back(scenes_dir);
	
	iso.pad(SECTOR_SIZE, 0);
	volume_size = iso.tell() / SECTOR_SIZE;
}

void enumerate_wads_recursive(std::vector<fs::path>& wads, fs::path dir, int depth) {
	if(depth > 10) {
		fprintf(stderr, "error: Directory depth limit (10 levels) reached!\n");
		exit(1);
	}
	for(const fs::directory_entry& file : fs::directory_iterator(dir)) {
		auto name = str_to_lower(file.path().filename().string());
		if(file.is_regular_file() && name.find(".wad") != std::string::npos) {
			if(wads.size() > 1000) {
				fprintf(stderr, "error: File count limit (1000) reached!\n");
				exit(1);
			}
			wads.push_back(file.path());
		} else if(file.is_directory()) {
			enumerate_wads_recursive(wads, file.path(), depth + 1);
		}
	}
}

void enumerate_non_wads_recursive(stream& iso, iso_directory& out, fs::path dir, int depth) {
	for(const fs::directory_entry& entry : fs::directory_iterator(dir)) {
		auto name = str_to_lower(entry.path().filename().string());
		if(entry.is_regular_file()) {
			if(name.find(".wad") != std::string::npos) {
				// WAD files are handled by enumerate_wads_recursive.
				continue;
			}
			
			if(name.find(".hdr") != std::string::npos) {
				// We're writing out a new table of contents, so if an old one
				// already exists we don't want to write it out.
				continue;
			}
			
			if(name.find("system.cnf") == 0) {
				// SYSTEM.CNF must be written out at a fixed LBA so we handle
				// it seperately.
				continue;
			}
			
			if(name.find(".iso") != std::string::npos) {
				continue;
			}
			
			file_stream file(entry.path().string());
			iso.pad(SECTOR_SIZE, 0);
			
			size_t file_size = file.size();
			
			iso_file_record record;
			record.name = name + ";1";
			record.lba = {(s32) (iso.tell() / SECTOR_SIZE)};
			record.size = file_size;
			out.files.push_back(record);
			print_file_record(record);
			
			stream::copy_n(iso, file, file_size);
		} else if(entry.is_directory()) {
			iso_directory subdir {name};
			// Prevent name collisions with the auto-generated directories.
			if(depth == 0 && (name == "global" || name == "levels" || name == "audio" || name == "scenes")) {
				continue;
			}
			enumerate_non_wads_recursive(iso, subdir, entry.path(), depth + 1);
			if(subdir.files.size() > 0 || subdir.subdirs.size() > 0) {
				out.subdirs.push_back(subdir);
			}
		}
	}
}

void print_file_record(iso_file_record& record) {
	assert(record.name.size() >= 2);
	auto display_name = record.name.substr(0, record.name.size() - 2);
	printf(row_format, (size_t) record.lba.sectors, (size_t) record.size, display_name.c_str());
}

std::string str_to_lower(std::string str) {
	for(char& c : str) {
		c = tolower(c);
	}
	return str;
}

void parse_pcsx2_stdout(std::string iso_path) {
	FILE* iso = fopen(iso_path.c_str(), "rb");
	verify(iso, "Failed to open ISO file for reading.");
	std::vector<u8> filesystem_buf = read_file(iso, 0, MAX_FILESYSTEM_SIZE_BYTES);
	fclose(iso);
	
	// First we enumerate where all the files on the ISO are. Note that this
	// command only works for stuff referenced by the filesystem.
	std::vector<iso_file_record> files;
	iso_directory root_dir;
	root_dir.files.push_back({"primary volume descriptor", 0x10, SECTOR_SIZE});
	std::string dummy_volume_id;
	if(!read_iso_filesystem(root_dir, dummy_volume_id, filesystem_buf)) {
		fprintf(stderr, "error: Failed to read ISO filesystem!\n");
		exit(1);
	}
	std::function<void(iso_directory&)> enumerate_dir = [&](iso_directory& dir) {
		for(iso_directory& subdir : dir.subdirs) {
			enumerate_dir(subdir);
		}
		files.insert(files.end(), dir.files.begin(), dir.files.end());
	};
	enumerate_dir(root_dir);
	
	const char* before_text = "DvdRead: Reading Sector ";
	
	auto file_from_lba = [&](size_t lba) -> iso_file_record* {
		for(iso_file_record& file : files) {
			size_t end_lba = file.lba.sectors + Sector32::size_from_bytes(file.size).sectors;
			if(lba >= file.lba.sectors && lba < end_lba) {
				return &file;
			}
		}
		return nullptr;
	};
	
	// If we get a line reporting a sector read from PCSX2, determine which file
	// is being read and print out its name.
	iso_file_record* last_file = nullptr;
	size_t last_lba = SIZE_MAX;
	std::string line;
	while(std::getline(std::cin, line)) {
		size_t before_pos = line.find(before_text);
		if(line.find(before_text) != std::string::npos) {
			line = line.substr(before_pos + strlen(before_text));
			if(line.find(" ") == std::string::npos) {
				continue;
			}
			line = line.substr(0, line.find(" "));
			size_t lba;
			try {
				lba = std::stoi(line);
			} catch(std::logic_error& e) {
				continue;
			}
			iso_file_record* file = file_from_lba(lba);
			if(lba > last_lba && lba <= last_lba + 0x10 && file == last_file) {
				// Don't spam stdout with every new sector that needs to be read
				// in. Only print when it's reading a different file, or it
				// seeks to a different position.
				last_lba = lba;
				continue;
			} else if(last_lba != SIZE_MAX) {
				printf(" ... 0x%lx abs 0x%lx\n", last_lba - (last_file ? last_file->lba.sectors : 0), last_lba);
			}
			if(file) {
				printf("%8lx %32s + 0x%lx", lba, file->name.c_str(), lba - file->lba.sectors);
			} else {
				printf("%8lx %32s + 0x%lx", lba, "(unknown)", lba);
			}
			last_lba = lba;
			last_file = file;
		}
	}
	if(last_lba != SIZE_MAX) {
		printf(" ... 0x%lx abs 0x%lx\n", last_lba - (last_file ? last_file->lba.sectors : 0), last_lba);
	}
}
