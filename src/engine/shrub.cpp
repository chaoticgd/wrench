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

#include "shrub.h"

#include <algorithm>

#include <core/tristrip_packet.h>
#include <core/vif.h>

static std::vector<TriStripConstraint> setup_shrub_constraints();
static f32 compute_optimal_scale(const GLTF::Mesh& mesh);
static std::pair<std::vector<ShrubNormal>, std::vector<s32>> compute_normal_clusters(
	const std::vector<Vertex>& vertices);
static s32 compute_lod_k(f32 distance);

ShrubClass read_shrub_class(Buffer src)
{
	ShrubClass shrub;
	
	ShrubClassHeader header = src.read<ShrubClassHeader>(0, "shrub header");
	
	shrub.bounding_sphere = header.bounding_sphere;
	shrub.mip_distance = header.mip_distance;
	shrub.mode_bits = header.mode_bits;
	shrub.scale = header.scale;
	shrub.o_class = header.o_class;
	
	for (auto& entry : src.read_multiple<ShrubPacketEntry>(sizeof(ShrubClassHeader), header.packet_count, "packet entry")) {
		ShrubPacket& packet = shrub.packets.emplace_back();
		
		Buffer command_buffer = src.subbuf(entry.offset, entry.size);
		std::vector<VifPacket> command_list = read_vif_command_list(command_buffer);
		std::vector<VifPacket> unpacks = filter_vif_unpacks(command_list);
		
		verify(unpacks.size() == 3, "Wrong number of unpacks.");
		Buffer header_unpack = unpacks[0].data;
		auto packet_header = header_unpack.read<ShrubPacketHeader>(0, "packet header");
		auto gif_tags = header_unpack.read_multiple<ShrubVertexGifTag>(0x10, packet_header.gif_tag_count, "gif tags");
		auto ad_gif = header_unpack.read_multiple<ShrubTexturePrimitive>(0x10 + packet_header.gif_tag_count * 0x10, packet_header.texture_count, "gs ad data");
		
		auto part_1 = Buffer(unpacks[1].data).read_multiple<ShrubVertexPart1>(0, packet_header.vertex_count, "vertices");
		auto part_2 = Buffer(unpacks[2].data).read_multiple<ShrubVertexPart2>(0, packet_header.vertex_count, "sts");
		
		ShrubVertexPrimitive* prim = nullptr;
		GeometryType prim_type;
		
		// Interpret the data in the order it would appear in the GS packet.
		s32 next_gif_tag = 0;
		s32 next_ad_gif = 0;
		s32 next_vertex = 0;
		s32 next_offset = 0;
		while (next_gif_tag < gif_tags.size() || next_ad_gif < ad_gif.size() || next_vertex < part_1.size()) {
			// GIF tags for the vertices (not the AD data).
			if (next_gif_tag < gif_tags.size() && gif_tags[next_gif_tag].gs_packet_offset == next_offset) {
				prim = nullptr;
				GsPrimRegister reg{(u32) gif_tags[next_gif_tag].tag.prim()};
				switch (reg.primitive()) {
					case GS_PRIMITIVE_TRIANGLE:
						prim_type = GeometryType::TRIANGLE_LIST;
						break;
					case GS_PRIMITIVE_TRIANGLE_STRIP:
						prim_type = GeometryType::TRIANGLE_STRIP;
						break;
					default: {
						verify_not_reached("Shrub data has primitives that aren't triangle lists or triangle strips.");
					}
				}
				
				next_gif_tag++;
				next_offset += 1;
				
				continue;
			}
			
			// AD data to change the texture.
			if (next_ad_gif < ad_gif.size() && ad_gif[next_ad_gif].gs_packet_offset == next_offset) {
				packet.primitives.emplace_back(ad_gif[next_ad_gif]);
				prim = nullptr;
				
				next_ad_gif++;
				next_offset += 5;
				
				continue;
			}
			
			// Normal vertices.
			if (next_vertex < part_1.size() && part_1[next_vertex].gs_packet_offset == next_offset) {
				if (prim == nullptr) {
					prim = &packet.primitives.emplace_back().emplace<1>();
				}
				
				prim->type = prim_type;
				
				ShrubVertex& vertex = prim->vertices.emplace_back();
				vertex.x = part_1[next_vertex].x;
				vertex.y = part_1[next_vertex].y;
				vertex.z = part_1[next_vertex].z;
				vertex.s = part_2[next_vertex].s;
				vertex.t = part_2[next_vertex].t;
				vertex.h = part_2[next_vertex].h;
				vertex.n = part_2[next_vertex].n_and_stop_cond & 0b0111111111111111;
				
				next_vertex++;
				next_offset += 3;
				
				continue;
			}
			
			// Padding vertices at the end of a small packet, all of which write over the same address.
			if (next_vertex < part_1.size() && part_1[next_vertex].gs_packet_offset == next_offset - 3) {
				break;
			}
			
			verify_not_reached("Bad shrub data.");
		}
	}
	
	if (header.billboard_offset > 0) {
		shrub.billboard = src.read<ShrubBillboard>(header.billboard_offset, "shrub billboard");
	}
	shrub.normals = src.read_multiple<ShrubNormal>(header.normals_offset, 24, "shrub normals").copy();
	
	return shrub;
}

