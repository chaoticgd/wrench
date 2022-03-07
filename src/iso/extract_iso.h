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

#ifndef ISO_EXTRACT_ISO_H
#define ISO_EXTRACT_ISO_H

#include <core/util.h>
#include <core/filesystem.h>
#include <iso/table_of_contents.h>

// This is true for R&C2, R&C3 and Deadlocked.
static const uint32_t SYSTEM_CNF_LBA = 1000;
static const s64 MAX_FILESYSTEM_SIZE_BYTES = RAC1_TABLE_OF_CONTENTS_LBA * SECTOR_SIZE;
static const std::string RAC1_VOLUME_ID = "RATCHETANDCLANK                 ";

void extract_iso(std::string iso_path, fs::path output_dir, const char* row_format);

#endif
