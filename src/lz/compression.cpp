/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2021 chaoticgd

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

#include "compression.h"

#include <thread>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <algorithm>

// Enable/disable debug output for the decompression function.
#define WAD_DEBUG(cmd) //cmd
// If this code breaks, dump the correct output and point to that here.
//#define WAD_DEBUG_EXPECTED_PATH "<file path goes here>"

bool validate_wad(char* magic) {
	return std::memcmp(magic, "WAD", 3) == 0;
}
// We don't want to use stream::copy_n since it uses virtual functions.
void copy_bytes(array_stream& dest, array_stream& src, std::size_t bytes) {
	for(std::size_t i = 0; i < bytes; i++) {
		dest.write8(src.read8());
	}
}

void decompress_wad(array_stream& dest, array_stream& src) {

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

	while(src.pos < header.total_size) {
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

		if(flag_byte < 0x10) { // Medium/big iteral packet (0x0-0xf).
			uint32_t num_bytes;
			if(flag_byte != 0) {
				num_bytes = flag_byte + 3;
			} else { // Big literal.
				num_bytes = src.read8() + 18;
			}
			copy_bytes(dest, src, num_bytes);
			
			if(src.pos < src.buffer.size() && src.peek8() < 0x10) {
				// The game disallows this so lets complain.
				throw stream_format_error("WAD decompression failed: Two literals in a row? Implausible!");
			}
			
			continue;
		} else if(flag_byte < 0x20) { // Far match packets + special cases (0x10-0x1f)
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
				while(src.pos % 0x1000 != 0x10) {
					src.pos++;
				}
				continue;
			}
		} else if(flag_byte < 0x40) { // Medium/big match packet (0x20-0x3f).
			bytes_to_copy = flag_byte & 0x1f;
			if(bytes_to_copy == 0) {
				bytes_to_copy = src.read8() + 0x1f;
			}
			bytes_to_copy += 2;

			uint8_t b1 = src.read8();
			uint8_t b2 = src.read8();
			lookback_offset = dest.pos - ((b1 >> 2) + b2 * 0x40) - 1;
		} else { // Little match packet (0x40-0xff).
			uint8_t b1 = src.read8();
			lookback_offset = dest.pos - b1 * 8 - ((flag_byte >> 2) & 7) - 1;
			bytes_to_copy = (flag_byte >> 5) + 1;
		}

		if(bytes_to_copy != 1) {
			for(int i = 0; i < bytes_to_copy; i++) {
				dest.write8(dest.peek8(lookback_offset + i));
			}
		}
		
		uint32_t little_literal_size = src.peek8(src.pos - 2) & 3;
		copy_bytes(dest, src, little_literal_size);
	}
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

void compress_wad_intermediate(
		std::vector<char>* intermediate,
		const uint8_t* src,
		size_t src_pos,
		size_t src_end);

struct match_result {
	size_t literal_size; // The number of bytes before a match was found.
	size_t match_offset;
	size_t match_size;
};

template <bool end_of_buffer>
match_result find_match(
	const uint8_t* src,
	size_t src_pos,
	size_t src_end,
	std::vector<int32_t>& ht,
	std::vector<int32_t>& chain);

void encode_match_packet(
		array_stream& dest,
		const uint8_t* src,
		size_t& src_pos,
		size_t src_end,
		uint32_t& last_flag,
		size_t match_offset,
		size_t match_size);

void encode_literal_packet(
		array_stream& dest,
		const uint8_t* src,
		size_t& src_pos,
		size_t src_end,
		uint32_t& last_flag,
		size_t literal_size);

size_t get_wad_packet_size(uint8_t* src, size_t bytes_left);

const int DO_NOT_INJECT_FLAG = 0x100;

const size_t MAX_MATCH_SIZE = 264;
const size_t MAX_LITERAL_SIZE = 273; // 0b11111111 + 18

