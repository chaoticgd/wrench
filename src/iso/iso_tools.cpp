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

#include "iso_tools.h"

#include <iostream>
#include <functional>

#include <core/filesystem.h>
#include <iso/iso_filesystem.h>
#include <iso/table_of_contents.h>
#include <iso/iso_unpacker.h>
#include <iso/iso_packer.h>
#include <iso/wad_identifier.h>

// Fun fact: This used to be its own command line tool called "toc". Now, it's
// been reduced to a humble subcommand within a greater tool. Pity it.
void inspect_iso(const std::string& iso_path)
{
	FileInputStream iso;
	verify(iso.open(iso_path), "Failed to open ISO file (%s).", iso.last_error.c_str());
	
	IsoFilesystem filesystem = read_iso_filesystem(iso);
	
	static const char* RAC1_VOLUME_ID = "RATCHETANDCLANK                 ";
	
	table_of_contents toc;
	if (memcmp(filesystem.pvd.volume_identifier, RAC1_VOLUME_ID, 32) == 0) {
		toc = read_table_of_contents_rac(iso);
	} else {
		toc = read_table_of_contents_rac234(iso);
		if (toc.levels.size() == 0) {
			fprintf(stderr, "error: Unable to locate level table!\n");
			exit(1);
		}
	}
	
	printf("+-[Global WADs]---------+-------------+-------------+\n");
	printf("| Index | Offset in ToC | Size in ToC | Data Offset |\n");
	printf("| ----- | ------------- | ----------- | ----------- |\n");
	for (s32 i = 0; i < (s32) toc.globals.size(); i++) {
		GlobalWadInfo& global = toc.globals[i];
		size_t base_offset = global.sector.bytes();
	printf("| %02d    | %08x      | %08x    | %08lx    |\n",
		i, global.offset_in_toc, (s32) global.header.size(), base_offset);
	}
	printf("+-------+---------------+-------------+-------------+\n");
	
	printf("+-[Level Table]---+------------------------+------------------------+------------------------+\n");
	printf("|                 | LEVELn.WAD             | AUDIOn.WAD             | SCENEn.WAD             |\n");
	printf("|                 | ----------             | ----------             | ----------             |\n");
	printf("| Index  Entry    | Offset      Size       | Offset      Size       | Offset      Size       |\n");
	printf("| -----  -----    | ------      ----       | ------      ----       | ------      ----       |\n");
	for (LevelInfo& level : toc.levels) {
		printf("| %03d    %08x |", level.level_table_index, level.level_table_entry_offset);
		for (const Opt<LevelWadInfo>& wad : {level.level, level.audio, level.scene}) {
			if (wad) {
				printf(" %010lx  %010lx |", wad->file_lba.bytes(), wad->file_size.bytes());
			} else {
				printf(" N/A         N/A        |");
			}
		}
		printf("\n");
	}
	printf("+-----------------+------------------------+------------------------+------------------------+\n");
}

void parse_pcsx2_cdvd_log(std::string iso_path)
{
	FileInputStream iso;
	verify(iso.open(iso_path), "Failed to open ISO file.");
	
	// First we enumerate where all the files on the ISO are. Note that this
	// command only works for stuff referenced by the filesystem.
	std::vector<IsoFileRecord> files;
	files.push_back({"primary volume descriptor", 0x10, SECTOR_SIZE});
	IsoFilesystem filesystem = read_iso_filesystem(iso);
	std::function<void(IsoDirectory&)> enumerate_dir = [&](IsoDirectory& dir) {
		for (IsoDirectory& subdir : dir.subdirs) {
			enumerate_dir(subdir);
		}
		files.insert(files.end(), dir.files.begin(), dir.files.end());
	};
	enumerate_dir(filesystem.root);
	
	const char* before_text = "DvdRead: Reading Sector ";
	
	auto file_from_lba = [&](size_t lba) -> IsoFileRecord* {
		for (IsoFileRecord& file : files) {
			size_t end_lba = file.lba.sectors + Sector32::size_from_bytes(file.size).sectors;
			if (lba >= file.lba.sectors && lba < end_lba) {
				return &file;
			}
		}
		return nullptr;
	};
	
	// If we get a line reporting a sector read from PCSX2, determine which file
	// is being read and print out its name.
	IsoFileRecord* last_file = nullptr;
	size_t last_lba = SIZE_MAX;
	std::string line;
	while (std::getline(std::cin, line)) {
		size_t before_pos = line.find(before_text);
		if (line.find(before_text) != std::string::npos) {
			line = line.substr(before_pos + strlen(before_text));
			if (line.find(" ") == std::string::npos) {
				continue;
			}
			line = line.substr(0, line.find(" "));
			size_t lba;
			try {
				lba = std::stoi(line);
			} catch (std::logic_error& e) {
				continue;
			}
			IsoFileRecord* file = file_from_lba(lba);
			if (lba > last_lba && lba <= last_lba + 0x10 && file == last_file) {
				// Don't spam stdout with every new sector that needs to be read
				// in. Only print when it's reading a different file, or it
				// seeks to a different position.
				last_lba = lba;
				continue;
			} else if (last_lba != SIZE_MAX) {
				printf(" ... 0x%lx abs 0x%lx\n", last_lba - (last_file ? last_file->lba.sectors : 0), last_lba);
			}
			if (file) {
				printf("%8lx %32s + 0x%lx", lba, file->name.c_str(), lba - file->lba.sectors);
			} else {
				printf("%8lx %32s + 0x%lx", lba, "(unknown)", lba);
			}
			last_lba = lba;
			last_file = file;
		}
	}
	if (last_lba != SIZE_MAX) {
		printf(" ... 0x%lx abs 0x%lx\n", last_lba - (last_file ? last_file->lba.sectors : 0), last_lba);
	}
}
