/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2022 chaoticgd

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

#include <vector>
#include <stdio.h>
#undef NDEBUG
#include <assert.h>
#include <stdint.h>

#include <platform/fileio.h>

int main(int argc, char** argv)
{
	size_t retval;

	if (argc <= 3) {
		fprintf(stderr, "Invalid number of arguments.");
		return 1;
	}

	WrenchFileHandle* dest = file_open(argv[1], WRENCH_FILE_MODE_WRITE);
	assert(dest);

	file_write_string("extern \"C\"{alignas(16) unsigned char wadinfo[]={", dest);
	for (int i = 2; i < argc; i++) {
		WrenchFileHandle* src = file_open(argv[i], WRENCH_FILE_MODE_READ);
		if (!src) {
			fprintf(stderr, "Failed to open file '%s'.\n", argv[i]);
			return 1;
		}

		int32_t header_size;
		retval = file_read(&header_size, 4, src) == 4;
		assert(retval == 1);
		retval = file_seek(src, 0, WRENCH_FILE_ORIGIN_START);
		assert(retval == 0);
		assert(header_size < 0x10000);

		std::vector<char> header(header_size);
		retval = file_read(header.data(), header_size, src) == header_size;
		assert(retval == 1);

		file_close(src);

		for (char byte : header) {
			file_printf(dest, "0x%hhx,", byte);
		}
	}
	file_write_string("};}\n", dest);

	file_close(dest);

	return 0;
}
