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

#include <time.h>
#include <stdlib.h>

#include "../formats/wad.h"

static const int TEST_ITERATIONS = 128;

void compression_test_iter(int buffer_size, int& happy, int& sad);

void compression_test() {
	printf("**** compression test ****\n");
	
	srand(time(NULL));
	
	int happy = 0, sad = 0;
	
	for(int i = 0; i < TEST_ITERATIONS / 8; i++) {
		compression_test_iter(0, happy, sad);
		compression_test_iter(1, happy, sad);
		compression_test_iter(4, happy, sad);
		compression_test_iter(8, happy, sad);
		compression_test_iter(1024, happy, sad);
		compression_test_iter(4 * 1024, happy, sad);
		compression_test_iter(8 * 1024, happy, sad);
		compression_test_iter(16 * 1024, happy, sad);
	}
	
	printf("results: %d happy, %d sad", happy, sad);
}

void compression_test_iter(int buffer_size, int& happy, int& sad) {
	array_stream plaintext;
	for(int j = 0; j < buffer_size; j++) {
		if(rand() % 8 <= 0) {
			plaintext.write8(rand());
		} else {
			plaintext.write8(0);
		}
	}
	
	auto write_sad_file = [&]() {
		sad++;
		std::string sad_file_path = "/tmp/wad_is_sad_" + std::to_string(sad) + ".bin";
		file_stream sad_file(sad_file_path, std::ios::out);
		plaintext.seek(0);
		stream::copy_n(sad_file, plaintext, plaintext.size());
		printf("Written sad file to %s\n", sad_file_path.c_str());
	};
	
	array_stream compressed;
	try {
		compress_wad(compressed, plaintext);
	} catch(std::exception& e) {
		printf("compress_wad threw: %s\n", e.what());
		write_sad_file();
		return;
	}
	
	array_stream output;
	try {
		decompress_wad(output, compressed);
	} catch(std::exception& e) {
		printf("decompress_wad threw: %s\n", e.what());
		write_sad_file();
		return;
	}
	
	if(plaintext.buffer.size() != output.buffer.size()) {
		write_sad_file();
		return;
	}
	
	bool happy_this_time = true;
	for(int j = 0; j < buffer_size; j++) {
		if(plaintext.buffer[j] != output.buffer[j]) {
			write_sad_file();
			happy_this_time = false;
			break;
		}
	}
	
	if(happy_this_time) {
		happy++;
	}
}
