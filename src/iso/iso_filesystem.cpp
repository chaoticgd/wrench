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

#include <core/filesystem.h>

packed_struct(IsoPathTableEntry,
	u8 identifier_length;
	u8 record_length;
	u32 lba;
	u16 parent;
)

void read_directory_record(IsoDirectory& dest, Buffer src, s64 ofs, size_t size, size_t depth);

IsoFilesystem read_iso_filesystem(InputStream& src)
{
	src.seek(0);
	std::vector<u8> filesystem_buf = src.read_multiple<u8>(MAX_FILESYSTEM_SIZE_BYTES);
	IsoFilesystem filesystem;
	if (!read_iso_filesystem(filesystem, Buffer(filesystem_buf))) {
		fprintf(stderr, "error: Missing or invalid ISO filesystem!\n");
		exit(1);
	}
	return filesystem;
}

bool read_iso_filesystem(IsoFilesystem& dest, Buffer src)
{
	IsoFilesystem filesystem;
	
	filesystem.pvd = src.read<IsoPrimaryVolumeDescriptor>(0x10 * SECTOR_SIZE, "primary volume descriptor");
	if (filesystem.pvd.volume_descriptor_type != 0x01) {
		return false;
	}
	if (memcmp(filesystem.pvd.standard_identifier, "CD001", 5) != 0) {
		return false;
	}
	if (filesystem.pvd.root_directory.data_length.lsb > 0x10000) {
		return false;
	}
	
	size_t root_dir_ofs = filesystem.pvd.root_directory.lba.lsb * SECTOR_SIZE;
	size_t root_dir_size = filesystem.pvd.root_directory.data_length.lsb;
	read_directory_record(filesystem.root, src, root_dir_ofs, root_dir_size, 0);
	
	dest = std::move(filesystem);
	
	return true;
}

void read_directory_record(IsoDirectory& dest, Buffer src, s64 ofs, size_t size, size_t depth)
{
	verify(depth <= 8, "Depth limit (8 levels) reached!");
	
	s64 end = ofs + size;
	
	size_t i;
	for (i = 0; i < 1000 && ofs < end; i++) {
		s64 record_ofs = ofs;
		auto& record = src.read<IsoDirectoryRecord>(ofs, "directory record");
		ofs += sizeof(IsoDirectoryRecord);
		if (record.record_length < 1) {
			ofs = record_ofs + 1;
			continue;
		}
		if (record.file_flags & 2) {
			if (i < 2) {
				// Skip dot and dot dot.
				ofs = record_ofs + record.record_length;
				continue;
			}
			IsoDirectory subdir;
			subdir.name = src.read_fixed_string(ofs, record.identifier_length);
			ofs += record.identifier_length;
			for (char& c : subdir.name) {
				c = tolower(c);
			}
			read_directory_record(subdir, src, record.lba.lsb * SECTOR_SIZE, record.data_length.lsb, depth + 1);
			dest.subdirs.push_back(subdir);
		} else if (record.identifier_length >= 2) {
			IsoFileRecord file;
			file.name = src.read_fixed_string(ofs, record.identifier_length);
			ofs += record.identifier_length;
			for (char& c : file.name) {
				c = tolower(c);
			}
			if (file.name.size() >= 2 && file.name[file.name.size() - 2] == ';' && file.name[file.name.size() - 1] == '1') {
				file.name = file.name.substr(0, file.name.size() - 2);
			}
			file.lba = {(s32) record.lba.lsb};
			file.size = record.data_length.lsb;
			dest.files.push_back(file);
		}
		ofs = record_ofs + record.record_length;
	}
	verify(i != 1000, "Iteration limit exceeded while reading directory!");
}

static void copy_and_pad(char* dest, const char* src, size_t size)
{
	size_t i;
	for (i = 0; i < strlen(src); i++) {
		dest[i] = src[i];
	}
	for (; i < size; i++) {
		dest[i] = ' ';
	}
}