void write_shrub_class(OutBuffer dest, const ShrubClass& shrub)
{
	s64 header_ofs = dest.alloc<ShrubClassHeader>();
	ShrubClassHeader header = {};
	
	// Fill in the header.
	header.bounding_sphere = shrub.bounding_sphere;
	header.mip_distance = shrub.mip_distance;
	header.mode_bits = shrub.mode_bits;
	header.scale = shrub.scale;
	header.o_class = shrub.o_class;
	header.packet_count = (s32) shrub.packets.size();
	
	s64 packet_list_ofs = dest.alloc_multiple<ShrubPacketEntry>(shrub.packets.size());
	
	// Write out the VIF command lists.
	for (const ShrubPacket& packet : shrub.packets) {
		ShrubPacketEntry entry;
		
		dest.pad(0x10, 0);
		s64 begin_ofs = dest.tell();
		entry.offset = begin_ofs - header_ofs;
		
		// Write command list prologue.
		dest.write<u32>(0x01000404); // stcycl
		dest.write<u32>(0x00000000); // nop
		dest.write<u32>(0x05000000); // stmod
		
		s32 offset = 0;
		
		// Transform the data into the order it will be written out in.
		ShrubPacketHeader packet_header = {};
		packet_header.vertex_offset = 1;
		std::vector<ShrubVertexGifTag> gif_tags;
		std::vector<ShrubTexturePrimitive> textures;
		std::vector<ShrubVertexPart1> part_1;
		std::vector<ShrubVertexPart2> part_2;
		for (const ShrubPrimitive& primitive : packet.primitives) {
			if (const ShrubTexturePrimitive* prim = std::get_if<ShrubTexturePrimitive>(&primitive)) {
				packet_header.texture_count++;
				packet_header.vertex_offset += 4;
				
				ShrubTexturePrimitive& texture = textures.emplace_back(*prim);
				texture.gs_packet_offset = offset;
				offset += 5;
			}
			if (const ShrubVertexPrimitive* prim = std::get_if<ShrubVertexPrimitive>(&primitive)) {
				packet_header.gif_tag_count++;
				packet_header.vertex_offset += 1;
				packet_header.vertex_count += (s32) prim->vertices.size();
				
				GsPrimRegister prim_reg;
				switch (prim->type) {
					case GeometryType::TRIANGLE_LIST:
						prim_reg.set_primitive(GS_PRIMITIVE_TRIANGLE);
						break;
					case GeometryType::TRIANGLE_STRIP:
						prim_reg.set_primitive(GS_PRIMITIVE_TRIANGLE_STRIP);
						break;
					case GeometryType::TRIANGLE_FAN:
						prim_reg.set_primitive(GS_PRIMITIVE_TRIANGLE_FAN);
						break;
				}
				prim_reg.set_iip(1);
				prim_reg.set_tme(1);
				prim_reg.set_fge(1);
				prim_reg.set_abe(1);
				prim_reg.set_aa1(0);
				prim_reg.set_fst(0);
				prim_reg.set_ctxt(0);
				prim_reg.set_fix(0);
				
				ShrubVertexGifTag& gif = gif_tags.emplace_back();
				gif.tag = {};
				gif.tag.set_nloop((u32) prim->vertices.size());
				gif.tag.set_pre(1);
				gif.tag.set_prim(prim_reg.val);
				gif.tag.set_flg(0);
				gif.tag.set_nreg(3);
				gif.tag.regs = 0x00000412;
				gif.gs_packet_offset = offset;
				offset += 1;
				
				for (const ShrubVertex& vertex : prim->vertices) {
					ShrubVertexPart1& p1 = part_1.emplace_back();
					p1.x = vertex.x;
					p1.y = vertex.y;
					p1.z = vertex.z;
					p1.gs_packet_offset = offset;
					ShrubVertexPart2& p2 = part_2.emplace_back();
					p2.s = vertex.s;
					p2.t = vertex.t;
					p2.h = vertex.h;
					p2.n_and_stop_cond = vertex.n;
					offset += 3;
				}
			}
		}
		
		verify_fatal(gif_tags.size() >= 1);
		gif_tags.back().tag.set_eop(true);
		
		// Insert padding vertices if there are less than 6 real vertices.
		s32 vertex_count = (s32) part_1.size();
		for (s32 i = 0; i < std::max(0, 6 - vertex_count); i++) {
			part_1.emplace_back(part_1.back());
			part_2.emplace_back(part_2.back());
			packet_header.vertex_count++;
		}
		
		// Write the stopping condition bit.
		part_2[part_2.size() - 4].n_and_stop_cond |= 0b1000000000000000;
		
		// Write header/gif tag/ad data unpack.
		VifCode header_code;
		header_code.interrupt = 0;
		header_code.cmd = (VifCmd) 0b1100000;
		header_code.num = 1 + packet_header.gif_tag_count + packet_header.texture_count * 4;
		header_code.unpack.vnvl = VifVnVl::V4_32;
		header_code.unpack.flg = VifFlg::USE_VIF1_TOPS;
		header_code.unpack.usn = VifUsn::SIGNED;
		header_code.unpack.addr = 0;
		dest.write<u32>(header_code.encode_unpack());
		dest.write(packet_header);
		dest.write_multiple(gif_tags);
		dest.write_multiple(textures);
		
		dest.write<u32>(0x05000000); // stmod
		
		// Write the primary vertex table.
		VifCode part_1_code;
		part_1_code.interrupt = 0;
		part_1_code.cmd = (VifCmd) 0b1100000;
		part_1_code.num = part_1.size();
		part_1_code.unpack.vnvl = VifVnVl::V4_16;
		part_1_code.unpack.flg = VifFlg::USE_VIF1_TOPS;
		part_1_code.unpack.usn = VifUsn::SIGNED;
		part_1_code.unpack.addr = packet_header.vertex_offset;
		dest.write<u32>(part_1_code.encode_unpack());
		dest.write_multiple(part_1);
		
		dest.write<u32>(0x05000000); // stmod
		
		// Write the secondary vertex table.
		VifCode part_2_code;
		part_2_code.interrupt = 0;
		part_2_code.cmd = (VifCmd) 0b1100000;
		part_2_code.num = part_2.size();
		part_2_code.unpack.vnvl = VifVnVl::V4_16;
		part_2_code.unpack.flg = VifFlg::USE_VIF1_TOPS;
		part_2_code.unpack.usn = VifUsn::SIGNED;
		part_2_code.unpack.addr = packet_header.vertex_offset + part_1.size();
		dest.write<u32>(part_2_code.encode_unpack());
		dest.write_multiple(part_2);
		
		entry.size = dest.tell() - begin_ofs;
		
		dest.write(packet_list_ofs, entry);
		packet_list_ofs += sizeof(ShrubPacketEntry);
	}
	
	// Write out the billboard.
	if (shrub.billboard.has_value()) {
		dest.pad(0x10, 0);
		header.billboard_offset = (s32) (dest.tell() - header_ofs);
		dest.write(*shrub.billboard);
	}
	
	// Write out the palette.
	dest.pad(0x10, 0);
	header.normals_offset = (s32) (dest.tell() - header_ofs);
	dest.write_multiple(shrub.normals);
	
	dest.write(header_ofs, header);
}

