/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2024 chaoticgd

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

#include "elf.h"

enum class ElfIdentClass : u8
{
	B32 = 0x1,
	B64 = 0x2
};

enum class ElfFileType : u16
{
	NONE   = 0x00,
	REL    = 0x01,
	EXEC   = 0x02,
	DYN    = 0x03,
	CORE   = 0x04,
	LOOS   = 0xfe00,
	HIOS   = 0xfeff,
	LOPROC = 0xff00,
	HIPROC = 0xffff
};

enum class ElfMachine : u16
{
	MIPS  = 0x08
};

packed_struct(ElfFileHeader,
	/* 0x00 */ u8 magic[4]; // 7f 45 4c 46
	/* 0x04 */ ElfIdentClass e_class;
	/* 0x05 */ u8 endianess;
	/* 0x06 */ u8 format_version;
	/* 0x07 */ u8 os_abi;
	/* 0x08 */ u8 abi_version;
	/* 0x09 */ u8 pad[7];
	/* 0x10 */ ElfFileType type;
	/* 0x12 */ ElfMachine machine;
	/* 0x14 */ s32 version;
	/* 0x18 */ s32 entry;
	/* 0x1c */ s32 phoff;
	/* 0x20 */ s32 shoff;
	/* 0x24 */ u32 flags;
	/* 0x28 */ u16 ehsize;
	/* 0x2a */ u16 phentsize;
	/* 0x2c */ u16 phnum;
	/* 0x2e */ u16 shentsize;
	/* 0x30 */ u16 shnum;
	/* 0x32 */ u16 shstrndx;
)

ElfFile read_elf_file(Buffer src)
{
	const ElfFileHeader& file_header = src.read<ElfFileHeader>(0, "ELF file header");
	verify(memcmp(file_header.magic, "\x7f\x45\x4c\x46", 4) == 0, "Magic bytes don't match.");
	
	ElfFile elf;
	
	auto section_headers = src.read_multiple<ElfSectionHeader>(file_header.shoff, file_header.shnum, "ELF section headers");
	auto program_headers = src.read_multiple<ElfProgramHeader>(file_header.phoff, file_header.phnum, "ELF program headers");
	
	const ElfSectionHeader& name_section = section_headers[file_header.shstrndx];
	for (size_t i = 0; i < section_headers.size(); i++) {
		const ElfSectionHeader& shdr = section_headers[i];
		if (i != file_header.shstrndx) {
			ElfSection& section = elf.sections.emplace_back();
			section.name = src.read_string(name_section.offset + shdr.name, false);
			for (s32 j = 0; j < (s32) program_headers.size(); j++) {
				const ElfProgramHeader& phdr = program_headers[j];
				if (shdr.offset >= program_headers[j].offset && shdr.offset + shdr.size <= phdr.offset + phdr.filesz) {
					section.segment = j;
					break;
				}
			}
			section.header = shdr;
			section.data = src.read_bytes(shdr.offset, shdr.size, "ELF section data");
		}
	}
	
	elf.segments = program_headers.copy();
	elf.entry_point = file_header.entry;
	
	return elf;
}

