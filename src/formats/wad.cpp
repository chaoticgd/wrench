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

#include "wad.h"

#include <cassert>
#include <iomanip>
#include <iostream>
#include <algorithm>

// Enable/disable debug output for the decompression function.
#define WAD_DEBUG(cmd) //cmd
#define WAD_DEBUG_STATS(cmd) //cmd // Print statistics about packet counts/min and max values.
// If this code breaks, dump the correct output and point to that here.
//#define WAD_DEBUG_EXPECTED_PATH "<file path goes here>"

bool validate_wad(char* magic) {
	return std::memcmp(magic, "WAD", 3) == 0;
}

void decompress_wad(array_stream& dest, array_stream& src) {
	decompress_wad_n(dest, src, 0);
}

// We don't want to use stream::copy_n since it uses virtual functions.
void copy_bytes(array_stream& dest, array_stream& src, std::size_t bytes) {
	for(std::size_t i = 0; i < bytes; i++) {
		dest.write8(src.read8());
	}
}

void decompress_wad_n(array_stream& dest, array_stream& src, std::size_t bytes_to_decompress) {

	WAD_DEBUG(
		#ifdef WAD_DEBUG_EXPECTED_PATH
			file_stream expected(WAD_DEBUG_EXPECTED_PATH);
			std::optional<file_stream*> expected_ptr(&expected);
		#else
			std::optional<file_stream*> expected_ptr;
		#endif
	)
	
	WAD_DEBUG_STATS(
		uint32_t stat_literals_count = 0;
		uint32_t stat_literals_max_size = 0;
		
		size_t big_match_min_lookback_diff = UINT32_MAX;
		size_t big_match_max_lookback_diff = 0;
		int big_match_min_bytes = INT32_MAX;
		int big_match_max_bytes = INT32_MIN;
		size_t little_match_min_lookback_diff = UINT32_MAX;
		size_t little_match_max_lookback_diff = 0;
		int little_match_min_bytes = INT32_MAX;
		int little_match_max_bytes = INT32_MIN;
	)
	
	auto header = src.read<wad_header>(0);
	if(!validate_wad(header.magic)) {
		throw stream_format_error("Invalid WAD header.");
	}

	while(
		src.pos < header.total_size &&
		(bytes_to_decompress == 0 || dest.pos < bytes_to_decompress)) {

		WAD_DEBUG(
			dest.print_diff(expected_ptr);
			std::cout << "{dest.pos -> " << dest.pos << ", src.pos -> " << src.pos << "}\n\n";
		)

		WAD_DEBUG(
			static int count = 0;
			std::cout << "*** PACKET " << count++ << " ***\n";
		)

		uint8_t flag_byte = src.read8();
		WAD_DEBUG(std::cout << "flag_byte = " << std::hex << (flag_byte & 0xff) << "\n";)

		std::size_t lookback_offset = -1;
		int bytes_to_copy = 0;

		if(flag_byte < 0x10) { // Literal packet (0x0-0xf).
			uint32_t num_bytes;
			if(flag_byte != 0) {
				num_bytes = flag_byte + 3;
			} else {
				num_bytes = src.read8() + 18;
			}
			WAD_DEBUG(std::cout << " => copy 0x" << (int) num_bytes << " (decision) bytes from compressed stream at 0x" << src.pos << " to 0x" << dest.pos << "\n";)
			copy_bytes(dest, src, num_bytes);
			
			WAD_DEBUG_STATS(
				stat_literals_count++;
				stat_literals_max_size = std::max(num_bytes, stat_literals_max_size);
			)
			
			continue;
		} else if(flag_byte < 0x20) { // (0x10-0x1f)
			WAD_DEBUG(std::cout << " -- packet type C\n";)

			bytes_to_copy = flag_byte & 7;
			if(bytes_to_copy == 0) {
				bytes_to_copy = src.read8() + 7;
			}
			
			uint8_t b0 = src.read8();
			uint8_t b1 = src.read8();
			
			lookback_offset = dest.pos + ((flag_byte & 8) * -0x800 - ((b0 >> 2) + b1 * 0x40));
			if(lookback_offset != dest.pos) {
				bytes_to_copy += 2;
				lookback_offset -= 0x4000;
			} else if(bytes_to_copy != 1) {
				WAD_DEBUG(std::cout << " -- padding detected\n";)
				while(src.pos % 0x1000 != 0x10) {
					src.pos++;
				}
				continue;
			}
		} else if(flag_byte < 0x40) { // Big match packet (0x20-0x3f).
			WAD_DEBUG(std::cout << " -- packet type B\n";)

			bytes_to_copy = flag_byte & 0x1f;
			if(bytes_to_copy == 0) {
				bytes_to_copy = src.read8() + 0x1f;
			}
			bytes_to_copy += 2;

			uint8_t b1 = src.read8();
			uint8_t b2 = src.read8();
			lookback_offset = dest.pos - ((b1 >> 2) + b2 * 0x40) - 1;
			
			WAD_DEBUG_STATS(
				size_t lookback_diff = dest.pos - lookback_offset;
				big_match_min_lookback_diff = std::min(lookback_diff, big_match_min_lookback_diff);
				big_match_max_lookback_diff = std::max(lookback_diff, big_match_max_lookback_diff);
				big_match_min_bytes = std::min(bytes_to_copy, big_match_min_bytes);
				big_match_max_bytes = std::max(bytes_to_copy, big_match_max_bytes);
			)
		} else { // Little match packet (0x40-0xff).
			WAD_DEBUG(std::cout << " -- packet type A\n";)

			uint8_t b1 = src.read8();
			WAD_DEBUG(std::cout << " -- pos_major = " << (int) b1 << ", pos_minor = " << (int) ((flag_byte >> 2) & 7) << "\n";)
			lookback_offset = dest.pos - b1 * 8 - ((flag_byte >> 2) & 7) - 1;
			bytes_to_copy = (flag_byte >> 5) + 1;
			
			WAD_DEBUG_STATS(
				size_t lookback_diff = dest.pos - lookback_offset;
				little_match_min_lookback_diff = std::min(lookback_diff, little_match_min_lookback_diff);
				little_match_max_lookback_diff = std::max(lookback_diff, little_match_max_lookback_diff);
				little_match_min_bytes = std::min(bytes_to_copy, little_match_min_bytes);
				little_match_max_bytes = std::max(bytes_to_copy, little_match_max_bytes);
			)
		}

		if(bytes_to_copy != 1) {
			WAD_DEBUG(std::cout << " => copy 0x" << (int) bytes_to_copy << " bytes from uncompressed stream at 0x" << lookback_offset << "\n";)
			for(int i = 0; i < bytes_to_copy; i++) {
				dest.write8(dest.peek8(lookback_offset + i));
			}
		}
		
		uint32_t snd_pos = src.peek8(src.pos - 2) & 3;
		if(snd_pos != 0) {
			WAD_DEBUG(std::cout << " => copy 0x" << snd_pos << " (snd_pos) bytes from compressed stream at 0x" << src.pos << " to 0x" << dest.pos << "\n";)
			copy_bytes(dest, src, snd_pos);
			continue;
		}
	}

	WAD_DEBUG(std::cout << "Stopped reading at " << src.pos << "\n";)
	
	WAD_DEBUG_STATS(
		std::cout << "\n*** stats ***\n";
		std::cout << "literals count = " << stat_literals_count << "\n";
		std::cout << "literals max size = " << stat_literals_max_size << "\n";
		
		std::cout << "big_match_min_lookback_diff = " << big_match_min_lookback_diff << "\n";
		std::cout << "big_match_max_lookback_diff = " << big_match_max_lookback_diff << "\n";
		std::cout << "big_match_min_bytes = " << big_match_min_bytes << "\n";
		std::cout << "big_match_max_bytes = " << big_match_max_bytes << "\n";
		
		
		std::cout << "little_match_min_lookback_diff = " << little_match_min_lookback_diff << "\n";
		std::cout << "little_match_max_lookback_diff = " << little_match_max_lookback_diff << "\n";
		std::cout << "little_match_min_bytes = " << little_match_min_bytes << "\n";
		std::cout << "little_match_max_bytes = " << little_match_max_bytes << "\n";
	)
}

