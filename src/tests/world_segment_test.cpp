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
	}
	
	worker_logger dummy;
	wrench_project project(*iso, dummy);
	
	int happy = 0, sad = 0;
	
	for(size_t i = 0; i < project.toc.levels.size(); i++) {
		project.open_level(i);
		level* lvl = project.selected_level();
		assert(lvl != nullptr);
		
		std::string before = md5_from_stream(*lvl->moby_stream());
		lvl->write();
		std::string after = md5_from_stream(*lvl->moby_stream());
		
		if(before != after) {
			sad++;
			printf("Level %ld was written incorrectly!\n", i);
		} else {
			happy++;
		}
	}
	
	printf("results: %d happy, %d sad\n", happy, sad);
}