void write_elf_file(OutBuffer dest, const ElfFile& elf)
{
	std::vector<ElfSectionHeader> section_headers;
	std::vector<ElfProgramHeader> program_headers = elf.segments;
	
	// Initialise the section headers.
	section_headers.reserve(elf.sections.size());
	for (const ElfSection& section : elf.sections) {
		section_headers.emplace_back(section.header);
	}
	
	// Determine the layout of the file and write out the section data.
	s64 file_header_ofs = dest.alloc<ElfFileHeader>();
	s64 program_headers_ofs = dest.alloc_multiple<ElfProgramHeader>(elf.segments.size());
	dest.pad(0x1000);
	for (size_t i = 0; i < elf.sections.size(); i++) {
		const ElfSection& section = elf.sections[i];
		if (section.data.empty())
			continue;
		
		// Insert padding.
		if (section.segment > -1) {
			if (i > 0) {
				const ElfSection& last_section = elf.sections[i - 1];
				if (last_section.segment == section.segment) {
					// Make sure the address field matches up with the offset
					// between different sections so they form a valid segment.
					s32 padding_size = section.header.addr - (last_section.header.addr + last_section.data.size());
					if (padding_size > 0) {
						for (s32 j = 0; j < padding_size; j++) {
							dest.write<u8>(0);
						}
					} else if (padding_size < 0) {
						fprintf(stderr, "warning: Padding calculation gave negative result, segments may be incorrect.\n");
					}
				} else {
					dest.pad(elf.segments.at(section.segment).align);
				}
			} else {
				dest.pad(0x80);
			}
		}
		
		// Write the data and fill in the section headers.
		s64 offset = dest.write_multiple(section.data);
		section_headers[i].offset = (s32) offset;
		section_headers[i].size = (s32) section.data.size();
	}
	
	s64 section_header_names_ofs = dest.tell();
	for (size_t i = 0; i < elf.sections.size(); i++) {
		s64 offset = dest.tell();
		dest.writesf("%s", elf.sections[i].name.c_str());
		dest.write<char>('\0');
		section_headers[i].name = (s32) (offset - section_header_names_ofs);
	}
	s64 shstrtab_string_ofs = dest.tell();
	dest.writesf(".shstrtab");
	dest.write<char>('\0');
	s64 section_header_names_end = dest.tell();
	dest.pad(4);
	s64 section_headers_ofs = dest.alloc_multiple<ElfSectionHeader>(elf.sections.size() + 1);
	
	// Fill in the program headers.
	for (size_t segment = 0; segment < elf.segments.size(); segment++) {
		s32 addr = INT32_MAX;
		s32 offset = INT32_MAX;
		s32 end = INT32_MIN;
		for (size_t section = 0; section < elf.sections.size(); section++) {
			if (elf.sections[section].segment == segment && section_headers[section].addr > 0) {
				addr = std::min(addr, section_headers[section].addr);
				offset = std::min(offset, section_headers[section].offset);
				end = std::max(end, section_headers[section].offset + (s32) elf.sections[section].data.size());
			}
		}
		
		if (addr == INT32_MAX) addr = 0;
		if (offset == INT32_MAX) offset = 0;
		if (end == INT32_MIN) end = 0;
		
		program_headers[segment].offset = offset;
		program_headers[segment].vaddr = addr;
		program_headers[segment].paddr = addr;
		program_headers[segment].filesz = end - offset;
		program_headers[segment].memsz = end - offset;
	}
	
	// Write out the file header.
	ElfFileHeader file_header = {};
	memcpy(file_header.magic, "\x7f\x45\x4c\x46", 4);
	file_header.e_class =  ElfIdentClass::B32;
	file_header.endianess = 1;
	file_header.format_version = 1;
	file_header.os_abi = 0;
	file_header.abi_version = 0;
	file_header.type = ElfFileType::EXEC;
	file_header.machine = ElfMachine::MIPS;
	file_header.version = 1;
	file_header.entry = elf.entry_point;
	if (!elf.segments.empty()) {
		file_header.phoff = (s32) program_headers_ofs;
	}
	file_header.shoff = (s32) section_headers_ofs;
	file_header.flags = 0x20924001;
	file_header.ehsize = (u16) sizeof(ElfFileHeader);
	file_header.phentsize = (u16) sizeof(ElfProgramHeader);
	file_header.phnum = (u16) elf.segments.size();
	file_header.shentsize = (u16) sizeof(ElfSectionHeader);
	file_header.shnum = (u16) elf.sections.size() + 1;
	file_header.shstrndx = (u16) elf.sections.size();
	dest.write(file_header_ofs, file_header);
	
	// Write out the section headers and program headers.
	dest.write_multiple(section_headers_ofs, section_headers);
	dest.write_multiple(program_headers_ofs, program_headers);
	
	// Write out the section header for the section name string table.
	ElfSectionHeader shstrtab = {};
	shstrtab.name = (s32) (shstrtab_string_ofs - section_header_names_ofs);
	shstrtab.type = SHT_STRTAB;
	shstrtab.flags = 0;
	shstrtab.addr = 0;
	shstrtab.offset = (s32) section_header_names_ofs;
	shstrtab.size = (s32) (section_header_names_end - section_header_names_ofs);
	shstrtab.link = 0;
	shstrtab.info = 0;
	shstrtab.addralign = 1;
	shstrtab.entsize = 0;
	dest.write(section_headers_ofs + elf.sections.size() * sizeof(ElfSectionHeader), shstrtab);
}