const size_t MIN_LITTLE_MATCH_SIZE = 3;
const size_t MAX_LITTLE_MATCH_SIZE = 8; // 0b111 + 1
const size_t MIN_MEDIUM_MATCH_SIZE = 3;
const size_t MAX_MEDIUM_MATCH_SIZE = 33; // 0b11111 + 2
const size_t MIN_BIG_MATCH_SIZE = 33;
const size_t MAX_BIG_MATCH_SIZE = 288; // 0b11111111 + 33
const size_t MIN_MEDIUM_FAR_MATCH_SIZE = 3;
const size_t MAX_MEDIUM_FAR_MATCH_SIZE = 9; // 0b111 + 2
const size_t MIN_BIG_FAR_MATCH_SIZE = 9;
const size_t MAX_BIG_FAR_MATCH_SIZE = 264; // 0b11111111 + 9

const size_t MIN_LITTLE_MATCH_LOOKBACK = 1;
const size_t MAX_LITTLE_MATCH_LOOKBACK = 2048; // 0b11111111 * 8 + 0b111 + 1
const size_t MIN_MEDIUM_MATCH_LOOKBACK = 1;
const size_t MAX_MEDIUM_MATCH_LOOKBACK = 16384; // 0b111111 + 0b11111111 * 0x40 + 1
const size_t MIN_BIG_MATCH_LOOKBACK = 1;
const size_t MAX_BIG_MATCH_LOOKBACK = 16384; // 0b111111 + 0b11111111 * 0x40 + 1
const size_t MIN_FAR_MATCH_LOOKBACK = 16385; // 16384 is reserved as a special case.
const size_t MAX_FAR_MATCH_LOOKBACK = 34752; // 0x4000 + 0x800 + 0b11111111 * 0x40
const size_t MAX_FAR_MATCH_LOOKBACK_WITH_A_EQ_ZERO = 32704; // 0x4000 + 0b11111111 * 0x40

const int32_t WINDOW_SIZE = 32768;
const int32_t WINDOW_MASK = WINDOW_SIZE - 1;

const std::vector<char> EMPTY_LITTLE_LITERAL = { 0x11, 0, 0 };

