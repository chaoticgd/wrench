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

#ifndef ISO_FILESYSTEM_H
#define ISO_FILESYSTEM_H

#include <map>

#include <core/buffer.h>
#include <core/filesystem.h>
#include <core/stream.h>
#include <assetmgr/asset_types.h>

packed_struct(IsoLsbMsb16,
	s16 lsb;
	s16 msb;
	
	static IsoLsbMsb16 from_scalar(s16 lsb) {
		IsoLsbMsb16 result;
		result.lsb = lsb;
		result.msb = byte_swap_16(lsb);
		return result;
	}
)

packed_struct(IsoLsbMsb32,
	s32 lsb;
	s32 msb;
	
	static IsoLsbMsb32 from_scalar(s32 lsb) {
		IsoLsbMsb32 result;
		result.lsb = lsb;
		result.msb = byte_swap_32(lsb);
		return result;
	}
)

packed_struct(IsoPvdDateTime,
	char year[4];
	char month[2];
	char day[2];
	char hour[2];
	char minute[2];
	char second[2];
	char hundredths_of_a_second[2];
	s8 time_zone;
)

packed_struct(IsoDirectoryDateTime,
	u8 years_since_1900;
	u8 month;
	u8 day;
	u8 hour;
	u8 minute;
	u8 second;
	u8 time_zone;
)

packed_struct(IsoDirectoryRecord,
	u8 record_length;
	u8 extended_attribute_record_length;
	IsoLsbMsb32 lba;
	IsoLsbMsb32 data_length;
	IsoDirectoryDateTime recording_date_time;
	u8 file_flags;
	u8 file_unit_size;
	u8 interleave_gap_size;
	IsoLsbMsb16 volume_sequence_number;
	u8 identifier_length;
	// Identifier follows.
)
static_assert(sizeof(IsoDirectoryRecord) == 0x21);

packed_struct(IsoPrimaryVolumeDescriptor,
	u8 volume_descriptor_type;
	char standard_identifier[5];
	u8 volume_descriptor_version;
	u8 unused_7;
	char system_identifier[32];
	char volume_identifier[32];
	u8 unused_48[8];
	IsoLsbMsb32 volume_space_size;
	u8 unused_58[32];
	IsoLsbMsb16 volume_set_size;
	IsoLsbMsb16 volume_sequence_number;
	IsoLsbMsb16 logical_block_size;
	IsoLsbMsb32 path_table_size;
	s32 l_path_table;
	s32 optional_l_path_table;
	s32 m_path_table;
	s32 optional_m_path_table;
	IsoDirectoryRecord root_directory;
	u8 root_directory_pad;
	char volume_set_identifier[128];
	char publisher_identifier[128];
	char data_preparer_identifier[128];
	char application_identifier[128];
	char copyright_file_identifier[38];
	char abstract_file_identifier[36];
	char bibliographic_file_identifier[37];
	IsoPvdDateTime volume_creation_date_time;
	IsoPvdDateTime volume_modification_date_time;
	IsoPvdDateTime volume_expiration_date_time;
	IsoPvdDateTime volume_effective_date_time;
	s8 file_structure_version;
	u8 unused_372;
	u8 application_use[512];
	u8 reserved[653];
)
static_assert(sizeof(IsoPrimaryVolumeDescriptor) == 0x800);

struct IsoFileRecord
{
	std::string name;
	Sector32 lba;
	u32 size;
	const FileAsset* asset = nullptr;
	fs::file_time_type modified_time;
};

struct IsoDirectory
{
	std::string name;
	std::vector<IsoFileRecord> files;
	std::vector<IsoDirectory> subdirs;
	
	// Fields below used internally by write_iso_filesystem.
	IsoDirectory* parent = nullptr;
	size_t index = 0;
	size_t parent_index = 0;
	Sector32 lba = {0};
	u32 size = 0;
};

struct IsoFilesystem
{
	IsoPrimaryVolumeDescriptor pvd;
	IsoDirectory root;
};

static const s64 MAX_FILESYSTEM_SIZE_BYTES = 1500 * SECTOR_SIZE;

// Read an ISO filesystem and output the root dir. Call exit(1) on failure.
IsoFilesystem read_iso_filesystem(InputStream& src);

// Read an ISO filesystem and output a map (dest) of the files in the root
// directory. Return true on success, false on failure.
bool read_iso_filesystem(IsoFilesystem& dest, Buffer src);

// Given a list of files including their LBA and size, write out an ISO
// filesystem. This function is "dumb" in that it doesn't work out any positions
// by itself.
void write_iso_filesystem(OutputStream& dest, IsoDirectory* root_dir);

void print_file_record(const IsoFileRecord& record);

#endif
