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

#include <iomanip>
#include <iostream>
#include <algorithm>

// Enable/disable debug output for the decompression function.
#define WAD_DEBUG(cmd)
//#define WAD_DEBUG(cmd) cmd
// If this code breaks, dump the correct output and point to that here.
//#define WAD_DEBUG_EXPECTED_PATH "<file path goes here>"

bool validate_wad(char* magic) {
	return std::memcmp(magic, "WAD", 3) == 0;
}

void decompress_wad(array_stream& dest, array_stream& src) {
	decompress_wad_n(dest, src, 0);
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

	// We don't want to use stream::copy_n since it uses virtual functions.
	auto copy_bytes = [](array_stream& dest, array_stream& src, std::size_t bytes) {
		for(std::size_t i = 0; i < bytes; i++) {
			dest.write8(src.read8());
		}
	};

	auto header = src.read<wad_header>(0);
	if(!validate_wad(header.magic)) {
		throw stream_format_error("Invalid WAD header.");
	}

	uint32_t starting_byte = src.read8();
	if(starting_byte == 0) {
		starting_byte = src.read8() + 0xf;
	}
	copy_bytes(dest, src, starting_byte + 3);

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

		bool read_from_dest = false;
		bool read_from_src = false;
		std::size_t lookback_offset = -1;
		int bytes_to_copy = 0;

		if(flag_byte < 0x40) {
			if(flag_byte > 0x1f) {
				WAD_DEBUG(std::cout << " -- packet type B\n";)

				bytes_to_copy = flag_byte & 0x1f;
				if(bytes_to_copy == 0) {
					bytes_to_copy = src.read8() + 0x1f;
				}
				bytes_to_copy += 2;

				uint8_t b1 = src.read8();
				uint8_t b2 = src.read8();
				lookback_offset = dest.pos - ((b1 >> 2) + b2 * 0x40) - 1;

				read_from_dest = true;
			} else {
				WAD_DEBUG(std::cout << " -- packet type C\n";)

				if(flag_byte < 0x10) {
					WAD_DEBUG(std::cout.flush();)
					throw stream_format_error("WAD decompression failed!");
				}

				bytes_to_copy = flag_byte & 7;
				if(bytes_to_copy == 0) {
					bytes_to_copy = src.read8() + 7;
				}

				uint8_t b0 = src.read8();
				uint8_t b1 = src.read8();

				if(b0 > 0 && flag_byte == 0x11) {
					copy_bytes(dest, src, b0);
					continue;
				}

				lookback_offset = dest.pos + ((flag_byte & 8) * -0x800 - ((b0 >> 2) + b1 * 0x40));
				if(lookback_offset != dest.pos) {
					bytes_to_copy += 2;
					lookback_offset -= 0x4000;
					read_from_dest = true;
				} else if(bytes_to_copy == 1) {
					read_from_src = true;
				} else {
					WAD_DEBUG(std::cout << " -- padding detected\n";)
					while(src.pos % 0x1000 != 0x10) {
						src.pos++;
					}
					read_from_src = true;
				}
			}
		} else {
			WAD_DEBUG(std::cout << " -- packet type A\n";)

			uint8_t b1 = src.read8();
			WAD_DEBUG(std::cout << " -- pos_major = " << (int) b1 << ", pos_minor = " << (int) (flag_byte >> 2 & 7) << "\n";)
			lookback_offset = dest.pos - b1 * 8 - (flag_byte >> 2 & 7) - 1;
			bytes_to_copy = (flag_byte >> 5) + 1;
			read_from_dest = true;
		}

		WAD_DEBUG(std::cout << " -- read_from_dest = " << read_from_dest << ", read_from_src = " << read_from_src << "\n";)

		if(read_from_dest) {
			WAD_DEBUG(std::cout << " => copy 0x" << (int) bytes_to_copy << " bytes from uncompressed stream at 0x" << lookback_offset << "\n";)
			for(int i = 0; i < bytes_to_copy; i++) {
				dest.write8(dest.peek8(lookback_offset + i));
			}

			uint32_t snd_pos = src.peek8(src.pos - 2) & 3;
			if(snd_pos != 0) {
				WAD_DEBUG(std::cout << " => copy 0x" << snd_pos << " (snd_pos) bytes from compressed stream at 0x" << src.pos << " to 0x" << dest.pos << "\n";)
				copy_bytes(dest, src, snd_pos);
				continue;
			}

			read_from_src = true;
		}

		if(read_from_src&& src.pos < src.buffer.size()) {
			uint8_t decision_byte = src.peek8();
			WAD_DEBUG(std::cout << " -- decision = " << (decision_byte & 0xff) << "\n";)
			if(decision_byte > 0xf) {
				// decision_byte is the control byte.
				continue;
			}
			src.pos++;

			if(decision_byte <= 0xf) {
				uint8_t num_bytes;
				if(decision_byte != 0) {
					num_bytes = decision_byte + 3;
				} else {
					num_bytes = src.read8() + 18;
				}
				WAD_DEBUG(std::cout << " => copy 0x" << (int) num_bytes << " (decision) bytes from compressed stream at 0x" << src.pos << " to 0x" << dest.pos << ".\n";)
				copy_bytes(dest, src, num_bytes);
			}
		}
	}

	WAD_DEBUG(std::cout << "Stopped reading at " << src.pos << "\n";)
}

