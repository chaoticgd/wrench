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

#include "../util.h"

uint16_t byte_swap_16(uint16_t val) {
	return (val >> 8) | (val << 8);
}

uint32_t byte_swap_32(uint32_t val) {
	uint32_t swapped = 0;
	swapped |= (val >> 24) & 0xff;
	swapped |= (val << 8) & 0xff0000;
	swapped |= (val >> 8) & 0xff00;
	swapped |= (val << 24) & 0xff000000;
	return swapped;
}

packed_struct(iso9660_i16_lsb_msb,
	int16_t lsb;
	int16_t msb;
	
	static iso9660_i16_lsb_msb from_scalar(int16_t lsb) {
		iso9660_i16_lsb_msb result;
		result.lsb = lsb;
		result.msb = byte_swap_16(lsb);
		return result;
	}
)

packed_struct(iso9660_i32_lsb_msb,
	int32_t lsb;
	int32_t msb;
	
	static iso9660_i32_lsb_msb from_scalar(int32_t lsb) {
		iso9660_i32_lsb_msb result;
		result.lsb = lsb;
		result.msb = byte_swap_32(lsb);
		return result;
	}
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

// We're only worried about the root directory.
packed_struct(iso9660_path_table_entry,
	uint8_t identifier_length;
	uint8_t record_length;
	uint32_t lba;
	uint16_t parent;
	char identifier;
	char pad;
)

bool read_iso_filesystem(std::vector<iso_file_record>& dest, stream& iso) {
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
			iso_file_record file;
			file.name.resize(record.identifier_length);
			iso.read_n(file.name.data(), file.name.size());
			file.lba = {(uint32_t) record.lba.lsb};
			file.size = record.data_length.lsb;
			dest.push_back(file);
		}
		
		iso.seek(pos + record.record_length);
	}
	
	
	
	return true;
}

void copy_and_pad(char* dest, const char* src, size_t size) {
	size_t i;
	for(i = 0; i < strlen(src); i++) {
		dest[i] = src[i];
	}
	for(; i < size; i++) {
		dest[i] = ' ';
	}
}

