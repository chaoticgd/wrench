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

#include "iso_filesystem.h"

packed_struct(iso9660_i16_lsb_msb,
	int16_t lsb;
	int16_t msb;
)

packed_struct(iso9660_i32_lsb_msb,
	int32_t lsb;
	int32_t msb;
)

packed_struct(iso9660_datetime,
	uint8_t dont_care[17];
)

packed_struct(iso9660_directory_record,
	uint8_t record_length;
	uint8_t extended_attribute_record_length;
	iso9660_i32_lsb_msb lba;
	iso9660_i32_lsb_msb data_length;
	uint8_t recording_date_time[7];
	uint8_t file_flags;
	uint8_t file_unit_size;
	uint8_t interleave_gap_size;
	iso9660_i16_lsb_msb volume_sequence_number;
	uint8_t identifier_length;
	// Identifier follows.
)

static_assert(sizeof(iso9660_directory_record) == 0x21);

packed_struct(iso9660_primary_volume_desc,
	uint8_t type_code;
	char standard_identifier[5];
	uint8_t version;
	uint8_t unused_7;
	char system_identifier[32];
	char volume_identifier[32];
	uint8_t unused_48[8];
	iso9660_i32_lsb_msb volume_space_size;
	uint8_t unused_58[32];
	iso9660_i16_lsb_msb volume_set_size;
	iso9660_i16_lsb_msb volume_sequence_number;
	iso9660_i16_lsb_msb logical_block_size;
	iso9660_i32_lsb_msb path_table_size;
	int32_t l_path_table;
	int32_t optional_l_path_table;
	int32_t m_path_table;
	int32_t optional_m_path_table;
	iso9660_directory_record root_directory;
	uint8_t root_directory_pad;
	char volume_set_identifier[128];
	char publisher_identifier[128];
	char data_preparer_identifier[128];
	char application_identifier[128];
	char copyright_file_identifier[38];
	char abstract_file_identifier[36];
	char bibliographic_file_identifier[37];
	iso9660_datetime volume_creation_date_time;
	iso9660_datetime volume_modification_date_time;
	iso9660_datetime volume_expiration_date_time;
	iso9660_datetime volume_effective_date_time;
	int8_t file_structure_version;
	uint8_t unused_372;
	uint8_t application_used[512];
	uint8_t reserved[653];
)

static_assert(sizeof(iso9660_primary_volume_desc) == 0x800);

bool read_iso_filesystem(std::map<std::string, std::pair<size_t, size_t>>& dest, stream& iso) {
	auto pvd = iso.read<iso9660_primary_volume_desc>(0x10 * SECTOR_SIZE);
	if(pvd.type_code != 0x01) {
		return false;
	}
	if(memcmp(pvd.standard_identifier, "CD001", 5) != 0) {
		return false;
	}
	if(pvd.root_directory.data_length.lsb > 0x10000) {
		return false;
	}
	
	size_t root_directory_pos = pvd.root_directory.lba.lsb * SECTOR_SIZE;
	iso.seek(root_directory_pos);
	while(iso.tell() < root_directory_pos + pvd.root_directory.data_length.lsb) {
		size_t pos = iso.tell();
		auto record = iso.read<iso9660_directory_record>();
		if(record.record_length < 1) {
			break;
		}
		
		if(record.identifier_length >= 2) {
			std::string identifier(record.identifier_length - 2, '\0');
			iso.read_n(identifier.data(), identifier.size());
			size_t offset = record.lba.lsb * SECTOR_SIZE;
			size_t size = record.data_length.lsb;
			dest[identifier] = {offset, size};
		}
		
		iso.seek(pos + record.record_length);
	}
	
	
	
	return true;
}

std::map<std::string, size_t> write_iso_filesystem(stream& dest, const std::map<std::string, size_t>& file_sizes) {
	
}