packed_struct(RatchetSectionHeader,
	s32 dest_address;
	s32 copy_size;
	ElfSectionType section_type;
	s32 entry_point;
)

ElfFile read_ratchet_executable(Buffer src)
{
	ElfFile elf;
	
	// Add the null section to the beginning. This is a convention for ELF files
	// so that a section index of zero can be reserved to mean null.
	elf.sections.emplace_back();
	
	s32 ofs = 0;
	for (s32 i = 0; ofs < src.size(); i++) {
		// Read the block header, set the entry point, and check for EOF.
		RatchetSectionHeader header = src.read<RatchetSectionHeader>(ofs, "ratchet section header");
		ofs += sizeof(RatchetSectionHeader);
		if (elf.entry_point == 0) {
			elf.entry_point = header.entry_point;
		} else if (header.entry_point != elf.entry_point) {
			// This is the logic the game uses for breaking out of the loop, but
			// it actually reads out of bounds at the end.
			break;
		}
		
		// Reconstruct the section header and copy the data.
		ElfSection& section = elf.sections.emplace_back();
		section.name = stringf(".unknown_%d", i);
		section.data = src.read_bytes(ofs, header.copy_size, "ratchet section data");
		section.header.type = header.section_type;
		section.header.flags = 0;
		section.header.addr = header.dest_address;
		section.header.offset = 0;
		section.header.size = header.copy_size;
		section.header.link = 0;
		section.header.info = 0;
		section.header.addralign = 1;
		section.header.entsize = 0;
		
		ofs += header.copy_size;
	}
	
	return elf;
}

void write_ratchet_executable(OutBuffer dest, const ElfFile& elf)
{
	for (const ElfSection& section : elf.sections) {
		if (section.header.addr > 0 && !section.data.empty()) {
			verify(section.header.addr % 4 == 0, "Loadable ELF section data must be aligned to 4 byte boundary in memory.");
			verify(section.data.size() % 4 == 0, "Loadable ELF section size in bytes must be a multiple of 4.");
			
			RatchetSectionHeader header;
			header.dest_address = section.header.addr;
			header.copy_size = section.data.size();
			header.section_type = section.header.type;
			header.entry_point = elf.entry_point;
			dest.write(header);
			dest.write_multiple(section.data);
		}
	}
}

bool fill_in_elf_headers(ElfFile& elf, const ElfFile& donor)
{
	if (elf.sections.size() != donor.sections.size())
		return false;
	
	for (s32 i = 0; i < donor.sections.size(); i++)
		if (elf.sections[i].header.type != donor.sections[i].header.type)
			return false;
	
	for (s32 i = 0; i < donor.sections.size(); i++) {
		ElfSection& section = elf.sections[i];
		const ElfSection& donor_section = donor.sections[i];
		section.name = donor_section.name;
		section.segment = donor_section.segment;
		section.header.flags = donor_section.header.flags;
		section.header.addralign = donor_section.header.addralign;
		section.header.entsize = donor_section.header.entsize;
	}
	
	elf.segments = donor.segments;
	
	return true;
}

