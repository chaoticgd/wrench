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

# /*
#	Scan a game data segment for a given indexed BMP file, even if said file has
#	a different palette.
# */

#include <iostream>

#include "../command_line.h"
#include "../formats/bmp.h"
#include "../formats/fip.h"

std::vector<std::size_t> hash_pixel_data(char* texture, std::size_t num_bytes);

int main(int argc, char** argv) {
	cxxopts::Options options("texturefinder", "Scan a game data segment for a given indexed BMP file, even if said file has a different palette. For example, you could dump a texture using PCSX2, convert it to an indexed BMP (with 256 colours) using the GNU Image Manipulation Program, and then feed it into this program to find where it is stored on disc, using the command \"./bin/texturefinder game.iso texture.bmp\"");
	options.add_options()
		("i,iso", "The data segment to scan.",
			cxxopts::value<std::string>())
		("t,target", "The texture to scan for.",
			cxxopts::value<std::string>());

	options.parse_positional({
		"iso", "target"
	});

	auto args = parse_command_line_args(argc, argv, options);
	std::string iso_path = args["iso"].as<std::string>();
	std::string target_path = args["target"].as<std::string>();
	
	file_stream iso(iso_path);
	file_stream target(target_path);
	
	// Hash the target texture.
	auto bmp_header = target.read<bmp_file_header>(0);
	auto info_header = target.read<bmp_info_header>();
	if(!validate_bmp(bmp_header)) {
		std::cerr << "Error: Input texture must be a valid indexed BMP file.\n";
		return 1;
	}
	uint32_t row_size = ((info_header.bits_per_pixel * info_header.width + 31) / 32) * 4;
	std::vector<char> pixels(info_header.width * info_header.height);
	target.seek(bmp_header.pixel_data.value);	
	for(int y = info_header.height - 1; y >= 0; y--) {
		target.read_n(pixels.data() + y * row_size, row_size);
	}
	std::vector<std::size_t> target_hash = hash_pixel_data(pixels.data(), 256);
	
	// Iterate over all uncompressed 2FIP textures.
	for(std::size_t i = 0; i < iso.size(); i += 0x800) {
		char magic[0x14];
		iso.seek(i);
		iso.read_n(magic, 0x14);
		
		std::optional<std::size_t> fip_offset;
		if(validate_fip(magic)) {
			fip_offset = 0;
		} else if(validate_fip(magic + 0x10)) {
			fip_offset = 0x10;
		}
		
		if(fip_offset) {
			// The sector contains a 2FIP texture.
			std::size_t test_offset = i + *fip_offset;
			char test_pixels[256];
			iso.seek(test_offset + sizeof(fip_header));
			iso.read_n(test_pixels, 256);
			
			// We cannot just compare each byte, since the palette indices
			// may be different.
			std::vector<std::size_t> test_hash = hash_pixel_data(test_pixels, 256);
			if(test_hash == target_hash) {
				std::cout << "Possible matching texture found at 0x" << std::hex << test_offset << "\n";
			}
		}
	}
}

std::vector<std::size_t> hash_pixel_data(char* texture, std::size_t num_bytes) {
	std::vector<std::size_t> offsets;
	for(std::size_t i = 1; i < num_bytes; i++) {
		if(texture[i] != texture[i - 1]) {
			offsets.push_back(i);
		}
	}
	return offsets;
}
