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

#include "vag.h"

Sector32 get_vag_size(InputStream& src, Sector32 sector)
{
	VagHeader header = src.read<VagHeader>(sector.bytes());
	if (memcmp(header.magic, "VAGp", 4) != 0) {
		return {1};
	}
	return Sector32::size_from_bytes(sizeof(VagHeader) + byte_swap_32(header.data_size));
}
