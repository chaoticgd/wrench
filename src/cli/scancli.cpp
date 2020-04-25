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
#include <nlohmann/json.hpp>

#include "../util.h"
#include "../command_line.h"
#include "../formats/wad.h"
#include "../formats/fip.h"

# /*
#	CLI tool to scan game files for known data segments.
# */

int main(int argc, char** argv) {
	cxxopts::Options options("scan", "Scan a given file for game data segments");
	options.add_options()
		("s,src", "The input file.",
			cxxopts::value<std::string>())
		("a,alignment", "A size in bytes that each segment in the target file should be aligned to.",
			cxxopts::value<std::string>()->default_value("0x100"))
		("i,initial-offset", "Where to start scanning. For example, if -a=0x100 and -i=0x10, offsets {0x110, 0x210, ...} will be checked.",
			cxxopts::value<std::string>()->default_value("0"));

	options.parse_positional({
		"src"
	});

	auto args = parse_command_line_args(argc, argv, options);
	std::string src_path = cli_get(args, "src");
	std::size_t alignment = parse_number(cli_get_or(args, "alignment", "0x100"));
	std::size_t initial_offset = parse_number(cli_get_or(args, "initial-offset", "0"));

	file_stream src(src_path);

	std::size_t buffer_size = std::max(sizeof(wad_header), sizeof(fip_header));
	std::size_t max_offset = src.size() - buffer_size - initial_offset;
	for(std::size_t offset = initial_offset; offset < max_offset; offset += alignment) {

		proxy_stream segment(&src, offset, src.size() - offset);
		stream* segment_ptr = &segment;

		nlohmann::json outer_output;
		nlohmann::json output;
		array_stream decompressed_segment;

		// If a segment is compressed, partially decompress it and then inspect
		// the result. Otherwise just inspect the raw data.
		char magic[4];
		segment.seek(0);
		segment.read_n(magic, 4);
		if(validate_wad(magic)) {
			auto wad = segment.read<wad_header>(0);
			try {
				// Decompress WAD.
				array_stream dest_array;
				array_stream src_array;
				
				uint32_t compressed_size = src.read<uint32_t>(0x3);
				stream::copy_n(src_array, segment, compressed_size);
				
				decompress_wad_n(dest_array, src_array, buffer_size);
				
				stream::copy_n(decompressed_segment, dest_array, dest_array.size());
				segment_ptr = &decompressed_segment;
				
				outer_output["type"] = "wad";
				std::size_t total_size = wad.total_size;
				outer_output["compressed_size"] = total_size;
			} catch(stream_error& e) {
				output["error"] = e.what();
			}
		}

		segment_ptr->seek(0);
		segment_ptr->read_n(magic, 4);
		if(validate_fip(magic)) {
			auto fip = segment_ptr->read<fip_header>(0);
			output["type"] = "fip";
			int width = fip.width;
			output["width"] = width;
			int height = fip.height;
			output["height"] = height;
			output["size"] = sizeof(fip_header) + width * height;
		}
		
		if(!outer_output.empty()) {
			outer_output["offset"] = offset;
			outer_output["compressed_data"] = output;
			std::cout << outer_output.dump() << std::endl;
			continue;
		}

		if(!output.empty()) {
			output["offset"] = offset;
			std::cout << output.dump() << std::endl;
		}
	}
}
