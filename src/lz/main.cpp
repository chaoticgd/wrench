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

#include <sstream>

#include "../editor/util.h"
#include "../editor/command_line.h"
#include "compression.h"

// CLI tool to decompress and recompress WAD LZ segments.
// Not to be confused with the *.WAD files in R&C2's filesystem.

static int run_test();

int main(int argc, char** argv) {
	cxxopts::Options options(argv[0], "Compress and decompress WAD LZ segments.");
	options.positional_help("compress|decompress|test [<input file> <output file>]");
	options.add_options()
		("c,command", "The operation to perform. Possible values are: compress, decompress, test.",
			cxxopts::value<std::string>())
		("s,src", "The input file.",
			cxxopts::value<std::string>())
		("d,dest", "The output file.", cxxopts::value<std::string>())
		("o,offset", "The offset in the input file where the header begins. Only applies for decompression.",
			cxxopts::value<std::string>()->default_value("0"))
		("t,threads", "The number of threads to use. Only applies for compression.",
			cxxopts::value<std::string>()->default_value("1"));

	options.parse_positional({
		"command", "src", "dest"
	});

	auto args = parse_command_line_args(argc, argv, options);
	std::string command = cli_get(args, "command");
	std::string src_path = cli_get_or(args, "src", "");
	std::string dest_path = cli_get_or(args, "dest", "");
	size_t offset = parse_number(cli_get_or(args, "offset", "0"));
	size_t thread_count = parse_number(cli_get_or(args, "threads", "1"));
	
	if(thread_count < 1) {
		std::cerr << "You must choose a positive number of threads. Defaulting to 1.\n";
		thread_count = 1;
	}
	
	if(command == "test") {
		return run_test();
	}
	
	bool decompress = command == "decompress";
	bool compress = command == "compress";
	if(!decompress && !compress) {
		std::cerr << "Invalid command.\n";
		return 1;
	}
	
	file_stream src_file(src_path);
	file_stream dest_file(dest_path, std::ios::out | std::ios::trunc);
	
	std::vector<uint8_t> dest, src;
	if(decompress) {
		uint32_t compressed_size = src_file.read<uint32_t>(offset + 0x3);
		src.resize(compressed_size);
		src_file.seek(offset);
		src_file.read_v(src);
		decompress_wad(dest, src);
		
	} else {
		src.resize(src_file.size());
		src_file.read_v(src);
		compress_wad(dest, src, thread_count);
	}
	
	dest_file.write_v(dest);

	return 0;
}

static const int TEST_ITERATIONS = 128;

static int run_test() {
	printf("**** compression test ****\n");
	
	srand(time(NULL));
	
	int happy = 0, sad = 0;
	
	for(int i = 0; i < TEST_ITERATIONS; i++) {
		int buffer_size = rand() % (64 * 1024);
		
		std::vector<uint8_t> plaintext(buffer_size);
		for(int j = 0; j < buffer_size; j++) {
			if(rand() % 8 <= 0) {
				plaintext[j] = rand();
			} else {
				plaintext[j] = 0;
			}
		}
		
		auto write_sad_file = [&]() {
			sad++;
			std::string sad_file_path = "/tmp/wad_is_sad_" + std::to_string(sad) + ".bin";
			file_stream sad_file(sad_file_path, std::ios::out);
			sad_file.write_v(plaintext);
			printf("Written sad file to %s\n", sad_file_path.c_str());
		};
		
		std::vector<uint8_t> compressed;
		try {
			int thread_count = 1 + (rand() % 15);
			compress_wad(compressed, plaintext, thread_count);
		} catch(std::exception& e) {
			printf("compress_wad threw: %s\n", e.what());
			write_sad_file();
			continue;
		}
		
		std::vector<uint8_t> output;
		try {
			decompress_wad(output, compressed);
		} catch(std::exception& e) {
			printf("decompress_wad threw: %s\n", e.what());
			write_sad_file();
			continue;
		}
		
		if(plaintext != output) {
			write_sad_file();
			continue;
		}
		
		happy++;
	}
	
	printf("results: %d happy, %d sad\n", happy, sad);
	return sad != 0;
}