// Used for calculating the bounds of the sliding window.
std::size_t sub_clamped(std::size_t lhs, std::size_t rhs) {
	if(rhs > lhs) {
		return 0;
	}
	return lhs - rhs;
}

#define WAD_COMPRESS_DEBUG(cmd)
//#define WAD_COMPRESS_DEBUG(cmd) cmd
// NOTE: You must zero the size field of the expected file while debugging (0x3 through 0x6).
// WAD_COMPRESS_DEBUG_EXPECTED_PATH isn't actually very useful, as this encoder
// is NOT identical to the one used by Insomniac. It was useful when starting
// to write the algorithm, but probably won't be for debugging it.
//#define WAD_COMPRESS_DEBUG_EXPECTED_PATH "dumps/mobyseg_compressed_null"

std::vector<char> encode_wad_packet(
		stream& src,
		std::size_t dest_pos,
		std::size_t packet_no,
		std::map<std::vector<char>, std::size_t>& dict);

std::optional<std::pair<std::size_t, std::size_t>>
find_match_fast(
		stream& st,
		std::size_t target,
		std::size_t low,
		std::size_t high,
		std::map<std::vector<char>, std::size_t>& dict);

// Find the longest byte array matching target between low and high.
// Returns { offset, size } on success.
std::optional<std::pair<std::size_t, std::size_t>>
find_longest_match_in_window(
		stream& st,
		std::size_t target,
		std::size_t low,
		std::size_t high);

std::size_t num_equal_bytes(stream& st, std::size_t l, std::size_t r);

