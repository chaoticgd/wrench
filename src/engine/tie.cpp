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

#include "tie.h"

#include <algorithm>

static GcUyaDlTieClassHeader read_tie_header(Buffer src, Game game);
static TiePacket read_tie_packet(Buffer src, const TiePacketHeader& header);
static void write_tie_packet(OutBuffer dest, const TiePacket& packet);
static ColladaScene recover_tie(const TieClass& tie);

TieClass read_tie_class(Buffer src, Game game)
{
	TieClass tie;
	
	GcUyaDlTieClassHeader header = read_tie_header(src, game);
	tie.scale = header.scale;
	
	for(s32 i = 0; i < 3; i++) {
		TieLod& lod = tie.lods[i];
		lod.packets.reserve(header.packet_count[i]);
		
		Buffer lod_buffer = src.subbuf(header.packets[i]);
		
		auto packet_table = lod_buffer.read_multiple<TiePacketHeader>(0, header.packet_count[i], "packet header");
		for(s32 j = 0; j < header.packet_count[i]; j++) {
			lod.packets.emplace_back(read_tie_packet(src.subbuf(header.packets[i] + packet_table[j].data), packet_table[j]));
		}
	}
	
	tie.ad_gifs = src.read_multiple<TieAdGifs>(header.ad_gif_ofs, header.texture_count, "ad gifs").copy();
	
	return tie;
}

void write_tie_class(OutBuffer dest, const TieClass& tie)
{
	
}

static GcUyaDlTieClassHeader read_tie_header(Buffer src, Game game)
{
	GcUyaDlTieClassHeader header = {};
	
	if(game == Game::RAC) {
		RacTieClassHeader rac_header = src.read<RacTieClassHeader>(0, "header");
		memcpy(header.packets, rac_header.packets, sizeof(header.packets));
		memcpy(header.packet_count, rac_header.packet_count, sizeof(header.packet_count));
		header.texture_count = rac_header.texture_count;
		header.near_dist = rac_header.near_dist;
		header.mid_dist = rac_header.mid_dist;
		header.far_dist = rac_header.far_dist;
		header.ad_gif_ofs = rac_header.ad_gif_ofs;
		header.scale = rac_header.scale;
		header.bsphere = rac_header.bsphere;
	} else {
		header = src.read<GcUyaDlTieClassHeader>(0, "header");
	}
	
	return header;
}