void compress_wad(array_stream& dest, array_stream& src, int thread_count) {
	WAD_COMPRESS_DEBUG(
		#ifdef WAD_COMPRESS_DEBUG_EXPECTED_PATH
			file_stream expected(WAD_COMPRESS_DEBUG_EXPECTED_PATH);
			std::optional<stream*> expected_ptr(&expected);
		#else
			std::optional<stream*> expected_ptr;
		#endif
	)
	
	// Compress the data into a stream of packets.
	std::vector<array_stream> intermediates(thread_count);
	if(thread_count == 1) {
		compress_wad_intermediate(&intermediates[0].buffer, (uint8_t*) src.buffer.data(), 0, src.buffer.size());
	} else {
		size_t min_block_size = 0x100 * thread_count;
		size_t total_size = src.buffer.size();
		total_size += min_block_size - (total_size % min_block_size);
		size_t block_size = total_size / thread_count;
		
		std::vector<std::thread> threads(thread_count);
		for(int i = 0; i < thread_count; i++) {
			const uint8_t* src_ptr = (uint8_t*) src.buffer.data();
			size_t src_pos = block_size * i;
			size_t src_end = std::min(src.buffer.size(), (block_size * (i + 1)));
			threads[i] = std::thread(compress_wad_intermediate, &intermediates[i].buffer, src_ptr, src_pos, src_end);
		}
		for(int i = 0; i < thread_count; i++) {
			threads[i].join();
		}
	}
	
	size_t header_pos = dest.tell();
	const char* header = "\x57\x41\x44\x00\x00\x00\x00\x57\x52\x45\x4e\x43\x48\x30\x31\x30";
	dest.write_n(header, 0x10);
	
	// Append the compressed data and insert padding where required.
	for(int i = 0; i < thread_count; i++) {
		array_stream& intermediate = intermediates[i];
		intermediate.pos = 0;
		while(intermediate.pos < intermediate.size()) {
			size_t packet_size = get_wad_packet_size(
				(uint8_t*) intermediate.buffer.data() + intermediate.pos,
				intermediate.buffer.size() - intermediate.pos);
			// The different blocks each thread generates may begin/end with
			// literal packets. Two consecutive literal packets aren't allowed,
			// so we add a dummy packet in between. We need to do this while
			// respecting the 0x2000 buffer size (see comment below), so we do
			// it here.
			bool insert_dummy = i != 0 && intermediate.pos == 0;
			size_t insert_size = packet_size;
			if(insert_dummy) {
				insert_size += 3;
			}
			// dest.pos is offset 0x10 bytes by the header:
			//  0x0000 WAD. .... .... ....
			//	0x0010 [data]
			//   ...
			//	0x2000 [data]
			//	0x2010 [start of new block]
			if((((dest.pos - header_pos) + 0x1ff0) % 0x2000) + insert_size > 0x2000 - 3) {
				// Every 0x2000 bytes or so there must be a pad packet or the
				// game crashes with a teq exception. This is because the game
				// copies the compressed data into the EE core's scratchpad, which
				// is 0x4000 bytes in size.
				dest.write8(0x12);
				dest.write8(0x0);
				dest.write8(0x0);
				while((dest.pos - header_pos) % 0x2000 != 0x10) {
					dest.write8(0xee);
				}
			}
			if(insert_dummy) {
				dest.write8(0x11);
				dest.write8(0);
				dest.write8(0);
			}
			dest.buffer.insert(dest.buffer.end(),
				intermediate.buffer.begin() + intermediate.pos,
				intermediate.buffer.begin() + intermediate.pos + packet_size);
			dest.pos += packet_size;
			intermediate.pos += packet_size;
		}
	}
	
	size_t end_pos = dest.tell();
	uint32_t total_size = end_pos - header_pos;
	dest.write<uint32_t>(header_pos + 0x3, total_size);
	dest.seek(end_pos);
}

void compress_wad_intermediate(
		std::vector<char>* intermediate,
		const uint8_t* src,
		size_t src_pos,
		size_t src_end) {
	uint32_t last_flag = DO_NOT_INJECT_FLAG;
	array_stream thread_dest;
	std::vector<int32_t> ht(WINDOW_SIZE, -WINDOW_SIZE);
	std::vector<int32_t> chain(WINDOW_SIZE, -WINDOW_SIZE);
	while(src_pos < src_end) {
		match_result match;
		if(src_pos + MAX_MATCH_SIZE >= src_end) {
			match = find_match<true>(src, src_pos, src_end, ht, chain);
		} else {
			match = find_match<false>(src, src_pos, src_end, ht, chain);
		}
		
		if(match.literal_size > 0) {
			encode_literal_packet(thread_dest, src, src_pos, src_end, last_flag, match.literal_size);
			thread_dest.pos = thread_dest.buffer.size();
		}
		if(match.match_size > 0) {
			encode_match_packet(thread_dest, src, src_pos, src_end, last_flag, match.match_offset, match.match_size);
			thread_dest.pos = thread_dest.buffer.size();
		}
	}
	*intermediate = std::move(thread_dest.buffer);
}

int32_t hash32(int32_t n) {
	return ((n * 12) + n) >> 3;
}