void compress_wad(stream& dest_disk, stream& src_disk) {
	WAD_COMPRESS_DEBUG(
		#ifdef WAD_COMPRESS_DEBUG_EXPECTED_PATH
			file_stream expected(WAD_COMPRESS_DEBUG_EXPECTED_PATH);
			std::optional<stream*> expected_ptr(&expected);
		#else
			std::optional<stream*> expected_ptr;
		#endif
	)

	array_stream dest, src;
	stream::copy_n(src, src_disk, src_disk.size());

	std::map<std::vector<char>, std::size_t> dictionary;

	dest.seek(0);
	src.seek(0);

	const char* header =
			"\x57\x41\x44"                          // magic
			"\x00\x00\x00\x00"                      // size (placeholder)
			"\x00\x00\x00\x00\x00\x00\x00\x00\x00"; // pad
	dest.write_n(header, 0x10);

	// Write initial section. This comes before the first packet and initialises the sliding window.
	{
		std::size_t init_size = 0;
		for(std::size_t i = 3; i < 32; i++) {
			auto match =
				find_longest_match_in_window(src, i, 0, (i > 3) ? (i - 1) : 0);
			if(!match) continue;
			if(match->second >= 3) {
				init_size = i;
			}
		}

		if(init_size >= 18) {
			dest.write<uint8_t>(0);
			dest.write<uint8_t>(init_size - 18);
		} else {
			dest.write<uint8_t>(init_size - 3);
		}
		std::vector<char> init_buf(init_size + 3);
		src.peek_n(init_buf.data(), src.tell(), init_size + 3);
		dest.write_n(init_buf.data(), init_size);
		src.seek(src.tell() + init_size);
	}

	for(int i = 0; src.tell() + 64 < src.size(); i++) {
		WAD_COMPRESS_DEBUG(
				std::cout << "{dest.tell() -> " << dest.tell() << ", src.tell() -> " << src.tell() << "}\n\n";
		)

		WAD_COMPRESS_DEBUG(
				static int count = 0;
				std::cout << "*** PACKET " << count++ << " ***\n";
		)

		if(i % 100 == 0) std::cout << "Encoded " << std::fixed << std::setprecision(4) <<  100 * (float) src.tell() / src.size() << "%\n";

		std::vector<char> packet = encode_wad_packet(src, dest.tell(), i, dictionary);
		dest.write_n(packet.data(), packet.size());
		
		// This check may fail for large packets.
		if(dest.tell() % 0x2000 > 0x1fd0) {
			// Every 0x2000 bytes or so there must be a pad packet or the
			// game crashes with a teq (Trap if Equal) exception.
			dest.write<uint8_t>(0x12);
			dest.write<uint8_t>(0x0);
			dest.write<uint8_t>(0x0);
			while(dest.tell() % 0x2000 != 0x10) {
				dest.write<uint8_t>(0xee);
			}
			
			WAD_COMPRESS_DEBUG(std::cout << "\n*** SPECIAL PAD PACKETS ***\n");
			
			// Padding must be followed by a packet with a flag of 0x11.
			std::size_t copy_size = std::min((std::size_t) 0xff, src.size() - src.tell());
			dest.write<uint8_t>(0x11);
			dest.write<uint8_t>(2);
			dest.write<uint8_t>(0);
			dest.write<uint8_t>(src.read<uint8_t>());
			dest.write<uint8_t>(src.read<uint8_t>());
			
			i += 2;
		}
	}

	// End of file packets.
	while(src.tell() < src.size()) {
		dest.write<uint8_t>(0x11);
		dest.write<uint8_t>(2); // size
		dest.write<uint8_t>(0); // unused
		dest.write<uint8_t>(src.read<uint8_t>());
		if(src.tell() < src.size()) {
			dest.write<uint8_t>(src.read<uint8_t>());
		} else {
			dest.write<uint8_t>(0);
		}
	}

	std::size_t total_size = dest.tell();
	dest.seek(3);
	dest.write<uint32_t>(total_size);
	dest.seek(0);
	stream::copy_n(dest_disk, dest, dest.size());
}