std::map<std::string, size_t> write_iso_filesystem(stream& dest, const std::vector<iso_file_record>& files) {
	// Write out system area.
	static const uint8_t zeroed_sector[SECTOR_SIZE] = {0};
	for(int i = 0; i < 0x10; i++) {
		dest.write_n((char*) zeroed_sector, sizeof(zeroed_sector));
	}
	
	iso9660_datetime zeroed_datetime = {{0}};
	
	// Write out primary volume descriptor.
	size_t pvd_pos = dest.tell();
	iso9660_primary_volume_desc pvd;
	defer([&]() {
		dest.seek(pvd_pos);
		dest.write(pvd);
		dest.seek(dest.size());
	});
	dest.seek(dest.tell() + sizeof(pvd));
	pvd.type_code = 0x01;
	memcpy(pvd.standard_identifier, "CD001", sizeof(pvd.standard_identifier));
	pvd.version = 1;
	pvd.unused_7 = 0;
	copy_and_pad(pvd.system_identifier, "WRENCH", sizeof(pvd.system_identifier));
	copy_and_pad(pvd.volume_identifier, "WRENCH", sizeof(pvd.volume_identifier));
	memset(pvd.unused_48, 0, 8);
	pvd.volume_space_size = iso9660_i32_lsb_msb::from_scalar(0);
	memset(pvd.unused_58, 0, 32);
	pvd.volume_set_size = iso9660_i16_lsb_msb::from_scalar(1);
	pvd.volume_sequence_number = iso9660_i16_lsb_msb::from_scalar(1);
	pvd.logical_block_size = iso9660_i16_lsb_msb::from_scalar(SECTOR_SIZE);
	pvd.path_table_size = iso9660_i32_lsb_msb::from_scalar(sizeof(iso9660_path_table_entry));
	pvd.l_path_table = 0;
	pvd.optional_l_path_table = 0;
	pvd.m_path_table = 0;
	pvd.optional_m_path_table = 0;
	pvd.root_directory.record_length = 0x22;
	pvd.root_directory.extended_attribute_record_length = 0;
	memset(pvd.root_directory.recording_date_time, 0, sizeof(pvd.root_directory.recording_date_time));
	pvd.root_directory.file_flags = 2;
	pvd.root_directory.file_unit_size = 0;
	pvd.root_directory.interleave_gap_size = 0;
	pvd.root_directory.volume_sequence_number = iso9660_i16_lsb_msb::from_scalar(1);
	pvd.root_directory.identifier_length = 1;
	pvd.root_directory_pad = 0;
	copy_and_pad(pvd.volume_set_identifier, "WRENCH", sizeof(pvd.volume_set_identifier));
	copy_and_pad(pvd.publisher_identifier, "WRENCH", sizeof(pvd.publisher_identifier));
	copy_and_pad(pvd.data_preparer_identifier, "WRENCH", sizeof(pvd.data_preparer_identifier));
	copy_and_pad(pvd.application_identifier, "WRENCH", sizeof(pvd.application_identifier));
	copy_and_pad(pvd.copyright_file_identifier, "WRENCH", sizeof(pvd.copyright_file_identifier));
	copy_and_pad(pvd.abstract_file_identifier, "WRENCH", sizeof(pvd.abstract_file_identifier));
	copy_and_pad(pvd.bibliographic_file_identifier, "WRENCH", sizeof(pvd.bibliographic_file_identifier));
	pvd.volume_creation_date_time = zeroed_datetime;
	pvd.volume_modification_date_time = zeroed_datetime;
	pvd.volume_expiration_date_time = zeroed_datetime;
	pvd.volume_effective_date_time = zeroed_datetime;
	pvd.file_structure_version = 1;
	pvd.unused_372 = 0;
	memset(pvd.application_used, 0, sizeof(pvd.application_used));
	memset(pvd.reserved, 0, sizeof(pvd.reserved));
	
	dest.pad(SECTOR_SIZE, 0);
	static const uint8_t volume_desc_set_terminator[] = {0xff, 'C', 'D', '0', '0', '1', 0x01};
	dest.write_n((char*) volume_desc_set_terminator, sizeof(volume_desc_set_terminator));
	
	// Write out path tables.
	iso9660_path_table_entry path_table;
	path_table.identifier_length = 1;
	path_table.record_length = 0;
	path_table.parent = 1;
	path_table.identifier = 0;
	path_table.pad = 0;
	
	// It seems like the path table is always expected to be at this LBA even if
	// we write a different one into the PVD. Maybe it's hardcoded?
	dest.pad(SECTOR_SIZE, 0);
	while(dest.tell() < 0x101 * SECTOR_SIZE) {
		dest.write_n((char*) zeroed_sector, sizeof(zeroed_sector));
	}
	size_t path_table_lba = dest.tell() / SECTOR_SIZE;
	path_table.lba = path_table_lba + 4;
	dest.write(path_table);
	dest.pad(SECTOR_SIZE, 0);
	dest.write(path_table);
	
	path_table.lba = byte_swap_32(path_table.lba);
	dest.pad(SECTOR_SIZE, 0);
	dest.write(path_table);
	dest.pad(SECTOR_SIZE, 0);
	dest.write(path_table);
	
	pvd.l_path_table = path_table_lba;
	pvd.optional_l_path_table = path_table_lba + 1;
	pvd.m_path_table = byte_swap_32(path_table_lba + 2);
	pvd.optional_m_path_table = byte_swap_32(path_table_lba + 3);
	
	// Determine root directory LBA and size.
	dest.pad(SECTOR_SIZE, 0);
	uint32_t root_dir_lba = (uint32_t) dest.tell() / SECTOR_SIZE;
	assert(root_dir_lba == path_table_lba + 4);
	uint32_t root_dir_size = (sizeof(iso9660_directory_record) + 1 + 14) * 2;
	for(const iso_file_record& file : files) {
		root_dir_size +=
			sizeof(iso9660_directory_record) +
			file.name.size() +
			(file.name.size() % 2 == 0) + 14;
	}
	pvd.root_directory.lba = iso9660_i32_lsb_msb::from_scalar(root_dir_lba);
	pvd.root_directory.data_length = iso9660_i32_lsb_msb::from_scalar(root_dir_size);
	
	// Write out root directory records.
	auto write_directory_record = [&](const iso_file_record& file, uint8_t flags) {
		iso9660_directory_record record;
		record.record_length =
			sizeof(iso9660_directory_record) +
			file.name.size() +
			(file.name.size() % 2 == 0) + 14;
		record.extended_attribute_record_length = 0;
		record.lba = iso9660_i32_lsb_msb::from_scalar(file.lba.sectors);
		record.data_length = iso9660_i32_lsb_msb::from_scalar(file.size);
		memset(record.recording_date_time, 0, sizeof(record.recording_date_time));
		record.file_flags = flags;
		record.file_unit_size = 0;
		record.interleave_gap_size = 0;
		record.volume_sequence_number = iso9660_i16_lsb_msb::from_scalar(1);
		record.identifier_length = file.name.size() + (file.name.size() == 0);
		dest.write(record);
		dest.write_n(file.name.data(), file.name.size());
		if(file.name.size() % 2 == 0) {
			dest.write<uint8_t>(0);
		}
		
		// Not sure why we need this padding.
		for(int i = 0; i < 14; i++) {
			dest.write<uint8_t>(0);
		}
	};
	iso_file_record dot = {"", root_dir_lba, root_dir_size};
	write_directory_record(dot, 2);
	iso_file_record dot_dot = {"\x01", root_dir_lba, root_dir_size};
	write_directory_record(dot_dot, 2);
	for(const iso_file_record& file : files) {
		write_directory_record(file, 0);
	}
	
	return {};
}
