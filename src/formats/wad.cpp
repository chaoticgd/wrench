/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019 chaoticgd

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

#include "wad.h"

#include <iostream>

// Enable/disable debug output for the decompression function.
#define WAD_DEBUG(cmd)
//#define WAD_DEBUG(cmd) cmd
// If this code breaks, dump the correct output and point to that here.
//#define WAD_DEBUG_EXPECTED_PATH "<file path goes here>"

bool validate_wad(char* magic) {
	return std::memcmp(magic, "WAD", 3) == 0;
}

void decompress_wad(stream& dest, stream& src) {
	decompress_wad_n(dest, src, 0);
}

void decompress_wad_n(stream& dest, stream& src, uint32_t bytes_to_decompress) {

	// The source file is split up into sections. The first byte of each of
	// these sections determines the code path taken when reading said section
	// and will henceforth be referred to as the control byte.

	WAD_DEBUG(
		#ifdef WAD_DEBUG_EXPECTED_PATH
			file_stream expected(WAD_DEBUG_EXPECTED_PATH);
			std::optional<file_stream*> expected_ptr(&expected);
		#else
			std::optional<file_stream*> expected_ptr;
		#endif
	)

	auto header = src.read<wad_header>(0);
	if(!validate_wad(header.magic)) {
		throw stream_format_error("Invalid WAD header.");
	}

	uint32_t starting_byte = src.read<uint8_t>();
	if(starting_byte == 0) {
		starting_byte = src.read<uint8_t>() + 0xf;
	}
	stream::copy_n(dest, src, starting_byte + 3);

	while(
		src.tell() < header.total_size &&
		(bytes_to_decompress == 0 || src.tell() < bytes_to_decompress)) {

		WAD_DEBUG(
			dest.print_diff(expected_ptr);
			std::cout << "{dest.tell() -> " << dest.tell() << ", src.tell() -> " << src.tell() << "}\n\n";
		)

		uint8_t control_byte = src.read<uint8_t>();
		WAD_DEBUG(std::cout << "control_byte = " << std::hex << (control_byte & 0xff) << "\n";)

		bool read_from_dest = false;
		bool read_from_src = false;
		int lookback_offset = -1;
		int bytes_to_copy = 0;

		if(control_byte < 0x40) {
			if(control_byte > 0x1f) {
				WAD_DEBUG(std::cout << " -- branch: 0\n";)

				bytes_to_copy = control_byte & 0x1f;
				if(bytes_to_copy == 0) {
					bytes_to_copy = src.read<uint8_t>() + 0x1f;
				}
				bytes_to_copy += 2;

				int b1 = src.read<uint8_t>();
				int b2 = src.read<uint8_t>();
				lookback_offset = dest.tell() - ((b1 >> 2) + b2 * 0x40) - 1;
				
				read_from_dest = true;
			} else {
				WAD_DEBUG(std::cout << " -- branch: 1\n";)

				if(control_byte < 0x10) {
					throw stream_format_error("WAD decompression failed!");
				}

				bytes_to_copy = control_byte & 7;
				if(bytes_to_copy == 0) {
					bytes_to_copy = src.read<uint8_t>() + 7;
				}

				uint8_t b0 = src.read<uint8_t>();
				uint8_t b1 = src.read<uint8_t>();

				if(b0 > 0 && control_byte == 0x11) {
					stream::copy_n(dest, src, b0);
					continue;
				}

				lookback_offset = dest.tell() + ((control_byte & 8) * -0x800 - ((b0 >> 2) + b1 * 0x40));
				if(lookback_offset != dest.tell()) {
					bytes_to_copy += 2;
					lookback_offset -= 0x4000;
					read_from_dest = true;
				} else if(bytes_to_copy == 1) {
					read_from_src = true;
				} else {
					WAD_DEBUG(std::cout << " -- padding detected\n";)
					while(src.tell() % 0x1000 != 0x10) {
						src.read<uint8_t>();
					}
					read_from_src = true;
				}
			}
		} else {
			WAD_DEBUG(std::cout << " -- branch: 2\n";)

			uint8_t b1 = src.read<uint8_t>();
			lookback_offset = dest.tell() - b1 * 8 - (control_byte >> 2 & 7) - 1;
			bytes_to_copy = (control_byte >> 5) + 1;
			read_from_dest = true;
		}

		WAD_DEBUG(std::cout << " -- read_from_dest = " << read_from_dest << ", read_from_src = " << read_from_src << "\n";)

		if(read_from_dest) {
			WAD_DEBUG(std::cout << " => copy 0x" << (int) bytes_to_copy << " bytes from destination stream at 0x" << lookback_offset << "\n";)
			for(int i = 0; i < bytes_to_copy; i++) {
				dest.write<uint8_t>(dest.peek<uint8_t>(lookback_offset + i));
			}

			uint32_t offset = src.peek<uint8_t>(src.tell() - 2) & 3;
			if(offset != 0) {
				WAD_DEBUG(std::cout << " => copy fixed 3 bytes from source (offset = 0x" << offset << ")\n";)
				stream::copy_n(dest, src, 3);
				dest.seek(dest.tell() + offset - 3);
				src.seek(src.tell() + offset - 3);
				continue;
			}

			read_from_src = true;
		}

		if(read_from_src) {
			uint8_t decision_byte = src.peek<uint8_t>();
			if(decision_byte > 0xf) {
				// decision_byte is the control byte.
				continue;
			}
			src.read<uint8_t>(); // Advance the position indicator.

			if(decision_byte <= 0xf) {
				uint8_t num_bytes;
				if(decision_byte != 0) {
					num_bytes = decision_byte + 3;
				} else {
					num_bytes = src.read<uint8_t>() + 18;
				}
				WAD_DEBUG(std::cout << " => copy 0x" << (int) num_bytes << " bytes from source at 0x" << src.tell() << ".\n";)
				stream::copy_n(dest, src, num_bytes);
			}
		}
	}

	WAD_DEBUG(std::cout << "Stopped reading at " << src.tell() << "\n";)
}
