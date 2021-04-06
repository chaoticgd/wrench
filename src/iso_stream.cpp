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

#include "iso_stream.h"

#include "app.h"
#include "md5.h"
#include "util.h"
#include "fs_includes.h"
#include "formats/wad.h"

simple_wad_stream::simple_wad_stream(stream* backing, size_t offset)
	: array_stream(backing) {
	array_stream compressed;
	uint32_t compressed_size = backing->peek<uint32_t>(offset + 0x3);
	backing->seek(offset);
	stream::copy_n(compressed, *backing, compressed_size);
	decompress_wad(*this, compressed);
}

std::string md5_from_stream(stream& st) {
	MD5_CTX ctx;
	MD5Init(&ctx);
	
	static const std::size_t BLOCK_SIZE = 1024 * 4;
	std::size_t file_size = st.size();
	
	st.seek(0);
	
	std::vector<uint8_t> block(BLOCK_SIZE);
	for(std::size_t i = 0; i < file_size / BLOCK_SIZE; i++) {
		st.read_n((char*) block.data(), BLOCK_SIZE);
		MD5Update(&ctx, block.data(), BLOCK_SIZE);
	}
	st.read_n((char*) block.data(), file_size % BLOCK_SIZE);
	MD5Update(&ctx, block.data(), file_size % BLOCK_SIZE);

	uint8_t digest[MD5_DIGEST_LENGTH];
	MD5Final(digest, &ctx);
	
	return md5_to_printable_string(digest);
}