GLTF::Mesh recover_shrub_class(const ShrubClass& shrub)
{
	GLTF::Mesh mesh;
	mesh.name = "mesh";
	
	GLTF::MeshPrimitive* dest_primitive = nullptr;
	s32 texture_index = -1;
	
	for (const ShrubPacket& packet : shrub.packets) {
		for (const ShrubPrimitive& src_primitive : packet.primitives) {
			if (const ShrubTexturePrimitive* prim = std::get_if<ShrubTexturePrimitive>(&src_primitive)) {
				texture_index = prim->d4_tex0_1.data_lo;
			}
			
			if (const ShrubVertexPrimitive* prim = std::get_if<ShrubVertexPrimitive>(&src_primitive)) {
				if (dest_primitive == nullptr || texture_index != *dest_primitive->material) {
					dest_primitive = &mesh.primitives.emplace_back();
					dest_primitive->attributes_bitfield = GLTF::POSITION | GLTF::TEXCOORD_0 | GLTF::NORMAL;
					dest_primitive->material = texture_index;
				}
				
				s32 base_index = (s32) mesh.vertices.size();
				for (const ShrubVertex& vertex : prim->vertices) {
					Vertex& dest_vertex = mesh.vertices.emplace_back();
					f32 x = vertex.x * shrub.scale * (1.f / 1024.f);
					f32 y = vertex.y * shrub.scale * (1.f / 1024.f);
					f32 z = vertex.z * shrub.scale * (1.f / 1024.f);
					dest_vertex.pos = glm::vec3(x, y, z);
					f32 nx = shrub.normals[vertex.n].x * (1.f / INT16_MAX);
					f32 ny = shrub.normals[vertex.n].y * (1.f / INT16_MAX);
					f32 nz = shrub.normals[vertex.n].z * (1.f / INT16_MAX);
					dest_vertex.normal = glm::vec3(nx, ny, nz);
					dest_vertex.tex_coord.s = vu_fixed12_to_float(vertex.s);
					dest_vertex.tex_coord.t = vu_fixed12_to_float(vertex.t);
				}
				
				if (prim->type == GeometryType::TRIANGLE_LIST) {
					for (s32 i = base_index; i < (s32) mesh.vertices.size(); i++) {
						dest_primitive->indices.emplace_back(i);
					}
				} else {
					for (s32 i = base_index; i < (s32) mesh.vertices.size() - 2; i++) {
						dest_primitive->indices.emplace_back(i + 0);
						dest_primitive->indices.emplace_back(i + 1);
						dest_primitive->indices.emplace_back(i + 2);
					}
				}
			}
		}
	}

	GLTF::deduplicate_vertices(mesh);
	GLTF::remove_zero_area_triangles(mesh);
	
	// The winding orders of the faces weren't preserved by Insomniac's triangle
	// stripper, so we need to recalculate them here.
	GLTF::fix_winding_orders_of_triangles_based_on_normals(mesh);
	
	return mesh;
}

