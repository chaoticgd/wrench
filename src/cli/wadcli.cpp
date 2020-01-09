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

#include <sstream>

#include "../command_line.h"
#include "../formats/wad.h"

# /*
#	CLI tool to decompress and recompress WAD segments. Not to be confused with WAD archives.
# */

void copy_and_decompress(stream& dest, stream& src);
void copy_and_compress(stream& dest, stream& src);

int main(int argc, char** argv) {
	return run_cli_converter(argc, argv,
		"Decompress WAD segments",
		{
			{ "decompress", copy_and_decompress },
			{ "compress", copy_and_compress }
		});
}

void copy_and_decompress(stream& dest, stream& src) {
	array_stream dest_array;
	array_stream src_array;
	
	uint32_t compressed_size = src.read<uint32_t>(0x3);
	src.seek(0);
	stream::copy_n(src_array, src, compressed_size);
	
	decompress_wad(dest_array, src_array);
	
	dest.seek(0);
	dest_array.seek(0);
	stream::copy_n(dest, dest_array, dest_array.size());
}

void copy_and_compress(stream& dest, stream& src) {
	array_stream dest_array;
	array_stream src_array;
	
	stream::copy_n(src_array, src, src.size());
	
	compress_wad(dest_array, src_array);
	
	dest.seek(0);
	dest_array.seek(0);
	stream::copy_n(dest, dest_array, dest_array.size());
}
