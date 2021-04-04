/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2020 chaoticgd

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

#include "../app.h"

#include <stdlib.h>

void world_segment_test() {
	printf("**** world segment test ****\n");
	
	const char* dir = getenv("GAME_DIR");
	if(dir == nullptr) {
		// If no ISO file is provided, we can't run this test.
		printf("No ISO file provided, test skipped.\n");
		printf("To run this test, you must set the GAME_DIR enviroment variable.\n");
		return;
	}
	
	auto levels_dir = fs::path(dir) / "levels";
	
	int happy = 0, sad = 0;
	for(auto& iter : fs::directory_iterator(levels_dir)) {
		file_stream lvl_file(iter.path());
		
		level lvl;
		lvl.read(lvl_file, iter.path());
		
		array_stream original;
		lvl.moby_stream()->seek(0);
		stream::copy_n(original, *lvl.moby_stream(), lvl.moby_stream()->size());
		array_stream built;
		lvl.world.write_rac23(built);
		
		if(!array_stream::compare_contents(original, built)) {
			sad++;
			
			std::string original_path = "/tmp/" + iter.path().filename().string() + "_worldseg_original.bin";
			file_stream original_file(original_path, std::ios::out);
			original.seek(0);
			stream::copy_n(original_file, original, original.size());
			printf("Written original file to %s\n", original_path.c_str());
			
			std::string built_path = "/tmp/" + iter.path().filename().string() + "_worldseg_built.bin";
			file_stream built_file(built_path, std::ios::out);
			built.seek(0);
			stream::copy_n(built_file, built, built.size());
			printf("Written built file to %s\n", built_path.c_str());
		} else {
			happy++;
		}
	}
	
	printf("results: %d happy, %d sad\n", happy, sad);
}