static void flatten_subdirs(std::vector<IsoDirectory*>* flat_dirs, IsoDirectory* dir)
{
	for (IsoDirectory& subdir : dir->subdirs) {
		subdir.parent = dir;
		subdir.index = flat_dirs->size();
		subdir.parent_index = dir->index;
		flat_dirs->push_back(&subdir);
		flatten_subdirs(flat_dirs, &subdir);
	}
}

static void write_directory_records(OutputStream& dest, const IsoDirectory& dir);
static void write_directory_record(OutputStream& dest, const IsoFileRecord& file, u8 flags);

void write_iso_filesystem(OutputStream& dest, IsoDirectory* root_dir)
{
	dest.seek(16 * SECTOR_SIZE);
	
	IsoPvdDateTime zeroed_datetime = {{0}};
	
	// Write out primary volume descriptor.
	size_t pvd_pos = dest.tell();
	IsoPrimaryVolumeDescriptor pvd;
	defer([&]() {
		size_t pos = dest.tell();
		dest.seek(pvd_pos);
		dest.write(pvd);
		dest.seek(pos);
	});
	dest.seek(dest.tell() + sizeof(pvd));
	pvd.volume_descriptor_type = 0x01;
	memcpy(pvd.standard_identifier, "CD001", sizeof(pvd.standard_identifier));
	pvd.volume_descriptor_version = 1;
	pvd.unused_7 = 0;
	copy_and_pad(pvd.system_identifier, "WRENCH", sizeof(pvd.system_identifier));
	copy_and_pad(pvd.volume_identifier, "WRENCH", sizeof(pvd.volume_identifier));
	memset(pvd.unused_48, 0, 8);
	pvd.volume_space_size = IsoLsbMsb32::from_scalar(0);
	memset(pvd.unused_58, 0, 32);
	pvd.volume_set_size = IsoLsbMsb16::from_scalar(1);
	pvd.volume_sequence_number = IsoLsbMsb16::from_scalar(1);
	pvd.logical_block_size = IsoLsbMsb16::from_scalar(SECTOR_SIZE);
	pvd.path_table_size = {0, 0};
	pvd.l_path_table = 0;
	pvd.optional_l_path_table = 0;
	pvd.m_path_table = 0;
	pvd.optional_m_path_table = 0;
	pvd.root_directory.record_length = 0x22;
	pvd.root_directory.extended_attribute_record_length = 0;
	memset(&pvd.root_directory.recording_date_time, 0, sizeof(pvd.root_directory.recording_date_time));
	pvd.root_directory.file_flags = 2;
	pvd.root_directory.file_unit_size = 0;
	pvd.root_directory.interleave_gap_size = 0;
	pvd.root_directory.volume_sequence_number = IsoLsbMsb16::from_scalar(1);
	pvd.root_directory.identifier_length = 1;
	pvd.root_directory_pad = 0;
	copy_and_pad(pvd.volume_set_identifier, "", sizeof(pvd.volume_set_identifier));
	copy_and_pad(pvd.publisher_identifier, "", sizeof(pvd.publisher_identifier));
	copy_and_pad(pvd.data_preparer_identifier, "", sizeof(pvd.data_preparer_identifier));
	copy_and_pad(pvd.application_identifier, "", sizeof(pvd.application_identifier));
	copy_and_pad(pvd.copyright_file_identifier, "", sizeof(pvd.copyright_file_identifier));
	copy_and_pad(pvd.abstract_file_identifier, "", sizeof(pvd.abstract_file_identifier));
	copy_and_pad(pvd.bibliographic_file_identifier, "", sizeof(pvd.bibliographic_file_identifier));
	pvd.volume_creation_date_time = zeroed_datetime;
	pvd.volume_modification_date_time = zeroed_datetime;
	pvd.volume_expiration_date_time = zeroed_datetime;
	pvd.volume_effective_date_time = zeroed_datetime;
	pvd.file_structure_version = 1;
	pvd.unused_372 = 0;
	memset(pvd.application_use, 0, sizeof(pvd.application_use));
	memset(pvd.reserved, 0, sizeof(pvd.reserved));
	
	dest.pad(SECTOR_SIZE, 0);
	static const u8 volume_desc_set_terminator[] = {0xff, 'C', 'D', '0', '0', '1', 0x01};
	dest.write_n(volume_desc_set_terminator, sizeof(volume_desc_set_terminator));
	
	// It seems like the path table is always expected to be at this LBA even if
	// we write a different one into the PVD. Maybe it's hardcoded?
	dest.pad(SECTOR_SIZE, 0);
	static const u8 zeroed_sector[SECTOR_SIZE] = {0};
	while (dest.tell() < 0x101 * SECTOR_SIZE) {
		dest.write_n(zeroed_sector, sizeof(zeroed_sector));
	}
	
	// Get a linear list of all the directories. This also sets the parent
	// pointers and indices.
	std::vector<IsoDirectory*> flat_dirs;
	flatten_subdirs(&flat_dirs, root_dir);
	
	// Fixup the file names.
	for (char& c : root_dir->name) {
		c = toupper(c);
	}
	for (IsoFileRecord& file : root_dir->files) {
		for (char& c : file.name) {
			c = toupper(c);
		}
		file.name += ";1";
	}
	for (IsoDirectory* dir : flat_dirs) {
		for (char& c : dir->name) {
			c = toupper(c);
		}
		for (IsoFileRecord& file : dir->files) {
			for (char& c : file.name) {
				c = toupper(c);
			}
			file.name += ";1";
		}
	}
	
	// Determine the LBAs of the path table and the root directory.
	dest.pad(SECTOR_SIZE, 0);
	pvd.l_path_table = dest.tell() / SECTOR_SIZE;
	pvd.optional_l_path_table = 0;
	pvd.m_path_table = byte_swap_32(pvd.l_path_table + 1);
	pvd.optional_m_path_table = 0;
	pvd.root_directory.lba = IsoLsbMsb32::from_scalar(pvd.l_path_table + 2);
	
	// Determine directory record LBAs and sizes.
	size_t next_dir_lba = pvd.root_directory.lba.lsb;
	root_dir->lba = {(s32) pvd.root_directory.lba.lsb};
	std::vector<u8> root_dummy_vec;
	MemoryOutputStream root_dummy(root_dummy_vec);
	write_directory_records(root_dummy, *root_dir);
	root_dir->size = root_dummy.size();
	pvd.root_directory.data_length = IsoLsbMsb32::from_scalar(root_dir->size);
	next_dir_lba += Sector32::size_from_bytes(root_dir->size).sectors;
	for (size_t i = 0; i < flat_dirs.size(); i++) {
		IsoDirectory* dir = flat_dirs[i];
		dir->lba = {(s32) next_dir_lba};
		std::vector<u8> dummy_vec;
		MemoryOutputStream dummy(dummy_vec);
		write_directory_records(dummy, *dir);
		dir->size = dummy.size();
		next_dir_lba += Sector32::size_from_bytes(dir->size).sectors;
	}
	
	// Write out little endian path table.
	size_t start_of_path_table = dest.tell();
	IsoPathTableEntry root_pte_lsb;
	root_pte_lsb.identifier_length = 1;
	root_pte_lsb.record_length = 0;
	root_pte_lsb.lba = pvd.root_directory.lba.lsb;
	root_pte_lsb.parent = 1;
	dest.write(root_pte_lsb);
	dest.write<u8>(0); // identifier
	dest.write<u8>(0); // pad
	for (IsoDirectory* dir : flat_dirs) {
		IsoPathTableEntry root_pte;
		root_pte.identifier_length = dir->name.size();
		root_pte.record_length = 0;
		root_pte.lba = dir->lba.sectors;
		root_pte.parent = dir->parent_index;
		dest.write(root_pte);
		dest.write_n((u8*) dir->name.data(), dir->name.size());
		if (root_pte.identifier_length % 2 == 1) {
			dest.write<u8>(0); // pad
		}
	}
	size_t end_of_path_table = dest.tell();
	
	pvd.path_table_size = IsoLsbMsb32::from_scalar(end_of_path_table - start_of_path_table);
	
	// Write out big endian path table.
	dest.pad(SECTOR_SIZE, 0);
	IsoPathTableEntry root_pte_msb;
	root_pte_msb.identifier_length = 1;
	root_pte_msb.record_length = 0;
	root_pte_msb.lba = pvd.root_directory.lba.msb;
	root_pte_msb.parent = 1;
	dest.write(root_pte_msb);
	dest.write<u8>(0); // identifier
	dest.write<u8>(0); // pad
	for (IsoDirectory* dir : flat_dirs) {
		IsoPathTableEntry root_pte;
		root_pte.identifier_length = dir->name.size();
		root_pte.record_length = 0;
		root_pte.lba = byte_swap_32(dir->lba.sectors);
		root_pte.parent = dir->parent_index;
		dest.write(root_pte);
		dest.write_n((u8*) dir->name.data(), dir->name.size());
		if (root_pte.identifier_length % 2 == 1) {
			dest.write<u8>(0); // pad
		}
	}
	
	// Write out all the directories.
	dest.pad(SECTOR_SIZE, 0);
	verify_fatal(dest.tell() == pvd.root_directory.lba.lsb * SECTOR_SIZE);
	write_directory_records(dest, *root_dir);
	for (IsoDirectory* dir : flat_dirs) {
		dest.pad(SECTOR_SIZE, 0);
		write_directory_records(dest, *dir);
	}
}

