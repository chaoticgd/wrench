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

#ifndef ISO_ISO_EXTRACTOR_H
#define ISO_ISO_EXTRACTOR_H

#include <core/util.h>
#include <core/filesystem.h>
#include <iso/table_of_contents.h>

// This is true for R&C2, R&C3 and Deadlocked.
static const uint32_t SYSTEM_CNF_LBA = 1000;

void extract_iso(const fs::path& output_dir, const std::string& iso_path, const char* row_format);

#endif
