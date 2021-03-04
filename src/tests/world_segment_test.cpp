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
#include "../project.h"

#include <stdlib.h>

void world_segment_test() {
	printf("**** world segment test ****\n");
	
	const char* iso_md5 = getenv("TEST_ISO_MD5");
	if(iso_md5 == nullptr) {
		// If no ISO file is provided, we can't run this test.
		printf("No ISO file provided, test skipped.\n");
		printf("To run this test, you must set the TEST_ISO_MD5 enviroment variable.\n");
		return;
	}
	
	config::get().read();
	std::optional<game_iso> iso;
	for(game_iso current_iso : config::get().game_isos) {
		if(current_iso.md5 == std::string(iso_md5)) {
			iso = current_iso;
		}
	}
	if(!iso) {
		fprintf(stderr, "error: Cannot find ISO in config file!\n");
		return;
	}
	
	worker_logger dummy;
	wrench_project project(*iso, dummy);
	
	int happy = 0, sad = 0;
	
	for(size_t i = 0; i < project.toc.levels.size(); i++) {
		project.open_level(i);
		level* lvl = project.selected_level();
		assert(lvl != nullptr);
		
		array_stream before;
		lvl->moby_stream()->seek(0);
		stream::copy_n(before, *lvl->moby_stream(), lvl->moby_stream()->size());
		array_stream after;
		lvl->world.write_rac23(after);
		
		if(!array_stream::compare_contents(before, after)) {
			sad++;
			
			std::string before_path = "/tmp/l" + std::to_string(i) + "_worldseg_before.bin";
			file_stream before_file(before_path, std::ios::out);
			before.seek(0);
			stream::copy_n(before_file, before, before.size());
			printf("Written before file to %s\n", before_path.c_str());
			
			std::string after_path = "/tmp/l" + std::to_string(i) + "_worldseg_after.bin";
			file_stream after_file(after_path, std::ios::out);
			after.seek(0);
			stream::copy_n(after_file, after, after.size());
			printf("Written after file to %s\n", after_path.c_str());
		} else {
			happy++;
		}
	}
	
	printf("results: %d happy, %d sad\n", happy, sad);
}
