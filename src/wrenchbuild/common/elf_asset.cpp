/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2023 chaoticgd

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

#include <core/elf.h>
#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>

static void unpack_elf_asset(ElfFileAsset& dest, InputStream& src, BuildConfig config, const char* hint);
static void pack_elf_asset(
	OutputStream& dest, const ElfFileAsset& src, BuildConfig config, const char* hint);
static bool extract_file(std::vector<u8>& ratchet, const std::vector<u8>& packed);

on_load(ElfFile, []() {
	ElfFileAsset::funcs.unpack_rac1 = wrap_hint_unpacker_func<ElfFileAsset>(unpack_elf_asset);
	ElfFileAsset::funcs.unpack_rac2 = wrap_hint_unpacker_func<ElfFileAsset>(unpack_elf_asset);
	ElfFileAsset::funcs.unpack_rac3 = wrap_hint_unpacker_func<ElfFileAsset>(unpack_elf_asset);
	ElfFileAsset::funcs.unpack_dl = wrap_hint_unpacker_func<ElfFileAsset>(unpack_elf_asset);
	
	ElfFileAsset::funcs.pack_rac1 = wrap_hint_packer_func<ElfFileAsset>(pack_elf_asset);
	ElfFileAsset::funcs.pack_rac2 = wrap_hint_packer_func<ElfFileAsset>(pack_elf_asset);
	ElfFileAsset::funcs.pack_rac3 = wrap_hint_packer_func<ElfFileAsset>(pack_elf_asset);
	ElfFileAsset::funcs.pack_dl = wrap_hint_packer_func<ElfFileAsset>(pack_elf_asset);
})

static void unpack_elf_asset(ElfFileAsset& dest, InputStream& src, BuildConfig config, const char* hint) {
	const char* type = next_hint(&hint);
	bool convert_from_packed_executable = strcmp(type, "packed") == 0;
	bool convert_from_ratchet_executable = strcmp(type, "ratchetexecutable") == 0;
	
	std::string path = dest.tag() + ".elf";
	auto [stream, ref] = dest.file().open_binary_file_for_writing(path);
	verify(stream.get(), "Cannot open ELF file '%s' for writing.", path.c_str());
	dest.set_src(ref);
	
	if (convert_from_packed_executable) {
		std::vector<u8> packed_bytes = src.read_multiple<u8>(0, src.size());
		std::vector<u8> ratchet_bytes;
		if (extract_file(ratchet_bytes, packed_bytes)) {
			ElfFile elf = read_ratchet_executable(ratchet_bytes);
			const ElfFile* donor_elf = nullptr;
			switch (config.game()) {
				case Game::RAC:
				case Game::GC:
					break;
				case Game::UYA: {
					donor_elf = &DONOR_UYA_BOOT_ELF_HEADERS;
					break;
				}
				case Game::DL: {
					donor_elf = &DONOR_DL_BOOT_ELF_HEADERS;
					break;
				}
				case Game::UNKNOWN: {}
			}
			verify(donor_elf && fill_in_elf_headers(elf, *donor_elf),
				"Failed to recover ELF section headers for the boot ELF!");
			std::vector<u8> elf_bytes;
			write_elf_file(elf_bytes, elf);
			stream->write_v(elf_bytes);
		} else {
			// The file isn't packed, so just copy the raw ELF.
			src.seek(0);
			Stream::copy(*stream, src, src.size());
		}
	} else if (convert_from_ratchet_executable) {
		std::vector<u8> ratchet_bytes = src.read_multiple<u8>(0, src.size());
		ElfFile elf = read_ratchet_executable(ratchet_bytes);
		const ElfFile* donor_elf = nullptr;
		if (config.game() == Game::DL) {
			if (elf.sections.size() >= 3 && elf.sections[2].header.type == SHT_NOBITS) {
				donor_elf = &DONOR_DL_LEVEL_ELF_NOBITS_HEADERS;
			} else {
				donor_elf = &DONOR_DL_LEVEL_ELF_PROGBITS_HEADERS;
			}
		} else {
			donor_elf = &DONOR_RAC_GC_UYA_LEVEL_ELF_HEADERS;
		}
		verify(fill_in_elf_headers(elf, *donor_elf),
			"Failed to recover ELF section headers for the level code!");
		std::vector<u8> elf_bytes;
		write_elf_file(elf_bytes, elf);
		stream->write_v(elf_bytes);
	} else {
		src.seek(0);
		Stream::copy(*stream, src, src.size());
	}
}

static void pack_elf_asset(OutputStream& dest, const ElfFileAsset& src, BuildConfig config, const char* hint)
{
	if (g_asset_packer_dry_run) {
		return;
	}
	
	bool convert_to_ratchet_executable = strcmp(next_hint(&hint), "ratchetexecutable") == 0;
	
	auto stream = src.src().open_binary_file_for_reading();
	verify(stream.get(), "Cannot open ELF file '%s' for reading.", src.src().path.string().c_str());
	
	if (convert_to_ratchet_executable) {
		std::vector<u8> elf_bytes = stream->read_multiple<u8>(0, stream->size());
		ElfFile elf = read_elf_file(elf_bytes);
		std::vector<u8> ratchet_bytes;
		write_ratchet_executable(ratchet_bytes, elf);
		dest.write_v(ratchet_bytes);
	} else {
		Stream::copy(dest, *stream, stream->size());
	}
}

static bool extract_file(std::vector<u8>& ratchet, const std::vector<u8>& packed)
{
	s64 wad_ofs = -1;
	for (s64 i = 0; i < packed.size() - 3; i++) {
		if (memcmp(&packed.data()[i], "WAD", 3) == 0) {
			wad_ofs = i;
			break;
		}
	}
	if (wad_ofs > -1) {
		decompress_wad(ratchet, WadBuffer{packed.data() + wad_ofs, packed.data() + packed.size()});
		return true;
	} else {
		return false;
	}
}
