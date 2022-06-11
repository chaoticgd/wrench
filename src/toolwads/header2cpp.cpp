/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2022 chaoticgd

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

#include <vector>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>

int main(int argc, char** argv) {
	assert(argc > 2);
	
	FILE* dest = fopen(argv[1], "wb");
	assert(dest);
	
	fprintf(dest, "extern \"C\"{alignas(16) unsigned char wadinfo[]={");
	for(int i = 2; i < argc; i++) {
		FILE* src = fopen(argv[i], "rb");
		assert(src);
		
		int32_t header_size;
		assert(fread(&header_size, 4, 1, src) == 1);
		assert(fseek(src, 0, SEEK_SET) == 0);
		assert(header_size < 0x10000);
		
		std::vector<char> header(header_size);
		assert(fread(header.data(), header_size, 1, src) == 1);
		
		for(char byte : header) {
			fprintf(dest, "0x%hhx,", byte);
		}
	}
	fprintf(dest, "};}\n");
	
	return 0;
}