std::vector<char> encode_wad_packet(
		stream& src,
		std::size_t dest_pos,
		std::size_t packet_no,
		std::map<std::vector<char>, std::size_t>& dict) {

	std::vector<char> packet { 0 };
	uint8_t flag_byte = 0;

	std::size_t base_src = src.tell();
	std::vector<char> packet_start_buf(4);
	src.peek_n(packet_start_buf.data(), src.tell(), 4);

	std::map<std::vector<char>, std::size_t> new_dict_entries;

	std::vector<char> dict_entry_key = { packet_start_buf[0], packet_start_buf[1], packet_start_buf[2] };
	new_dict_entries[dict_entry_key] = src.tell();
	dict_entry_key.push_back(packet_start_buf[3]);
	new_dict_entries[dict_entry_key] = src.tell();

	static const std::size_t TYPE_A_MAX_LOOKBACK = 2045;

	// Encode the first part of each packet.
	{
		std::size_t high = src.tell() - 3;
		std::size_t low = sub_clamped(high, TYPE_A_MAX_LOOKBACK);
		WAD_COMPRESS_DEBUG(std::cout << "sliding window: low=" << low << ", high=" << high << "\n";)

		auto match = find_match_fast(src, src.tell(), low, high, dict);
		if(!match) {
			// Create packets of type C and of length 2 until there is a match.
			packet[0] = 0x11;
			packet.push_back(2);
			packet.push_back(0);
			packet.push_back(src.read<uint8_t>());
			packet.push_back(src.read<uint8_t>());

			for(auto [key, val] : new_dict_entries) {
				dict[key] = val;
			}
			return packet;
		}

		auto [match_offset, match_size] = *match;

		std::size_t delta = src.tell() - match_offset - 1;

		WAD_COMPRESS_DEBUG(
			//flags[match_offset] += "\033[1;32mg\033[0m";
		)

		// Max bytes_to_copy for a packet of type A is 0x8.
		if(match_size <= 0x8 && match_size >= 0x2) { // A type
			WAD_COMPRESS_DEBUG(
					std::cout << "match at 0x" << std::hex << match_offset << " of size 0x" << match_size << "\n";)

			flag_byte |= (match_size - 1) << 5;

			uint8_t pos_major = delta / 8;
			uint8_t pos_minor = delta % 8;

			WAD_COMPRESS_DEBUG(
					std::cout << "pos_major = " << (int) pos_major << ", pos_minor = " << (int) pos_minor << "\n";)

			flag_byte |= pos_minor << 2;
			packet.push_back(pos_major);
		} else if(match_size > 0x2) { // B type
			WAD_COMPRESS_DEBUG(std::cout << "B type detected!\n";)

			if(match_size > 0x120) {
				match_size = 0x120;
			}

			if(match_size > 0x21) {
				packet.push_back(match_size - 0x21);
			} else {
				flag_byte |= match_size - 2;
			}

			flag_byte |= 1 << 5; // Set packet type.

			uint8_t pos_minor = delta % 0x40;
			uint8_t pos_major = delta / 0x40;

			packet.push_back(pos_minor << 2);
			packet.push_back(pos_major);
		} else {
			throw std::runtime_error("WAD compression failed: Unhandled branch!");
		}

		WAD_COMPRESS_DEBUG(
			std::cout << " => copy 0x" << std::hex << (int) match_size
				<< " bytes from uncompressed stream at 0x" << match_offset << " (source = " << src.tell() << ")\n";
		)
		src.seek(src.tell() + match_size);
	}

	for(auto [key, val] : new_dict_entries) {
		dict[key] = val;
	}

	// If the next byte string to be encoded can be found in the
	// dictionary, we can skip the second part of the packet.
	bool skip_rest;
	{
		std::size_t high = src.tell() - 3;
		std::size_t low = sub_clamped(high, TYPE_A_MAX_LOOKBACK);
		auto match = find_match_fast(src, src.tell(), low, high, dict);
		WAD_COMPRESS_DEBUG(if(match) printf("match_offset: %x\n", match->first);)
		skip_rest = match && match->second >= 4;
	}

	WAD_COMPRESS_DEBUG(std::cout << " -- skip_rest: " << skip_rest << "\n";)

	// Encode the second part of each packet.
	if(!skip_rest) {
		std::size_t snd_pos = 0;
		for(std::size_t i = 1; i < 274; i++) {
			// Try to make the next packet start on a repeating pattern.
			std::size_t high = src.tell() + i - 3;
			std::size_t low = sub_clamped(high, TYPE_A_MAX_LOOKBACK);
			auto match = find_match_fast(src, src.tell() + i, low, high, dict);
			if(!match) continue;
			if(match->second >= 3) {
				snd_pos = i;
				break;
			}
		}

		if(snd_pos < 0x4) {
			WAD_COMPRESS_DEBUG(
				std::cout << " => copy 0x" << std::hex << (int) snd_pos
					  << " bytes (snd) from uncompressed stream at 0x" << src.tell() << " to 0x" << dest_pos
					  << "\n";
			)

			packet[packet.size() - 2] |= snd_pos;
			std::vector<char> snd_buf(snd_pos);
			src.read_n(snd_buf.data(), snd_pos);
			packet.insert(packet.end(), snd_buf.begin(), snd_buf.end());

		} else {
			WAD_COMPRESS_DEBUG(std::cout << " -- snd_pos: " << snd_pos << "\n";)

			if(snd_pos > 0x12) {
				packet.push_back(0);
				packet.push_back(snd_pos - 3 - 15);
			} else {
				packet.push_back(snd_pos - 3);
			}

			WAD_COMPRESS_DEBUG(
					std::size_t copy_pos = dest_pos + packet.size();
					std::cout << " => copy 0x" << std::hex << snd_pos << " bytes (dec) from uncompressed stream at 0x"
							  << src.tell() << " to 0x" << copy_pos
							  << "\n";
			)
			std::vector<char> dec_buf(snd_pos);
			src.read_n(dec_buf.data(), snd_pos);
			packet.insert(packet.end(), dec_buf.begin(), dec_buf.end());
		}
	}

	packet[0] |= flag_byte;

	WAD_COMPRESS_DEBUG(std::cout << "flag_byte = " << std::hex << (packet[0] & 0xff) << "\n");

	return packet;
}

