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

#include "shrub.h"

#include <core/vif.h>

ShrubClass read_shrub_class(Buffer src) {
	ShrubClass shrub;
	
	ShrubClassHeader header = src.read<ShrubClassHeader>(0, "shrub header");
	
	s32 texture_index = -1;
	
	for(auto& entry : src.read_multiple<ShrubPacketEntry>(sizeof(ShrubClassHeader), header.packet_count, "packet entry")) {
		Buffer command_buffer = src.subbuf(entry.offset, entry.size);
		std::vector<VifPacket> command_list = read_vif_command_list(command_buffer);
		std::vector<VifPacket> unpacks = filter_vif_unpacks(command_list);
		
		verify(unpacks.size() == 3, "Wrong number of unpacks.");
		Buffer header_unpack = unpacks[0].data;
		auto header = header_unpack.read<ShrubPacketHeader>(0, "packet header");
		auto gif_tags = header_unpack.read_multiple<ShrubVertexGifTag>(0x10, header.gif_tag_count, "gif tags");
		auto ad_gif = header_unpack.read_multiple<ShrubTexturePrimitive>(0x10 + header.gif_tag_count * 0x10, header.texture_count, "gs ad data");
		
		auto vertices = Buffer(unpacks[1].data).read_multiple<ShrubVertex>(0, header.vertex_count, "vertices");
		auto sts = Buffer(unpacks[2].data).read_multiple<ShrubTexCoord>(0, header.vertex_count, "sts");
		
		// Interpret the data in the order it would be written to the GS packet.
		s32 next_gif_tag = 0;
		s32 next_ad_gif = 0;
		s32 next_vertex = 0;
		s32 next_offset = 0;
		while(next_gif_tag < gif_tags.size() || next_ad_gif < ad_gif.size() || next_vertex < vertices.size()) {
			bool progress = false;
			
			if(next_gif_tag < gif_tags.size() && gif_tags[next_gif_tag].gs_packet_offset == next_offset) {
				next_gif_tag++;
				next_offset += 1;
				progress = true;
			}
			
			if(next_ad_gif < ad_gif.size() && ad_gif[next_ad_gif].gs_packet_offset == next_offset) {
				texture_index = ad_gif[next_ad_gif].tex0_1.data_lo;
				
				next_ad_gif++;
				next_offset += 5;
				progress = true;
			}
			
			if(next_vertex < vertices.size() && vertices[next_vertex].gs_packet_offset == next_offset) {
				next_vertex++;
				next_offset += 3;
				progress = true;
			}
			
			verify(progress, "Bad shrub data, will build broken GS packet.");
		}
	}
	
	return shrub;
}

void write_shrub_class(OutBuffer dest, const ShrubClass& shrub) {
	
}
