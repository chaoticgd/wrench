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

#include "../util.h"
#include "../command_line.h"
#include "compression.h"

// CLI tool to decompress and recompress WAD segments.
// Not to be confused with the *.WAD files in R&C2's filesystem.

void copy_and_decompress(stream& dest, stream& src);
void copy_and_compress(stream& dest, stream& src);

int main(int argc, char** argv) {
	cxxopts::Options options(argv[0], "Compress and decompress WAD segments.");
	options.positional_help("compress|decompress <input file> <output file>");
	options.add_options()
		("c,command", "The operation to perform. Possible values are: compress, decompress.",
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
	std::string src_path = cli_get(args, "src");
	std::string dest_path = cli_get(args, "dest");
	size_t offset = parse_number(cli_get_or(args, "offset", "0"));
	size_t thread_count = parse_number(cli_get_or(args, "threads", "1"));
	
	if(thread_count < 1) {
		std::cerr << "You must choose a positive number of threads. Defaulting to 1.\n";
		thread_count = 1;
	}
	
	bool decompress = command == "decompress";
	bool compress = command == "compress";
	if(!decompress && !compress) {
		std::cerr << "Invalid command.\n";
		return 1;
	}
	
	file_stream src_file(src_path);
	file_stream dest_file(dest_path, std::ios::in | std::ios::out | std::ios::trunc);
	
	array_stream src, dest;
	if(decompress) {
		uint32_t compressed_size = src_file.read<uint32_t>(offset + 0x3);
		src_file.seek(offset);
		stream::copy_n(src, src_file, compressed_size);
		decompress_wad(dest, src);
	} else {
		stream::copy_n(src, src_file, src_file.size());
		compress_wad(dest, src, thread_count);
	}
	
	dest.seek(0);
	stream::copy_n(dest_file, dest, dest.size());

	return 0;
}