static const u32 AX = SHF_ALLOC | SHF_EXECINSTR;
static const u32 WA = SHF_WRITE | SHF_ALLOC;
static const u32 A = SHF_ALLOC;
static const u32 WAP = SHF_WRITE | SHF_ALLOC | SHF_MASKPROC;
static const u32 WAX = SHF_WRITE | SHF_ALLOC | SHF_EXECINSTR;

const ElfFile DONOR_UYA_BOOT_ELF_HEADERS = {
	{
		// Name         Seg  N  Type              Flag Ad Of Sz Lk In Algn ES
		{""           , -1, {0, SHT_NULL        ,   0, 0, 0, 0, 0, 0,  0,  0}},
		{".reginfo"   , -1, {0, SHT_MIPS_REGINFO,   0, 0, 0, 0, 0, 0,  4,  1}},
		{".vutext"    ,  0, {0, SHT_PROGBITS    ,  AX, 0, 0, 0, 0, 0,  16, 0}},
		{"core.text"  ,  0, {0, SHT_PROGBITS    ,  AX, 0, 0, 0, 0, 0,  64, 0}},
		{"core.data"  ,  0, {0, SHT_PROGBITS    ,  WA, 0, 0, 0, 0, 0, 128, 0}},
		{"core.rdata" ,  0, {0, SHT_PROGBITS    ,   A, 0, 0, 0, 0, 0,  16, 0}},
		{"core.bss"   ,  0, {0, SHT_NOBITS      , WAP, 0, 0, 0, 0, 0,  64, 0}},
		{"core.lit"   ,  0, {0, SHT_PROGBITS    , WAP, 0, 0, 0, 0, 0,   8, 0}},
		{".lit"       ,  0, {0, SHT_PROGBITS    , WAP, 0, 0, 0, 0, 0,  64, 0}},
		{".bss"       ,  0, {0, SHT_NOBITS      , WAP, 0, 0, 0, 0, 0,  64, 0}},
		{".data"      ,  0, {0, SHT_PROGBITS    ,  WA, 0, 0, 0, 0, 0,  64, 0}},
		{"lvl.vtbl"   ,  0, {0, SHT_PROGBITS    ,   A, 0, 0, 0, 0, 0,   1, 0}},
		{"lvl.camvtbl",  0, {0, SHT_PROGBITS    ,   A, 0, 0, 0, 0, 0,   1, 0}},
		{"lvl.sndvtbl",  0, {0, SHT_PROGBITS    ,   A, 0, 0, 0, 0, 0,   1, 0}},
		{".text"      ,  0, {0, SHT_PROGBITS    ,  AX, 0, 0, 0, 0, 0,  64, 0}},
		{"patch.data" ,  0, {0, SHT_MIPS_REGINFO,  AX, 0, 0, 0, 0, 0,   4, 1}},
		{"legal.data" ,  1, {0, SHT_PROGBITS    ,  WA, 0, 0, 0, 0, 0,   4, 0}},
		{"mc1.data"   ,  1, {0, SHT_PROGBITS    ,  WA, 0, 0, 0, 0, 0,  64, 0}},
		{"mc1.data"   ,  1, {0, SHT_PROGBITS    ,  WA, 0, 0, 0, 0, 0,  64, 0}},
	},
	{
		// Type   Of VA PA FS MS Flags               Align
		{PT_LOAD, 0, 0, 0, 0, 0, PF_R | PF_W | PF_X, 0x1000},
		{PT_LOAD, 0, 0, 0, 0, 0, PF_R | PF_W | PF_X, 0x1000}
	}
};

