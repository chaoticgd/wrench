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

#ifndef CORE_ELF_H
#define CORE_ELF_H

#include <core/buffer.h>

enum ElfSectionType : u32
{
	SHT_NULL = 0x0,
	SHT_PROGBITS = 0x1,
	SHT_SYMTAB = 0x2,
	SHT_STRTAB = 0x3,
	SHT_RELA = 0x4,
	SHT_HASH = 0x5,
	SHT_DYNAMIC = 0x6,
	SHT_NOTE = 0x7,
	SHT_NOBITS = 0x8,
	SHT_REL = 0x9,
	SHT_SHLIB = 0xa,
	SHT_DYNSYM = 0xb,
	SHT_INIT_ARRAY = 0xe,
	SHT_FINI_ARRAY = 0xf,
	SHT_PREINIT_ARRAY = 0x10,
	SHT_GROUP = 0x11,
	SHT_SYMTAB_SHNDX = 0x12,
	SHT_NUM = 0x13,
	SHT_LOOS = 0x60000000,
	SHT_MIPS_DEBUG = 0x70000005,
	SHT_MIPS_REGINFO = 0x70000006
};

enum ElfSectionFlags
{
	SHF_WRITE = (1 << 0),
	SHF_ALLOC = (1 << 1),
	SHF_EXECINSTR = (1 << 2),
	SHF_MERGE = (1 << 4),
	SHF_STRINGS = (1 << 5),
	SHF_INFO_LINK = (1 << 6),
	SHF_LINK_ORDER = (1 << 7),
	SHF_OS_NONCONFORMING = (1 << 8),
	SHF_GROUP = (1 << 9),
	SHF_TLS = (1 << 10),
	SHF_COMPRESSED = (1 << 11),
	SHF_MASKOS = 0x0ff00000,
	SHF_MASKPROC = 0xf0000000,
	SHF_GNU_RETAIN = (1 << 21),
	SHF_ORDERED = (1 << 30),
	SHF_EXCLUDE = (1U << 31)
};

packed_struct(ElfSectionHeader,
	/* 0x00 */ s32 name;
	/* 0x04 */ ElfSectionType type;
	/* 0x08 */ u32 flags;
	/* 0x0c */ s32 addr;
	/* 0x10 */ s32 offset;
	/* 0x14 */ s32 size;
	/* 0x18 */ s32 link;
	/* 0x1c */ s32 info;
	/* 0x20 */ s32 addralign;
	/* 0x24 */ s32 entsize;
)

struct ElfSection
{
	std::string name;
	s32 segment = -1;
	ElfSectionHeader header = {};
	std::vector<u8> data;
};

enum ElfProgramHeaderType : u32
{
	PT_NULL = 0,
	PT_LOAD = 1,
	PT_DYNAMIC = 2,
	PT_INTERP = 3,
	PT_NOTE = 4,
	PT_SHLIB = 5,
	PT_PHDR = 6,
	PT_TLS = 7,
	PT_NUM = 8,
	PT_LOOS = 0x60000000,
	PT_GNU_EH_FRAME = 0x6474e550,
	PT_GNU_STACK = 0x6474e551,
	PT_GNU_RELRO = 0x6474e552,
	PT_GNU_PROPERTY = 0x6474e553,
	PT_LOSUNW = 0x6ffffffa,
	PT_SUNWBSS = 0x6ffffffa,
	PT_SUNWSTACK = 0x6ffffffb,
	PT_HISUNW = 0x6fffffff,
	PT_HIOS = 0x6fffffff,
	PT_LOPROC = 0x70000000,
	PT_HIPROC = 0x7fffffff
};

enum ElfProgramHeaderFlags
{
	PF_X = (1 << 0),
	PF_W = (1 << 1),
	PF_R = (1 << 2),
	PF_MASKO = 0x0ff00000,
	PF_MASKPRO = 0xf0000000
};

packed_struct(ElfProgramHeader,
	/* 0x00 */ ElfProgramHeaderType type;
	/* 0x04 */ s32 offset;
	/* 0x08 */ s32 vaddr;
	/* 0x0c */ s32 paddr;
	/* 0x10 */ s32 filesz;
	/* 0x14 */ s32 memsz;
	/* 0x18 */ u32 flags;
	/* 0x1c */ s32 align;
)

struct ElfFile
{
	std::vector<ElfSection> sections;
	std::vector<ElfProgramHeader> segments;
	s32 entry_point = 0;
};

ElfFile read_elf_file(Buffer src);
void write_elf_file(OutBuffer dest, const ElfFile& elf);

ElfFile read_ratchet_executable(Buffer src);
void write_ratchet_executable(OutBuffer dest, const ElfFile& elf);

bool fill_in_elf_headers(ElfFile& elf, const ElfFile& donor);

extern const ElfFile DONOR_UYA_BOOT_ELF_HEADERS;
extern const ElfFile DONOR_DL_BOOT_ELF_HEADERS;
extern const ElfFile DONOR_RAC_GC_UYA_LEVEL_ELF_HEADERS;
extern const ElfFile DONOR_DL_LEVEL_ELF_NOBITS_HEADERS;
extern const ElfFile DONOR_DL_LEVEL_ELF_PROGBITS_HEADERS;

#endif
