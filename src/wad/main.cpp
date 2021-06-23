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

#include <fstream>

#include "util.h"
#include "wad_file.h"

static void run_extractor(fs::path input_path, fs::path output_path);
static void run_test(fs::path input_path);

static std::vector<u8> read_header(FILE* file);
static WadFileDescription match_wad(FILE* file, const std::vector<u8>& header);
static std::vector<u8> read_lump(FILE* file, Sector32 offset, Sector32 size);
static void write_file(const char* path, Buffer buffer);

int main(int argc, char** argv) {
	verify(argc == 3 || argc == 4, "Wrong number of arguments.");
	
	std::string mode = argv[1];
	fs::path input_path = argv[2];
	
	if(mode == "extract") {
		run_extractor(input_path, argc == 4 ? argv[3] : "wad_extracted");
	} else if(mode == "test") {
		run_test(input_path);
	} else {
		verify_not_reached("Invalid command: %s", mode.c_str());
	}
}

static void run_extractor(fs::path input_path, fs::path output_path) {
	FILE* file = fopen(input_path.string().c_str(), "rb");
	verify(file, "Failed to open input file.");
	
	const std::vector<u8> header = read_header(file);
	const WadFileDescription file_desc = match_wad(file, header);
	std::unique_ptr<Wad> wad = file_desc.create();
	assert(wad.get());
	for(const WadLumpDescription& lump_desc : file_desc.fields) {
		for(s32 i = 0; i < lump_desc.count; i++) {
			auto& [offset, size] = Buffer(header).read<SectorRange>(lump_desc.offset + i * 8, "WAD header");
			if(size.sectors != 0) {
				std::vector<u8> src = read_lump(file, offset, size);
				verify(lump_desc.types.read(lump_desc, *wad.get(), src), "Failed to convert lump.");
			}
		}
	}
	
	for(auto& [name, asset] : wad->binary_assets) {
		if(asset.is_array) {
			fs::path dir = output_path/name;
			fs::create_directories(dir);
			for(size_t i = 0; i < asset.buffers.size(); i++) {
				fs::path path = dir/(std::to_string(i) + ".bin");
				write_file(path.string().c_str(), asset.buffers[i]);
			}
		} else {
			assert(asset.buffers.size() == 1);
			fs::path path = output_path/(name + ".bin");
			write_file(path.string().c_str(), asset.buffers[0]);
		}
	}
	
	fclose(file);
}

static void run_test(fs::path input_path) {
	FILE* file = fopen(input_path.string().c_str(), "rb");
	verify(file, "Failed to open input file.");
	
	const std::vector<u8> header = read_header(file);
	const WadFileDescription file_desc = match_wad(file, header);
	
	std::unique_ptr<Wad> wad = file_desc.create();
	assert(wad.get());
	
	std::optional<WadLumpDescription> gameplay_desc;
	for(const WadLumpDescription& lump_desc : file_desc.fields) {
		if(strcmp(lump_desc.name, "gameplay_core") == 0) {
			gameplay_desc = lump_desc;
		}
	}
	verify(gameplay_desc.has_value(), "The given WAD doesn't contain a gameplay file.");
	
	auto& [offset, size] = Buffer(header).read<SectorRange>(gameplay_desc->offset, "WAD header");
	std::vector<u8> compressed = read_lump(file, offset, size);
	std::vector<u8> src;
	verify(decompress_wad(src, compressed), "Decompressing gameplay file failed.");
	Gameplay gameplay;
	verify(read_gameplay(gameplay, src), "Failed to parse gameplay file.");
	std::vector<u8> dest = write_gameplay(gameplay);
	
	diff_buffers(src, dest, "Gameplay");
}

static std::vector<u8> read_header(FILE* file) {
	const char* ERR_READ_HEADER = "Failed to read header.";
	s32 header_size;
	verify(fseek(file, 0, SEEK_SET) == 0, ERR_READ_HEADER);
	verify(fread(&header_size, 4, 1, file) == 1, ERR_READ_HEADER);
	verify(header_size < 0x10000, "Invalid header.");
	std::vector<u8> header(header_size);
	verify(fseek(file, 0, SEEK_SET) == 0, ERR_READ_HEADER);
	verify(fread(header.data(), header.size(), 1, file) == 1, ERR_READ_HEADER);
	return header;
}

static WadFileDescription match_wad(FILE* file, const std::vector<u8>& header) {
	for(const WadFileDescription& desc : wad_files) {
		if(desc.header_size == header.size()) {
			return desc;
		}
	}
	verify_not_reached("Unable to identify WAD file.");
}

static std::vector<u8> read_lump(FILE* file, Sector32 offset, Sector32 size) {
	const char* ERR_READ_BLOCK = "Failed to read lump.";
	
	std::vector<u8> lump(size.bytes());
	verify(fseek(file, offset.bytes(), SEEK_SET) == 0, ERR_READ_BLOCK);
	verify(fread(lump.data(), lump.size(), 1, file) == 1, ERR_READ_BLOCK);
	return lump;
}

static void write_file(const char* path, Buffer buffer) {
	FILE* file = fopen(path, "wb");
	verify(file, "Failed to open file '%s' for writing.", path);
	assert(buffer.size() > 0);
	verify(fwrite(buffer.lo, buffer.size(), 1, file) == 1, "Failed to write output file '%s'.", path);
	fclose(file);
	printf("Wrote %s (%ld KiB)\n", path, buffer.size() / 1024);
}