extern const ElfFile DONOR_DL_BOOT_ELF_HEADERS = {
	{
		// Name         Seg  N  Type              Flag Ad Of Sz Lk In Algn ES
		{""           , -1, {0, SHT_NULL        ,   0, 0, 0, 0, 0, 0,  0,  0}},
		{".reginfo"   , -1, {0, SHT_MIPS_REGINFO,   0, 0, 0, 0, 0, 0,   4, 1}},
		{".vutext"    ,  0, {0, SHT_PROGBITS    ,  AX, 0, 0, 0, 0, 0,  16, 0}},
		{"core.text"  ,  0, {0, SHT_PROGBITS    ,  AX, 0, 0, 0, 0, 0,  64, 0}},
		{"core.data"  ,  0, {0, SHT_PROGBITS    ,  WA, 0, 0, 0, 0, 0, 128, 0}},
		{"core.rdata" ,  0, {0, SHT_PROGBITS    ,   A, 0, 0, 0, 0, 0,  16, 0}},
		{"core.bss"   ,  0, {0, SHT_NOBITS      , WAP, 0, 0, 0, 0, 0,  64, 0}},
		{"core.lit"   ,  0, {0, SHT_PROGBITS    , WAP, 0, 0, 0, 0, 0,   8, 0}},
		{".lit"       ,  0, {0, SHT_PROGBITS    , WAP, 0, 0, 0, 0, 0,  64, 0}},
		{".bss"       ,  0, {0, SHT_NOBITS      , WAP, 0, 0, 0, 0, 0,  64, 0}},
		{".data"      ,  0, {0, SHT_PROGBITS    ,  WA, 0, 0, 0, 0, 0,  64, 0}},
		{"lvl.vtbl"   ,  0, {0, SHT_PROGBITS    ,   A, 0, 0, 0, 0, 0,   1, 0}},
		{"lvl.camvtbl",  0, {0, SHT_PROGBITS    ,   A, 0, 0, 0, 0, 0,   1, 0}},
		{"lvl.sndvtbl",  0, {0, SHT_PROGBITS    ,   A, 0, 0, 0, 0, 0,   1, 0}},
		{".text"      ,  0, {0, SHT_PROGBITS    ,  AX, 0, 0, 0, 0, 0,  64, 0}},
		{"patch.data" ,  0, {0, SHT_MIPS_REGINFO,  AX, 0, 0, 0, 0, 0,   4, 1}},
		{"net.text"   ,  1, {0, SHT_PROGBITS    , WAX, 0, 0, 0, 0, 0,  16, 0}},
		{"net.nostomp",  1, {0, SHT_PROGBITS    , WAX, 0, 0, 0, 0, 0,   8, 0}}
	},
	{
		// Type   Of VA PA FS MS Flags               Align
		{PT_LOAD, 0, 0, 0, 0, 0, PF_R | PF_W | PF_X, 0x1000},
		{PT_LOAD, 0, 0, 0, 0, 0, PF_R | PF_W | PF_X, 0x1000}
	}
};

extern const ElfFile DONOR_RAC_GC_UYA_LEVEL_ELF_HEADERS = {
	{
		// Name         Seg  N  Type              Flag Ad Of Sz Lk In Algn ES
		{""           , -1, {0, SHT_NULL        ,   0, 0, 0, 0, 0, 0,  0,  0}},
		{".lit"       ,  0, {0, SHT_PROGBITS    , WAP, 0, 0, 0, 0, 0,  64, 0}},
		{".bss"       ,  0, {0, SHT_NOBITS      , WAP, 0, 0, 0, 0, 0,  64, 0}},
		{".data"      ,  0, {0, SHT_PROGBITS    ,  WA, 0, 0, 0, 0, 0,  64, 0}},
		{"lvl.vtbl"   ,  0, {0, SHT_PROGBITS    ,   A, 0, 0, 0, 0, 0,   1, 0}},
		{"lvl.camvtbl",  0, {0, SHT_PROGBITS    ,   A, 0, 0, 0, 0, 0,   1, 0}},
		{"lvl.sndvtbl",  0, {0, SHT_PROGBITS    ,   A, 0, 0, 0, 0, 0,   1, 0}},
		{".text"      ,  0, {0, SHT_PROGBITS    ,  AX, 0, 0, 0, 0, 0,  64, 0}}
	},
	{
		// Type   Of VA PA FS MS Flags               Align
		{PT_LOAD, 0, 0, 0, 0, 0, PF_R | PF_W | PF_X, 0x1000}
	}
};