template <bool end_of_buffer>
match_result find_match(
	const uint8_t* src,
	size_t src_pos,
	size_t src_end,
	std::vector<int32_t>& ht,
	std::vector<int32_t>& chain) {
	size_t max_literal_size = end_of_buffer ?
		std::min(MAX_LITERAL_SIZE, src_end - src_pos) : MAX_LITERAL_SIZE;
	
	match_result match;
	match.literal_size = max_literal_size;
	match.match_offset = 0;
	match.match_size = 0;
	
	// Matching algorithm taken from: https://glinscott.github.io/lz/
	for(size_t i = 0; i < max_literal_size; i++) {
		int64_t target = src_pos + i;
		size_t max_match_size = end_of_buffer ?
			std::min(MAX_MATCH_SIZE, src_end - src_pos - i) : MAX_MATCH_SIZE;
		
		int32_t key = hash32(src[target] | (src[target + 1] << 8) | (src[target + 2] << 16));
		key &= WINDOW_MASK;
		int64_t next = ht[key];
		
		int64_t low = target - MAX_FAR_MATCH_LOOKBACK_WITH_A_EQ_ZERO;
		int hits = 0;
		while(next > low && ++hits < 16) {
			// This makes matching much faster.
			if(!end_of_buffer && *(uint16_t*) &src[next] != *(uint16_t*) &src[target]) {
				continue;
			}
			
			// Count number of equal bytes.
			size_t k = end_of_buffer ? 0 : 2;
			for(; k < max_match_size; k++) {
				auto l_val = src[target + k];
				auto r_val = src[next + k];
				if(l_val != r_val) {
					break;
				}
			}
			
			if(k > match.match_size) {
				match.match_size = k;
				match.match_offset = next;
			}
			next = chain[next & WINDOW_MASK];
		}

		chain[target & WINDOW_MASK] = ht[key];
		ht[key] = target;
		
		if(match.match_size >= 3) {
			match.literal_size = i;
			break;
		}
	}
	
	if(match.match_size < 3) {
		match.match_offset = 0;
		match.match_size = 0;
	}
	
	return match;
}

void encode_match_packet(
		array_stream& dest,
		const uint8_t* src,
		size_t& src_pos,
		size_t src_end,
		uint32_t& last_flag,
		size_t match_offset,
		size_t match_size) {
	size_t lookback = src_pos - match_offset;
	
	if(match_size <= MAX_LITTLE_MATCH_SIZE && lookback <= MAX_LITTLE_MATCH_LOOKBACK) { // A type
		assert(match_size >= 3);
		
		uint8_t a = (lookback - 1) % 8;
		uint8_t b = (lookback - 1) / 8;
		
		dest.buffer.push_back(((match_size - 1) << 5) | (a << 2)); // flag
		dest.buffer.push_back(b);
	} else if(lookback <= MAX_BIG_MATCH_LOOKBACK) {
		if(match_size > MAX_MEDIUM_MATCH_SIZE) { // Big match
			dest.buffer.push_back(0b00100000); // flag
			dest.buffer.push_back(match_size - MAX_MEDIUM_MATCH_SIZE);
		} else { // Medium match
			dest.buffer.push_back(0b00100000 | (match_size - 2)); // flag
		}
		
		uint8_t a = (lookback - 1) % 0x40;
		uint8_t b = (lookback - 1) / 0x40;
		
		dest.buffer.push_back(a << 2);
		dest.buffer.push_back(b);
	} else { // Far matches.
		assert(lookback <= MAX_FAR_MATCH_LOOKBACK);
		
		uint8_t a = lookback > MAX_FAR_MATCH_LOOKBACK_WITH_A_EQ_ZERO;
		uint32_t diff = a ? 0x4800 : 0x4000;
		uint8_t b = (lookback - diff) % 0x40;
		uint8_t c = (lookback - diff) / 0x40;
		
		if(match_size > MAX_MEDIUM_FAR_MATCH_SIZE) { // Big far match.
			assert(match_size <= MAX_BIG_FAR_MATCH_SIZE);
			dest.buffer.push_back(0b00010000 | (a << 3)); // flag
			dest.buffer.push_back(match_size - MAX_MEDIUM_FAR_MATCH_SIZE);
		} else {
			dest.buffer.push_back(0b00010000 | (a << 3) | (match_size - 2)); // flag
		}
		
		dest.buffer.push_back(b << 2);
		dest.buffer.push_back(c);
	}
	
	src_pos += match_size;
	last_flag = dest.buffer[dest.pos];
}

