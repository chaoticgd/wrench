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
#include "../formats/fip.h"

int main(int argc, char** argv) {
	return run_cli_converter(argc, argv,
		"Converts indexed colour textures in the FIP format to BMP files",
		{
			{ "export", fip_to_bmp },
			{ "import", bmp_to_fip }
		});
}