ShrubClass build_shrub_class(
	const GLTF::Mesh& mesh,
	const std::vector<Material>& materials,
	f32 mip_distance,
	u16 mode_bits,
	s16 o_class,
	Opt<ShrubBillboardInfo> billboard_info)
{
	ShrubClass shrub = {};
	shrub.mip_distance = mip_distance;
	shrub.mode_bits = mode_bits;
	shrub.scale = compute_optimal_scale(mesh);
	shrub.o_class = o_class;
	
	shrub.bounding_sphere = Vec4f::pack(approximate_bounding_sphere(mesh.vertices) / shrub.scale);
	
	auto [normals, normal_indices] = compute_normal_clusters(mesh.vertices);
	
	TriStripConfig config;
	// Make sure the packets that get written out aren't too big to fit in VU1 memory.
	config.constraints = setup_shrub_constraints();
	// The shrub renderer doesn't use an index buffer.
	config.support_index_buffer = false;
	// Make sure AD GIFs are added at the beginning of each packet.
	config.support_instancing = true;
	
	// Generate the strips.
	auto [effectives, material_to_effective] = effective_materials(materials, MATERIAL_ATTRIB_SURFACE | MATERIAL_ATTRIB_WRAP_MODE);
	GeometryPrimitives primitives = weave_tristrips(mesh, effectives);
	GeometryPackets output = generate_tristrip_packets(primitives, config);
	
	// Build the shrub packets.
	for (const GeometryPacket& src_packet : output.packets) {
		s32 last_effective_material = -1;
		ShrubPacket& dest_packet = shrub.packets.emplace_back();
		for (s32 i = 0; i < src_packet.primitive_count; i++) {
			const GeometryPrimitive& src_primitive = output.primitives[src_packet.primitive_begin + i];
			verify(src_primitive.effective_material > -1, "Bad material index.");
			
			if (src_primitive.effective_material != last_effective_material) {
				const EffectiveMaterial& effective = effectives.at(src_primitive.effective_material);
				const Material& material = materials.at(effective.materials.at(0));
				verify(material.surface.type == MaterialSurfaceType::TEXTURE,
					"A shrub material does not have a texture.");
				
				// The data written here doesn't match the layout of the
				// respective GS registers. This is because the data is fixed up
				// at runtime by the game.
				ShrubTexturePrimitive& dest_primitive = dest_packet.primitives.emplace_back().emplace<ShrubTexturePrimitive>();
				dest_primitive.d1_tex1_1.address = GIF_AD_TEX1_1;
				dest_primitive.d1_tex1_1.data_lo = compute_lod_k(mip_distance);
				dest_primitive.d1_tex1_1.data_hi = 0x04; // mmin
				dest_primitive.d2_clamp_1.address = GIF_AD_CLAMP_1;
				if (material.wrap_mode_s == WrapMode::CLAMP) {
					dest_primitive.d2_clamp_1.data_lo = 1;
				}
				if (material.wrap_mode_t == WrapMode::CLAMP) {
					dest_primitive.d2_clamp_1.data_hi = 1;
				}
				dest_primitive.d3_miptbp1_1.address = GIF_AD_MIPTBP1_1;
				dest_primitive.d3_miptbp1_1.data_lo = material.surface.texture;
				dest_primitive.d4_tex0_1.address = GIF_AD_TEX0_1;
				dest_primitive.d4_tex0_1.data_lo = material.surface.texture;
				
				last_effective_material = src_primitive.effective_material;
			}
			
			ShrubVertexPrimitive& dest_primitive = dest_packet.primitives.emplace_back().emplace<ShrubVertexPrimitive>();
			dest_primitive.type = src_primitive.type;
			for (s32 j = 0; j < src_primitive.index_count; j++) {
				ShrubVertex& dest_vertex = dest_primitive.vertices.emplace_back();
				s32 vertex_index = output.indices.at(src_primitive.index_begin + j);
				const Vertex& src = mesh.vertices.at(vertex_index);
				f32 x = src.pos.x * (1.f / shrub.scale) * 1024.f;
				f32 y = src.pos.y * (1.f / shrub.scale) * 1024.f;
				f32 z = src.pos.z * (1.f / shrub.scale) * 1024.f;
				verify_fatal(x >= INT16_MIN && x <= INT16_MAX
					&& y >= INT16_MIN && y <= INT16_MAX
					&& z >= INT16_MIN && z <= INT16_MAX);
				dest_vertex.x = (s16) x;
				dest_vertex.y = (s16) y;
				dest_vertex.z = (s16) z;
				dest_vertex.s = vu_float_to_fixed12(src.tex_coord.x);
				dest_vertex.t = vu_float_to_fixed12(src.tex_coord.y);
				dest_vertex.h = vu_float_to_fixed12(1.f);
				dest_vertex.n = normal_indices[vertex_index];
			}
		}
	}
	
	if (billboard_info.has_value()) {
		ShrubBillboard billboard = {};
		billboard.fade_distance = billboard_info->fade_distance;
		billboard.width = billboard_info->width;
		billboard.height = billboard_info->height;
		billboard.z_ofs = billboard_info->z_ofs;
		billboard.d1_tex1_1.data_lo = compute_lod_k(billboard_info->fade_distance);
		billboard.d1_tex1_1.data_hi = 4;
		billboard.d2_tex0_1.data_lo = 1;
		shrub.billboard = billboard;
	}
	shrub.normals = std::move(normals);
	
	return shrub;
}

