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

#include "core/png.h"

int main(int argc, char** argv) {
	if(argc != 2) {
		printf("usage: %s <path to png file>", argv[0]);
		return 1;
	}
	Opt<Texture> texture = read_png(argv[1]);
	verify(texture.has_value(), "Failed to read PNG file.");
	std::string hash = hash_texture(*texture);
	printf("md5: %s\n", hash.c_str());
}
