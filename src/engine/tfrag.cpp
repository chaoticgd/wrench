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

#include "tfrag.h"

std::vector<Tfrag> read_tfrags(Buffer src) {
	TfragsHeader table_header = src.read<TfragsHeader>(0, "tfrags header");
	auto table = src.read_multiple<TfragHeader>(table_header.table_offset, table_header.tfrag_count, "tfrag table");
	for(s32 i = 0; i < table_header.tfrag_count; i++) {
		const TfragHeader& header = table[i];
		printf("%f %f %f\n", header.bsphere.x / 1024.f, header.bsphere.y / 1024.f, header.bsphere.z / 1024.f);
	}
}

void write_tfrags(OutBuffer dest, const std::vector<Tfrag>& tfrags) {
	
}