static std::vector<TriStripConstraint> setup_shrub_constraints()
{
	std::vector<TriStripConstraint> constraints;
	
	TriStripConstraint& unpacked_data_size = constraints.emplace_back();
	unpacked_data_size.constant_cost = 1; // header
	unpacked_data_size.strip_cost = 1; // gif tag
	unpacked_data_size.vertex_cost = 0; // non-indexed
	unpacked_data_size.index_cost = 2; // second and third unpacks
	unpacked_data_size.material_cost = 4; // ad data
	unpacked_data_size.max_cost = 118; // buffer size
	
	TriStripConstraint& gs_packet_size = constraints.emplace_back();
	gs_packet_size.constant_cost = 0;
	gs_packet_size.strip_cost = 1; // gif tag
	gs_packet_size.vertex_cost = 0; // non-indexed
	gs_packet_size.index_cost = 3; // st rgbaq xyzf2
	gs_packet_size.material_cost = 5; // gif tag + ad data
	gs_packet_size.max_cost = 168; // max GS packet size in original files
	
	// The VIF packet size is bounded by the unpacked data size, so no
	// additional checks need to be made for it.
	
	return constraints;
}

static f32 compute_optimal_scale(const GLTF::Mesh& mesh)
{
	// Compute the minimum axis-aligned bounding box.
	f32 xmin = 0.f, xmax = 0.f;
	f32 ymin = 0.f, ymax = 0.f;
	f32 zmin = 0.f, zmax = 0.f;
	for (const Vertex& v : mesh.vertices) {
		xmin = std::min(xmin, v.pos.x);
		ymin = std::min(ymin, v.pos.y);
		zmin = std::min(zmin, v.pos.z);
		
		xmax = std::max(xmax, v.pos.x);
		ymax = std::max(ymax, v.pos.y);
		zmax = std::max(zmax, v.pos.z);
	}
	// Find the largest vertex position component we have to represent.
	f32 required_range = std::max({
		fabsf(xmin), fabsf(xmax),
		fabsf(ymin), fabsf(ymax),
		fabsf(zmin), fabsf(zmax)});
	// Calculate a scale such that said value is quantized to the largest
	// representable value.
	return required_range * (1024.f / (INT16_MAX - 1.f));
}