static void write_directory_records(OutputStream& dest, const IsoDirectory& dir)
{
	// Either this is being written out to a dummy stream to calculate the space
	// required for the directory record, or this should be being written out at
	// the correct LBA.
	verify_fatal(dest.tell() == 0 || dest.tell() == dir.lba.bytes());
	IsoFileRecord dot = {"", dir.lba, dir.size};
	write_directory_record(dest, dot, 2);
	IsoFileRecord dot_dot = {"\x01", dir.lba, dir.size};
	if (dir.parent != nullptr) {
		dot_dot.lba = dir.parent->lba;
		dot_dot.size = dir.parent->size;
	}
	write_directory_record(dest, dot_dot, 2);
	for (const IsoFileRecord& file : dir.files) {
		write_directory_record(dest, file, 0);
	}
	for (const IsoDirectory& dir : dir.subdirs) {
		IsoFileRecord record = {dir.name, dir.lba, dir.size};
		record.modified_time = fs::file_time_type::clock::now();
		write_directory_record(dest, record, 2);
	}
}

static void write_directory_record(OutputStream& dest, const IsoFileRecord& file, u8 flags)
{
	IsoDirectoryRecord record = {0};
	record.record_length =
		sizeof(IsoDirectoryRecord) +
		file.name.size() +
		(file.name.size() % 2 == 0);
	record.extended_attribute_record_length = 0;
	record.lba = IsoLsbMsb32::from_scalar(file.lba.sectors);
	record.data_length = IsoLsbMsb32::from_scalar(file.size);
	
	// Curse this library.
	auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>
		(file.modified_time - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
	time_t cftime = std::chrono::system_clock::to_time_t(sctp);
	tm* t = std::localtime(&cftime);
	if (t) {
		IsoDirectoryDateTime& dt = record.recording_date_time;
		dt.years_since_1900 = t->tm_year;
		dt.month = t->tm_mon + 1;
		dt.day = t->tm_mday;
		dt.hour = t->tm_hour - 1;
		dt.minute = t->tm_min;
		dt.second = t->tm_sec;
		dt.time_zone = 0;
	}
	
	record.file_flags = flags;
	record.file_unit_size = 0;
	record.interleave_gap_size = 0;
	record.volume_sequence_number = IsoLsbMsb16::from_scalar(1);
	record.identifier_length = file.name.size() + (file.name.size() == 0);
	if ((dest.tell() % SECTOR_SIZE) + record.record_length > SECTOR_SIZE) {
		// Directory records cannot cross sector boundaries.
		dest.pad(SECTOR_SIZE, 0);
	}
	dest.write(record);
	dest.write_n((u8*) file.name.data(), file.name.size());
	if (file.name.size() % 2 == 0) {
		dest.write<u8>(0);
	}
}

void print_file_record(const IsoFileRecord& record)
{
	printf("%-16ld%-16ld%s\n", (size_t) record.lba.sectors, (size_t) record.size, record.name.c_str());
}