extern const ElfFile DONOR_DL_LEVEL_ELF_NOBITS_HEADERS = {
	{
		// Name         Seg  N  Type              Flag Ad Of Sz Lk In Algn ES
		{""           , -1, {0, SHT_NULL        ,   0, 0, 0, 0, 0, 0,  0,  0}},
		{".lit"       ,  0, {0, SHT_PROGBITS    , WAP, 0, 0, 0, 0, 0,  64, 0}},
		{".bss"       ,  0, {0, SHT_NOBITS      , WAP, 0, 0, 0, 0, 0,  64, 0}},
		{".data"      ,  0, {0, SHT_PROGBITS    ,  WA, 0, 0, 0, 0, 0,  64, 0}},
		{"lvl.vtbl"   ,  0, {0, SHT_PROGBITS    ,   A, 0, 0, 0, 0, 0,   1, 0}},
		{"lvl.camvtbl",  0, {0, SHT_PROGBITS    ,   A, 0, 0, 0, 0, 0,   1, 0}},
		{"lvl.sndvtbl",  0, {0, SHT_PROGBITS    ,   A, 0, 0, 0, 0, 0,   1, 0}},
		{".text"      ,  0, {0, SHT_PROGBITS    ,  AX, 0, 0, 0, 0, 0,  64, 0}},
		{"net.text"   ,  1, {0, SHT_PROGBITS    , WAX, 0, 0, 0, 0, 0,  16, 0}}
	},
	{
		// Type   Of VA PA FS MS Flags               Align
		{PT_LOAD, 0, 0, 0, 0, 0, PF_R | PF_W | PF_X, 0x1000},
		{PT_LOAD, 0, 0, 0, 0, 0, PF_R | PF_W | PF_X, 0x1000}
	}
};

// For some reason the .bss section is of type PROGBITS for some levels.
extern const ElfFile DONOR_DL_LEVEL_ELF_PROGBITS_HEADERS = {
	{
		// Name         Seg  N  Type              Flag Ad Of Sz Lk In Algn ES
		{""           , -1, {0, SHT_NULL        ,   0, 0, 0, 0, 0, 0,  0,  0}},
		{".lit"       ,  0, {0, SHT_PROGBITS    , WAP, 0, 0, 0, 0, 0,  64, 0}},
		{".bss"       ,  0, {0, SHT_PROGBITS    , WAP, 0, 0, 0, 0, 0,  64, 0}},
		{".data"      ,  0, {0, SHT_PROGBITS    ,  WA, 0, 0, 0, 0, 0,  64, 0}},
		{"lvl.vtbl"   ,  0, {0, SHT_PROGBITS    ,   A, 0, 0, 0, 0, 0,   1, 0}},
		{"lvl.camvtbl",  0, {0, SHT_PROGBITS    ,   A, 0, 0, 0, 0, 0,   1, 0}},
		{"lvl.sndvtbl",  0, {0, SHT_PROGBITS    ,   A, 0, 0, 0, 0, 0,   1, 0}},
		{".text"      ,  0, {0, SHT_PROGBITS    ,  AX, 0, 0, 0, 0, 0,  64, 0}},
		{"net.text"   ,  1, {0, SHT_PROGBITS    , WAX, 0, 0, 0, 0, 0,  16, 0}}
	},
	{
		// Type   Of VA PA FS MS Flags               Align
		{PT_LOAD, 0, 0, 0, 0, 0, PF_R | PF_W | PF_X, 0x1000},
		{PT_LOAD, 0, 0, 0, 0, 0, PF_R | PF_W | PF_X, 0x1000}
	}
};