std::optional<std::pair<std::size_t, std::size_t>> find_match_fast(
		stream& st,
		std::size_t target,
		std::size_t low,
		std::size_t high,
		std::map<std::vector<char>, std::size_t>& dict) {

	std::size_t offset = 0;
	std::size_t dict_match_len = 0;

	std::vector<char> pattern_buf(4);
	st.peek_n(pattern_buf.data(), target, 4);

	if(dict.find(pattern_buf) != dict.end()) {
		offset = dict[pattern_buf];
		dict_match_len = 4;
	} else {
		pattern_buf.pop_back();
		if(dict.find(pattern_buf) != dict.end()) {
			offset = dict[pattern_buf];
			dict_match_len = 3;
		}
	}

	if(dict_match_len > 0 && dict[pattern_buf] >= low && dict[pattern_buf] < high) {
		// The pattern has been found in the dictionary. Use the pre-computed offset.
		std::size_t match_size = dict_match_len;
		while(st.peek<uint8_t>(target + match_size) == st.peek<uint8_t>(offset + match_size)) {
			match_size++;
		}
		return std::make_optional<std::pair<std::size_t, std::size_t>>(offset, match_size);
	}

	// Fall back to a brute-force scan of the sliding window.
	auto match =
			find_longest_match_in_window(st, target, low, high);
	return match;
}

std::optional<std::pair<std::size_t, std::size_t>> find_longest_match_in_window(
		stream& st,
		std::size_t target,
		std::size_t low,
		std::size_t high) {
	std::optional<std::size_t> match_offset;
	std::size_t match_size = 0;
	for(std::size_t i = low; i <= high; i++) {
		std::size_t cur_bytes = num_equal_bytes(st, target, i);
		if(cur_bytes >= 3 && cur_bytes > match_size) {
			match_offset = i;
			match_size = cur_bytes;
		}
	}

	if(match_offset) {
		return std::pair<std::size_t, std::size_t>(*match_offset, match_size);
	}
	return {};
}

std::size_t num_equal_bytes(stream& st, std::size_t l, std::size_t r) {
	std::size_t result = 0;
	std::size_t st_size = st.size();
	for(std::size_t i = 0;; i++) {
		if(l + i >= st_size || r + i >= st_size) {
			break;
		}
		auto l_val = st.peek<uint8_t>(l + i);
		auto r_val = st.peek<uint8_t>(r + i);
		if(l_val != r_val) {
			break;
		}
		result++;
	}
	return result;
}