// Used for calculating the bounds of the sliding window.
std::size_t sub_clamped(std::size_t lhs, std::size_t rhs) {
	if(rhs > lhs) {
		return 0;
	}
	return lhs - rhs;
}

#define WAD_COMPRESS_DEBUG(cmd) //cmd
// NOTE: You must zero the size field of the expected file while debugging (0x3 through 0x6).
// WAD_COMPRESS_DEBUG_EXPECTED_PATH isn't actually very useful, as this encoder
// is NOT identical to the one used by Insomniac. It was useful when starting
// to write the algorithm, but probably won't be for debugging it.
//#define WAD_COMPRESS_DEBUG_EXPECTED_PATH "dumps/mobyseg_compressed_null"

std::vector<char> encode_wad_packet(
		array_stream& dest,
		array_stream& src,
		std::size_t dest_pos,
		std::size_t packet_no,
		uint8_t& last_flag);

void compress_wad(array_stream& dest, array_stream& src) {
	WAD_COMPRESS_DEBUG(
		#ifdef WAD_COMPRESS_DEBUG_EXPECTED_PATH
			file_stream expected(WAD_COMPRESS_DEBUG_EXPECTED_PATH);
			std::optional<stream*> expected_ptr(&expected);
		#else
			std::optional<stream*> expected_ptr;
		#endif
	)

	dest.seek(0);

	const char* header =
		"\x57\x41\x44"     // magic
		"\x00\x00\x00\x00" // size (placeholder)
		"WRENCH010";       // pad
	dest.write_n(header, 0x10);
	
	uint8_t last_flag = 0;
	
	src.seek(0);
	for(size_t i = 0; src.pos + 0x100 < src.buffer.size(); i++) {
		WAD_COMPRESS_DEBUG(
			std::cout << "{dest.pos -> " << dest.pos << ", src.pos -> " << src.pos << "}\n\n";
		)

		WAD_COMPRESS_DEBUG(
			static int count = 0;
			std::cout << "*** PACKET " << count++ << " ***\n";
		)

		if(i % 100 == 0) std::cout << "Encoded " << std::fixed << std::setprecision(4) <<  100 * (float) src.pos / src.buffer.size() << "%\n";
		
		size_t old_src_pos = src.pos;
		std::vector<char> packet = encode_wad_packet(dest, src, dest.pos, i, last_flag);
		
		bool overrun = false;
		if((dest.pos % 0x2000) + packet.size() < 0x2000) {
			dest.write_n(packet.data(), packet.size());
		} else {
			// The packet has overrun a 0x2000 byte boundary, this simply cannot be.
			src.pos = old_src_pos;
			i--;
			overrun = true;
		}
		
		if(dest.pos % 0x2000 > 0x1ff0 || overrun) {
			// Every 0x2000 bytes or so there must be a pad packet or the
			// game crashes with a teq (Trap if Equal) exception.
			dest.write8(0x12);
			dest.write8(0x0);
			dest.write8(0x0);
			while(dest.pos % 0x2000 != 0x10) {
				dest.write8(0xee);
			}
			
			WAD_COMPRESS_DEBUG(std::cout << "\n*** SPECIAL PAD PACKETS ***\n");
			
			// Padding must be followed by a packet with a flag of 0x11.
			dest.write8(0x11);
			dest.write8(0);
			dest.write8(0);
			
			i += 2;
		}
	}

	// End of file packets.
	while(src.pos < src.buffer.size()) {
		dest.write8(0x11);
		dest.write8(1); // size
		dest.write8(0); // unused
		dest.write8(src.read8());
	}

	std::size_t total_size = dest.pos;
	dest.seek(3);
	dest.write<uint32_t>(total_size);
}