void encode_literal_packet(
		array_stream& dest,
		const uint8_t* src,
		size_t& src_pos,
		size_t src_end,
		uint32_t& last_flag,
		size_t literal_size) {
	if(last_flag < 0x10) { // Two literals in a row? Implausible!
		last_flag = 0x11;
		dest.buffer.insert(dest.buffer.end(), EMPTY_LITTLE_LITERAL.begin(), EMPTY_LITTLE_LITERAL.end());
		dest.pos = dest.buffer.size();
	}
	
	if(literal_size <= 3) {
		// If the last flag is a literal, or there's already a little literal
		// injected into the last packet, we need to push a new dummy packet
		// that we can stuff a literal into on the next iteration.
		if(last_flag == DO_NOT_INJECT_FLAG) {
			last_flag = 0x11;
			dest.buffer.insert(dest.buffer.end(), EMPTY_LITTLE_LITERAL.begin(), EMPTY_LITTLE_LITERAL.end());
			dest.pos = dest.buffer.size();
		}
		
		WAD_COMPRESS_DEBUG(
			std::cout << " => copy 0x" << std::hex << (int) literal_size
				<< " (snd_pos) bytes from compressed stream at 0x" << dest_pos + packet.size() << " (target = " << src_pos << ")\n";
		)
		dest.buffer[dest.pos - 2] |= literal_size;
		dest.buffer.insert(dest.buffer.end(), src + src_pos, src + src_pos + literal_size);
		src_pos += literal_size;
		last_flag = DO_NOT_INJECT_FLAG;
		return;
	} else if(literal_size <= 18) {
		// We can encode the size in the flag byte.
		dest.buffer.push_back(literal_size - 3); // flag
	} else {
		// We have to push it as a seperate byte.
		dest.buffer.push_back(0); // flag
		dest.buffer.push_back(literal_size - 18);
	}
	
	WAD_COMPRESS_DEBUG(
		std::cout << " => copy 0x" << std::hex << (int) literal_size
			<< " bytes from compressed stream at 0x" << dest_pos + packet.size() << " (target = " << src_pos << ")\n";
	)
	dest.buffer.insert(dest.buffer.end(), src + src_pos, src + src_pos + literal_size);
	src_pos += literal_size;
	
	last_flag = dest.buffer[dest.pos];
}

size_t get_wad_packet_size(uint8_t* src, size_t bytes_left) {
	size_t size_of_packet = 1; // flag
	uint8_t flag_byte = src[0];
	if(flag_byte < 0x10) { // Literal packet (0x0-0xf).
		if(flag_byte != 0) {
			size_of_packet += flag_byte + 3; // mediumlit
		} else {
			size_of_packet += 1 + (src[1] + 18); // size + biglit
		}
		if(size_of_packet < bytes_left && src[size_of_packet] < 0x10) {
			throw std::runtime_error("Compression failed: Intermediate buffer corrupted (double literal)!\n");
		}
		return size_of_packet; // We can't put a little literal inside another literal.
	} else if(flag_byte < 0x20) { // Far matches (0x10-0x1f)
		uint8_t bytes_to_copy = flag_byte & 7;
		if(bytes_to_copy == 0) {
			size_of_packet++; // bytes_to_copy
		}
		size_of_packet += 1 + 1; // (b + n) + c
	} else if(flag_byte < 0x40) { // Big/medium match packet (0x20-0x3f).
		uint8_t bytes_to_copy = flag_byte & 0x1f;
		if(bytes_to_copy == 0) { // Big match packet.
			size_of_packet++;
		}
		size_of_packet += 1 + 1; // a + b
	} else { // Little match packet (0x40-0xff).
		size_of_packet++; // pos_major
	}
	size_of_packet += src[size_of_packet - 2] & 3; // Add on the little literal.
	return size_of_packet;
}
