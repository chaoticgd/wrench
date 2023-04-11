/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2023 chaoticgd

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

#include "tfrag_high.h"

enum class TfaceType : u8 {
	QUAD, TRI
};

struct Tface {
	TfaceType type;
	u8 side_divisions[3] = {};
	s32 faces[16] = {};
};

struct TfaceOutline {
	s32 ad_gif;
	s32 corner_vertices[4];
};

static std::vector<TfaceOutline> enumerate_tfaces(const Tfrag& tfrag);

ColladaScene recover_tfrags(const Tfrags& tfrags) {
	if(tfrag_debug_output_enabled()) {
		return recover_tfrags_debug(tfrags);
	}
	
	ColladaScene scene;
	
	for(const Tfrag& tfrag : tfrags.fragments) {
		std::vector<TfaceOutline> outlines = enumerate_tfaces(tfrag);
	}
	
	return scene;
}

static std::vector<TfaceOutline> enumerate_tfaces(const Tfrag& tfrag) {
	// At the lowest LOD level, tfaces are represented by just a single face.
	std::vector<TfaceOutline> tfaces;
	s32 active_ad_gif = -1;
	for(const TfragStrip& strip : tfrag.lod_2_strips) {
		s8 vertex_count = strip.vertex_count_and_flag;
		if(vertex_count <= 0) {
			if(vertex_count == 0) {
				break;
			} else if(strip.end_of_packet_flag >= 0) {
				active_ad_gif = strip.ad_gif_offset / 0x5;
			}
			vertex_count += 128;
		}
		verify(vertex_count % 2 == 0, "Triangle strip contains non-quad faces.");
		
		for(s32 i = 0; i < vertex_count - 2; i += 2) {
			TfaceOutline& tface = tfaces.emplace_back();
			tface.ad_gif = active_ad_gif;
			for(s32 j = 0; j < 4; j++) {
				tface.corner_vertices[j] = tfrag.lod_2_indices.at(i + j);
			}
		}
	}
	return tfaces;
}