std::vector<char> encode_wad_packet(
		array_stream& dest,
		array_stream& src,
		std::size_t dest_pos,
		std::size_t packet_no,
		uint8_t& last_flag) {

	std::vector<char> packet { 0 };
	uint8_t flag_byte = 0;

	static const size_t TYPE_A_MAX_LOOKBACK = 2044;
	static const size_t MAX_LITERAL_SIZE = 255;//255 + 18;
	static const size_t MAX_MATCH_SIZE = 0x100;
	
	// Determine where the next repeating pattern is.
	size_t literal_size = MAX_LITERAL_SIZE;
	size_t match_offset = 0;
	size_t match_size = 0;
	const auto find_match = [&]() {
		literal_size = MAX_LITERAL_SIZE;
		match_offset = 0;
		match_size = 0;
		
		for(size_t i = 0; i < MAX_LITERAL_SIZE; i++) {
			size_t high = sub_clamped(src.pos + i, 4);
			size_t low = sub_clamped(high, TYPE_A_MAX_LOOKBACK);
			for(size_t j = low; j <= high; j++) {
				// Count number of equal bytes.
				size_t st_size = src.buffer.size();
				size_t l = src.pos + i;
				size_t r = j;
				size_t k = 0;
				for(; k < MAX_MATCH_SIZE; k++) {
					if(l + k >= st_size || r + k >= st_size) {
						break;
					}
					auto l_val = src.peek8(l + k);
					auto r_val = src.peek8(r + k);
					if(l_val != r_val) {
						break;
					}
				}
				
				if(k >= 3 && k > match_size) {
					match_offset = j;
					match_size = k;
				}
			}
			if(match_size >= 3) {
				literal_size = i;
				break;
			}
		}
	};
	find_match();
	
	if(packet_no > 0 && literal_size == 0) { // Match packet.
		std::size_t delta = src.pos - match_offset - 1;

		// Max bytes_to_copy for a packet of type A is 0x8.
		if(match_size <= 0x8) { // A type
			assert(match_size >= 3);
		
			WAD_COMPRESS_DEBUG(
				std::cout << "match at 0x" << std::hex << match_offset << " of size 0x" << match_size << "\n";
			)

			flag_byte |= (match_size - 1) << 5;

			uint8_t pos_major = delta / 8;
			uint8_t pos_minor = delta % 8;

			WAD_COMPRESS_DEBUG(
				std::cout << "pos_major = " << (int) pos_major << ", pos_minor = " << (int) pos_minor << "\n";
			)

			flag_byte |= pos_minor << 2;
			packet.push_back(pos_major);
		} else { // B type
			WAD_COMPRESS_DEBUG(std::cout << "B type detected!\n";)
			
			if(match_size > (0b11111 + 2)) {
				packet.push_back(match_size - (0b11111 + 2));
			} else {
				flag_byte |= match_size - 2;
			}

			flag_byte |= 1 << 5; // Set packet type.

			uint8_t pos_minor = delta % 0x40;
			uint8_t pos_major = delta / 0x40;

			packet.push_back(pos_minor << 2);
			packet.push_back(pos_major);
		}

		WAD_COMPRESS_DEBUG(
			std::cout << " => copy 0x" << std::hex << (int) match_size
				<< " bytes from uncompressed stream at 0x" << match_offset << " (source = " << src.pos << ")\n";
		)
		src.pos += match_size;
	} else { // Literal packet.
		if(literal_size <= 3) {
			// If the current literal is small and the last packet was a match
			// packet, we can stuff the literal size in the last packet.
			if(last_flag >= 0x10) {
				WAD_COMPRESS_DEBUG(
					if(literal_size > 0) {
						std::cout << " => copy 0x" << std::hex << (int) literal_size
							<< " (snd_pos) bytes from compressed stream at 0x" << dest_pos + packet.size() << " (target = " << src.pos << ")\n";
					}
				)
				dest.buffer[dest.pos - 2] |= literal_size;
				packet.insert(packet.end(), src.buffer.begin() + src.pos, src.buffer.begin() + src.pos + literal_size);
				src.pos += literal_size;
				packet.erase(packet.begin()); // We don't need no flag byte!
				return packet;
			} else {
				// Bad path.
				literal_size = 4;
			}
		}
		if(literal_size <= 18) {
			// We can encode the size in the flag byte.
			flag_byte |= literal_size - 3;
		} else {
			// We have to push it as a seperate byte (leave the flag as zero).
			packet.push_back(literal_size - 18);
		}
		
		WAD_COMPRESS_DEBUG(
			std::cout << " => copy 0x" << std::hex << (int) literal_size
				<< " bytes from compressed stream at 0x" << dest_pos + packet.size() << " (target = " << src.pos << ")\n";
		)
		packet.insert(packet.end(), src.buffer.begin() + src.pos, src.buffer.begin() + src.pos + literal_size);
		src.pos += literal_size;
	}
	
	packet[0] |= flag_byte;
	last_flag = flag_byte;

	WAD_COMPRESS_DEBUG(std::cout << "flag_byte = " << std::hex << (packet[0] & 0xff) << "\n");

	return packet;
}
