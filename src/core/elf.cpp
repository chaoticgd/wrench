#include "elf.h"

enum class ElfIdentClass : u8 {
	B32 = 0x1,
	B64 = 0x2
};

enum class ElfFileType : u16 {
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

enum class ElfMachine : u16 {
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

ElfFile read_elf_file(Buffer src) {
	const ElfFileHeader& file_header = src.read<ElfFileHeader>(0, "ELF file header");
	verify(memcmp(file_header.magic, "\x7f\x45\x4c\x46", 4) == 0, "Magic bytes don't match.");
	
	ElfFile elf;
	elf.entry_point = file_header.entry;
	auto section_headers = src.read_multiple<ElfSectionHeader>(file_header.shoff, file_header.shnum, "ELF section headers");
	const ElfSectionHeader& name_section = section_headers[file_header.shstrndx];
	for(size_t i = 0; i < section_headers.size(); i++) {
		const ElfSectionHeader& section_header = section_headers[i];
		if(i != file_header.shstrndx) {
			ElfSection& section = elf.sections.emplace_back();
			section.header = section_header;
			section.data = src.read_bytes(section_header.offset, section_header.size, "ELF section data");
		}
	}
	
	auto program_headers = src.read_multiple<ElfProgramHeader>(file_header.phoff, file_header.phnum, "ELF program headers");
	for(const ElfProgramHeader& program_header : program_headers) {
		ElfProgramHeader& segment = elf.segments.emplace_back(program_header);
	}
	
	return elf;
}

void write_elf_file(OutBuffer dest, const ElfFile& elf) {
	std::vector<ElfSectionHeader> section_headers;
	std::vector<ElfProgramHeader> program_headers = elf.segments;
	
	// Initialise the section headers.
	section_headers.reserve(elf.sections.size());
	for(const ElfSection& section : elf.sections) {
		section_headers.emplace_back(section.header);
	}
	
	// Determine the layout of the file and write out the data.
	s64 file_header_ofs = dest.alloc<ElfFileHeader>();
	s64 program_headers_ofs = dest.alloc_multiple<ElfProgramHeader>(elf.segments.size());
	dest.pad(0x1000);
	for(size_t i = 0; i < elf.sections.size(); i++) {
		const ElfSection& section = elf.sections[i];
		// Insert padding.
		if(section.segment > -1) {
			if(i > 0) {
				const ElfSection& last_section = elf.sections[i - 1];
				if(last_section.segment == section.segment) {
					// Make sure the address field matches up with the offset
					// between different sections so they form a valid segment.
					s32 padding_size = section.header.addr - (last_section.header.addr + last_section.data.size());
					if(padding_size > 0) {
						for(s32 j = 0; j < padding_size; j++) {
							dest.write<u8>(0);
						}
					} else if(padding_size < 0) {
						fprintf(stderr, "warning: Padding calculation gave negative result, segments may be incorrect.\n");
					}
				}
			} else {
				dest.pad(0x80);
			}
		}
		// Write the data.
		s64 offset = dest.write_multiple(section.data);
		section_headers[i].offset = (s32) offset;
		section_headers[i].size = (s32) section.data.size();
	}
	s64 section_header_names_ofs = dest.tell();
	dest.write<char>('\0');
	for(size_t i = 0; i < elf.sections.size(); i++) {
		s64 offset = dest.tell();
		dest.writesf("%s", elf.sections[i].name.c_str());
		dest.write<char>('\0');
		section_headers[i].name = (s32) (offset - section_header_names_ofs);
	}
	s64 shstrtab_string_ofs = dest.tell();
	dest.writesf(".shstrtab");
	dest.write<char>('\0');
	s64 section_header_names_end = dest.tell();
	s64 section_headers_ofs = dest.alloc_multiple<ElfSectionHeader>(elf.sections.size() + 1);
	
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
	file_header.phoff = (s32) program_headers_ofs;
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

ElfFile read_ratchet_executable(Buffer src) {
	ElfFile elf;
	s32 ofs = 0;
	for(s32 i = 0; ofs < src.size(); i++) {
		// Read the block header, set the entry poin, and check for EOF.
		RatchetSectionHeader header = src.read<RatchetSectionHeader>(ofs, "ratchet section header");
		ofs += sizeof(RatchetSectionHeader);
		if(elf.entry_point == 0) {
			elf.entry_point = header.entry_point;
		} else if(header.entry_point != elf.entry_point) {
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

void write_ratchet_executable(const ElfFile& elf) {
	
}

bool recover_deadlocked_section_info(ElfFile& elf) {
	static const u32 ax = SHF_ALLOC | SHF_EXECINSTR;
	static const u32 wa = SHF_WRITE | SHF_ALLOC;
	static const u32 a = SHF_ALLOC;
	static const u32 wap = SHF_WRITE | SHF_ALLOC | SHF_MASKPROC;
	static const u32 wax = SHF_WRITE | SHF_ALLOC | SHF_EXECINSTR;
	static const ElfSection expected_sections[17] = {
		//               Seg  N  Type              Flag Ad Of Sz Lk In Algn ES
		{".reginfo"    , -1, {0, SHT_MIPS_REGINFO,   0, 0, 0, 0, 0, 0,   4, 1}},
		{".vutext"     ,  0, {0, SHT_PROGBITS    ,  ax, 0, 0, 0, 0, 0,  16, 0}},
		{"core.text"   ,  0, {0, SHT_PROGBITS    ,  ax, 0, 0, 0, 0, 0,  64, 0}},
		{"core.data"   ,  0, {0, SHT_PROGBITS    ,  wa, 0, 0, 0, 0, 0, 128, 0}},
		{"core.rdata"  ,  0, {0, SHT_PROGBITS    ,   a, 0, 0, 0, 0, 0,  16, 0}},
		{"core.bss"    ,  0, {0, SHT_NOBITS      , wap, 0, 0, 0, 0, 0,  64, 0}},
		{"core.lit"    ,  0, {0, SHT_PROGBITS    , wap, 0, 0, 0, 0, 0,   8, 0}},
		{".lit"        ,  0, {0, SHT_PROGBITS    , wap, 0, 0, 0, 0, 0,  64, 0}},
		{".bss"        ,  0, {0, SHT_NOBITS      , wap, 0, 0, 0, 0, 0,  64, 0}},
		{".data"       ,  0, {0, SHT_PROGBITS    ,  wa, 0, 0, 0, 0, 0,  64, 0}},
		{"lvl.vtbl"    ,  0, {0, SHT_PROGBITS    ,   a, 0, 0, 0, 0, 0,   1, 0}},
		{"lvl.camvtbl" ,  0, {0, SHT_PROGBITS    ,   a, 0, 0, 0, 0, 0,   1, 0}},
		{"lvl.sndvtbl" ,  0, {0, SHT_PROGBITS    ,   a, 0, 0, 0, 0, 0,   1, 0}},
		{".text"       ,  0, {0, SHT_PROGBITS    ,  ax, 0, 0, 0, 0, 0,  64, 0}},
		{"patch.data"  ,  0, {0, SHT_MIPS_REGINFO,  ax, 0, 0, 0, 0, 0,   4, 1}},
		{"net.text"    ,  1, {0, SHT_PROGBITS    , wax, 0, 0, 0, 0, 0,  16, 0}},
		{"net.nostomp" ,  1, {0, SHT_PROGBITS    , wax, 0, 0, 0, 0, 0,   8, 0}}
	};
	
	if(elf.sections.size() != ARRAY_SIZE(expected_sections))
		return false;
	
	for(s32 i = 0; i < ARRAY_SIZE(expected_sections); i++)
		if(elf.sections[i].header.type != expected_sections[i].header.type)
			return false;
	
	for(s32 i = 0; i < ARRAY_SIZE(expected_sections); i++) {
		ElfSection& section = elf.sections[i];
		const ElfSection& expected = expected_sections[i];
		section.name = expected.name;
		section.header.flags = expected.header.flags;
		section.header.addralign = expected.header.addralign;
		section.header.entsize = expected.header.entsize;
	}
	
	return true;
}