static std::pair<std::vector<ShrubNormal>, std::vector<s32>> compute_normal_clusters(
	const std::vector<Vertex>& vertices)
{
	// https://stackoverflow.com/questions/9600801/evenly-distributing-n-points-on-a-sphere
	std::vector<glm::vec3> clusters = {
		{0.0, 1.0, 0.0},
		{-0.3007449209690094, 0.9130434989929199, 0.27550697326660156},
		{0.049268126487731934, 0.8260869383811951, -0.561384916305542},
		{0.409821480512619, 0.739130437374115, 0.5345395803451538},
		{-0.7464811205863953, 0.6521739363670349, -0.13204200565814972},
		{0.696049153804779, 0.5652173757553101, -0.44276949763298035},
		{-0.22798912227153778, 0.47826087474823, 0.848108172416687},
		{-0.4241549074649811, 0.3913043439388275, -0.8166844844818115},
		{0.89476078748703, 0.30434781312942505, 0.3267652094364166},
		{-0.9022393822669983, 0.21739129722118378, 0.37243130803108215},
		{0.42022502422332764, 0.1304347813129425, -0.8979964852333069},
		{0.2990008592605591, 0.043478261679410934, 0.9532618522644043},
		{-0.8643930554389954, -0.043478261679410934, -0.5009334087371826},
		{0.9683319330215454, -0.1304347813129425, -0.2128850519657135},
		{-0.5613749623298645, -0.21739129722118378, 0.7984980940818787},
		{-0.12241426855325699, -0.30434781312942505, -0.9446624517440796},
		{0.7036768794059753, -0.3913043439388275, 0.5930596590042114},
		{-0.8774678707122803, -0.47826087474823, 0.03628605231642723},
		{0.5847431421279907, -0.5652173757553101, -0.5818975567817688},
		{-0.035016320645809174, -0.6521739363670349, 0.7572602033615112},
		{-0.4315575361251831, -0.739130437374115, -0.5171501636505127},
		{0.558509886264801, -0.8260869383811951, 0.07514671981334686},
		{-0.33479711413383484, -0.9130434989929199, 0.2329431176185608},
		{0.0, -1.0, -0.0}
	};
	
	// TODO: Refine clusters e.g. using k-means algorithm?
	
	// Quantize the normal for each clusters.
	std::vector<ShrubNormal> normals;
	normals.reserve(24);
	for (glm::vec3& cluster : clusters) {
		ShrubNormal& normal = normals.emplace_back();
		normal.x = (s16) roundf(cluster.x * INT16_MAX);
		normal.y = (s16) roundf(cluster.y * INT16_MAX);
		normal.z = (s16) roundf(cluster.z * INT16_MAX);
	}
	
	// Generate a mapping of vertex indices to cluster indices.
	std::vector<s32> indices;
	indices.reserve(vertices.size());
	for (const Vertex& vertex : vertices) {
		// Find which cluster the vertex is in.
		s32 best = -1;
		f32 distance = 0.f;
		for (s32 j = 0; j < 24; j++) {
			f32 new_distance = glm::distance(clusters[j], vertex.normal);
			if (best == -1 || new_distance < distance) {
				best = j;
				distance = new_distance;
			}
		}
		if (best == -1) {
			printf("warning: Failed to assign vertex to cluster. This might be due to bad normals.\n");
			best = 0;
		}
		indices.emplace_back(best);
	}
	
	return {normals, indices};
}

static s32 compute_lod_k(f32 distance)
{
	// This is similar to the equation in the GS User's Manual and seems to fit
	// most of the points in the original files. It's kinda off for larger
	// distances such as those of billbaords but I'm not really sure.
	if (distance < 0.0001f) {
		distance = 0.0001f;
	}
	s16 k = (s16) roundf(-log2(distance) * 16 - 73.f);
	return (s32) (u32) (u16) k;
}