static TiePacket read_tie_packet(Buffer src, const TiePacketHeader& header)
{
	TiePacket packet;
	
	auto ad_gif_dest_offsets = src.read_multiple<s32>(0x0, 4, "ad gif destination offsets");
	auto ad_gif_src_offsets = src.read_multiple<s32>(0x10, 4, "ad gif source offsets");
	TieUnpackHeader unpack_header = src.read<TieUnpackHeader>(0x20, "header");
	
	s32 strip_ofs = 0x2c;
	auto strips = src.read_multiple<TieStrip>(strip_ofs, unpack_header.strip_count, "strips");
	
	Buffer vertex_buffer = src.subbuf(header.vert_ofs * 0x10, header.vert_size * 0x10);
	s32 dinky_vertex_count = (unpack_header.dinky_vertices_size_plus_four - 4) / 2;
	std::vector<TieDinkyVertex> dinky_vertices = vertex_buffer.read_multiple<TieDinkyVertex>(0, dinky_vertex_count, "dinky vertices").copy();
	auto fat_vertices = vertex_buffer.read_all<TieFatVertex>(dinky_vertex_count * 0x10);
	
	// Combine the dinky and fat vertices.
	std::vector<TieDinkyVertex> raw_vertices;
	for(const TieDinkyVertex& src : dinky_vertices) {
		TieDinkyVertex& dest_1 = raw_vertices.emplace_back();
		dest_1 = src;
		
		if(src.gs_packet_write_ofs_2 != 0 && src.gs_packet_write_ofs_2 != src.gs_packet_write_ofs) {
			TieDinkyVertex& dest_2 = raw_vertices.emplace_back();
			dest_2 = dest_1;
			dest_2.gs_packet_write_ofs = src.gs_packet_write_ofs_2;
		}
	}
	for(const TieFatVertex& src : fat_vertices) {
		TieDinkyVertex& dest_1 = raw_vertices.emplace_back();
		dest_1.x = src.x;
		dest_1.y = src.y;
		dest_1.z = src.z;
		dest_1.gs_packet_write_ofs = src.gs_packet_write_ofs;
		dest_1.s = src.s;
		dest_1.t = src.t;
		dest_1.q = src.q;
		dest_1.gs_packet_write_ofs_2 = src.gs_packet_write_ofs_2;
		
		if(src.gs_packet_write_ofs_2 != 0 && src.gs_packet_write_ofs_2 != src.gs_packet_write_ofs) {
			TieDinkyVertex& dest_2 = raw_vertices.emplace_back();
			dest_2 = dest_1;
			dest_2.gs_packet_write_ofs = src.gs_packet_write_ofs_2;
		}
	}
	
	// The vertices in the file are not sorted by their GS packet address,
	// probably to help with buffering. For the purposes of reading ties, we
	// want to read them in the order they appear in the GS packet.
	std::sort(BEGIN_END(raw_vertices), [](TieDinkyVertex& lhs, TieDinkyVertex& rhs)
		{ return lhs.gs_packet_write_ofs < rhs.gs_packet_write_ofs; });
	
	// Each packet must have a minimum of 4 regular vertices, so there may be
	// duplicates to pad out small packets. These can be safely removed.
	std::vector<TieDinkyVertex> vertices;
	if(raw_vertices.size() >= 1) {
		vertices.emplace_back(raw_vertices[0]);
	}
	for(size_t i = 1; i < raw_vertices.size(); i++) {
		if(raw_vertices[i].gs_packet_write_ofs != raw_vertices[i - 1].gs_packet_write_ofs) {
			vertices.emplace_back(raw_vertices[i]);
		}
	}
	
	s32 material_index = -1;
	TiePrimitive* prim = nullptr;
	
	s32 next_strip = 0;
	s32 next_vertex = 0;
	s32 next_ad_gif = 0;
	s32 next_offset = 0;
	
	// The first AD GIF is always at the beginning of the GIF packet.
	{
		material_index = ad_gif_src_offsets[next_ad_gif++] / 0x50;
		next_offset += 6;
	}
	
	// Interpret the data in the order it would appear in the GS packet.
	while(next_strip < strips.size() || next_vertex < vertices.size()) {
		// Data used to generate GIF tags for each of the strips.
		if(next_strip < strips.size() && strips[next_strip].gif_tag_offset == next_offset) {
			prim = &packet.primitives.emplace_back();
			prim->material_index = material_index;
			// for RC3/4 this is used to indicate which faces need their winding order flipped for backface culling
			prim->winding_order = strips[next_strip].rc34_winding_order != 0;
			
			next_strip++;
			next_offset += 1;
			
			continue;
		}
		
		// Regular vertices.
		if(next_vertex < vertices.size() && vertices[next_vertex].gs_packet_write_ofs == next_offset) {
			verify(prim != nullptr, "Tie has bad GS packet data.");
			prim->vertices.emplace_back(vertices[next_vertex]);
			
			next_vertex++;
			next_offset += 3;
			
			continue;
		}
		
		// AD GIF data to change the texture.
		if(next_ad_gif < ad_gif_src_offsets.size() && ad_gif_dest_offsets[next_ad_gif - 1] == next_offset) {
			material_index = ad_gif_src_offsets[next_ad_gif] / 0x50;
			
			next_ad_gif++;
			next_offset += 6;
			
			continue;
		}
		
		verify_not_reached("Bad GS packet, expected next offset 0x%x.", next_offset);
	}
	
	return packet;
}

static void write_tie_packet(OutBuffer dest, const TiePacket& packet)
{
	
}

ColladaScene recover_tie_class(const TieClass& tie)
{
	ColladaScene scene;
	
	for(s32 i = 0; i < (s32) tie.ad_gifs.size(); i++) {
		ColladaMaterial& material = scene.materials.emplace_back();
		material.name = stringf("%d", i);
		material.surface = MaterialSurface(i);
	}
	
	for(s32 i = 0; i < (s32) tie.ad_gifs.size(); i++) {
		scene.texture_paths.emplace_back(stringf("%d.png", i));
	}
	
	Mesh& mesh = scene.meshes.emplace_back();
	mesh.name = "mesh";
	mesh.flags |= MESH_HAS_TEX_COORDS;
	
	for(const TiePacket& packet : tie.lods[0].packets) {
		for(const TiePrimitive& primitive : packet.primitives) {
			s32 base_vertex = (s32) mesh.vertices.size();
			
			SubMesh& submesh = mesh.submeshes.emplace_back();
			submesh.material = primitive.material_index;
			
			for(s32 i = 0; i < (s32) primitive.vertices.size(); i++) {
				Vertex& dest = mesh.vertices.emplace_back();
				const TieDinkyVertex& src = primitive.vertices[i];
				dest.pos.x = src.x * (tie.scale / 1024.f);
				dest.pos.y = src.y * (tie.scale / 1024.f);
				dest.pos.z = src.z * (tie.scale / 1024.f);
				dest.tex_coord.s = vu_fixed12_to_float(src.s);
				dest.tex_coord.t = vu_fixed12_to_float(src.t);
				
				if(i >= 2) {
					if(i % 2 == primitive.winding_order) {
						submesh.faces.emplace_back(base_vertex + i - 2, base_vertex + i - 1, base_vertex + i);
					} else {
						submesh.faces.emplace_back(base_vertex + i - 0, base_vertex + i - 1, base_vertex + i - 2);
					}
				}
			}
		}
	}
	
	return scene;
}
