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

#include "tests.h"

static std::optional<WadLumpDescription> find_lump(WadFileDescription file_desc, const char* name);
struct GameplayTestArgs {
	std::string wad_file_path;
	FILE* file;
	SectorRange lump = {0, 0};
	const char* name;
	const std::vector<GameplayBlockDescription>* blocks;
	bool compressed;
};
static void run_gameplay_lump_test(GameplayTestArgs args);

void run_tests(fs::path input_path) {
	for(fs::path wad_file_path : fs::directory_iterator(input_path)) {
		FILE* file = fopen(wad_file_path.string().c_str(), "rb");
		verify(file, "Failed to open input file.");
		
		const std::vector<u8> header = read_header(file);
		const WadFileDescription file_desc = match_wad(file, header);
		
		std::unique_ptr<Wad> wad = file_desc.create();
		assert(wad.get());
		
		auto build_args = [&](s32 header_offset, const char* name, const std::vector<GameplayBlockDescription>& blocks, bool compressed) {
			GameplayTestArgs args;
			args.wad_file_path = wad_file_path.string();
			args.file = file;
			args.lump = Buffer(header).read<SectorRange>(header_offset, "WAD header");
			args.name = name;
			args.blocks = &blocks;
			args.compressed = compressed;
			return args;
		};
		
		auto gameplay_core = find_lump(file_desc, "gameplay_core");
		assert(gameplay_core.has_value());
		run_gameplay_lump_test(build_args(gameplay_core->offset, gameplay_core->name, GAMEPLAY_CORE_BLOCKS, true));
		
		auto art_instances = find_lump(file_desc, "art_instances");
		assert(art_instances.has_value());
		run_gameplay_lump_test(build_args(art_instances->offset, art_instances->name, ART_INSTANCE_BLOCKS, true));
		
		auto missions_instances = find_lump(file_desc, "gameplay_mission_instances");
		assert(missions_instances.has_value());
		for(s32 i = 0; i < missions_instances->count; i++) {
			std::string name = std::string(missions_instances->name) + " " + std::to_string(i);
			run_gameplay_lump_test(build_args(missions_instances->offset + i * 8, name.c_str(), GAMEPLAY_MISSION_INSTANCE_BLOCKS, false));
		}
	}
}

static std::optional<WadLumpDescription> find_lump(WadFileDescription file_desc, const char* name) {
	for(const WadLumpDescription& lump_desc : file_desc.fields) {
		if(strcmp(lump_desc.name, name) == 0) {
			return lump_desc;
		}
	}
	return {};
}

static void run_gameplay_lump_test(GameplayTestArgs args) {
	if(args.lump.offset.sectors == 0) {
		return;
	}
	std::vector<u8> raw = read_lump(args.file, args.lump.offset, args.lump.size);
	std::vector<u8> src;
	if(args.compressed) {
		verify(decompress_wad(src, raw), "Decompressing %s file failed.", args.name);
	} else {
		src = std::move(raw);
	}
	Gameplay gameplay;
	read_gameplay(gameplay, src, *args.blocks);
	std::vector<u8> dest = write_gameplay(gameplay, *args.blocks);
	
	Buffer dest_buf(dest);
	Buffer src_buf(src);
	
	bool good = true;
	
	std::string gameplay_header_str = args.wad_file_path + " " + args.name + " header";
	std::string gameplay_data_str = args.wad_file_path + " " + args.name + " data";
	good &= diff_buffers(src_buf.subbuf(0, 0x80), dest_buf.subbuf(0, 0x80), 0, gameplay_header_str.c_str());
	good &= diff_buffers(src_buf.subbuf(0x80), dest_buf.subbuf(0x80), 0x80, gameplay_data_str.c_str());
	
	if(!good) {
		FILE* gameplay_file = fopen("/tmp/gameplay.bin", "wb");
		verify(gameplay_file, "Failed to open /tmp/gameplay.bin for writing.");
		fwrite(src.data(), src.size(), 1, gameplay_file);
		fclose(gameplay_file);
		exit(1);
	}
}
