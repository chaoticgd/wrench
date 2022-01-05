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

#include "moby.h"

// Debug settings.
#define MOBY_EXPORT_SUBMESHES_SEPERATELY false

#define WRENCH_PI 3.14159265358979323846

static MobyBangles read_moby_bangles(Buffer src);
static s64 write_moby_bangles(OutBuffer dest, const MobyBangles& bangles);
static MobyCornCob read_moby_corncob(Buffer src);
static s64 write_moby_corncob(OutBuffer dest, const MobyCornCob& corncob);
static std::vector<Opt<MobySequence>> read_moby_sequences(Buffer src, s64 sequence_count, s32 joint_count, Game game);
static void write_moby_sequences(OutBuffer dest, const std::vector<Opt<MobySequence>>& sequences, s64 list_ofs, s32 joint_count, Game game);
static MobyCollision read_moby_collision(Buffer src);
static s64 write_moby_collision(OutBuffer dest, const MobyCollision& collision);
static std::vector<MobyJointEntry> read_moby_joints(Buffer src, s64 joints_ofs);
static s64 write_moby_joints(OutBuffer dest, const std::vector<MobyJointEntry>& joints);
static std::vector<MobySubMesh> read_moby_submeshes(Buffer src, s64 table_ofs, s64 count, MobyFormat format);
using GifUsageTable = std::vector<MobyGifUsageTableEntry>;
static void write_moby_submeshes(OutBuffer dest, GifUsageTable& gif_usage, s64 table_ofs, const std::vector<MobySubMesh>& submeshes, MobyFormat format);
static std::vector<MobyMetalSubMesh> read_moby_metal_submeshes(Buffer src, s64 table_ofs, s64 count);
static void write_moby_metal_submeshes(OutBuffer dest, s64 table_ofs, const std::vector<MobyMetalSubMesh>& submeshes);
static s64 write_shared_moby_vif_packets(OutBuffer dest, GifUsageTable* gif_usage, const MobySubMeshBase& submesh);
#define NO_SUBMESH_FILTER -1
static Mesh recover_moby_mesh(const std::vector<MobySubMesh>& submeshes, const char* name, s32 o_class, s32 texture_count, f32 scale, s32 submesh_filter);
static std::vector<Joint> recover_moby_joints(const MobyClassData& moby);
static std::vector<MobySubMesh> build_moby_submeshes(const Mesh& mesh, const std::vector<Material>& materials, f32 scale);
static Vertex recover_vertex(const MobyVertex& vertex, const MobyTexCoord& tex_coord, f32 scale);
static MobyVertex build_vertex(const Vertex& v, s32 id, f32 inverse_scale);
struct BlendAttributes {
	s32 count;
	u8 joints[4];
	f32 weights[4];
};
static BlendAttributes recover_blend_attributes(Opt<BlendAttributes> blend_buffer[64], const MobyVertex& mv, s32 ind, s32 two_way_count, s32 three_way_count);
struct IndexMappingRecord {
	s32 submesh = -1;
	s32 index = -1; // The index of the vertex in the vertex table.
	s32 id = -1; // The index of the vertex in the intermediate buffer.
	s32 dedup_out_edge = -1; // If this vertex is a duplicate, this points to the canonical vertex.
};
static void find_duplicate_vertices(std::vector<IndexMappingRecord>& index_mapping, const std::vector<Vertex>& vertices);
static f32 acotf(f32 x);

// FIXME: Figure out what points to the mystery data instead of doing this.
static s64 mystery_data_ofs;

MobyClassData read_moby_class(Buffer src, Game game) {
	auto header = src.read<MobyClassHeader>(0, "moby class header");
	MobyClassData moby;
	moby.submesh_count = header.submesh_count;
	moby.low_lod_submesh_count = header.low_lod_submesh_count;
	moby.metal_submesh_count = header.metal_submesh_count;
	moby.joint_count = header.joint_count;
	moby.unknown_9 = header.unknown_9;
	moby.rac1_byte_a = header.rac1_byte_a;
	moby.rac1_byte_b = header.rac12_byte_b;
	moby.lod_trans = header.lod_trans;
	moby.shadow = header.shadow;
	moby.scale = header.scale;
	moby.mip_dist = header.mip_dist;
	moby.bounding_sphere = header.bounding_sphere.unpack();
	moby.glow_rgba = header.glow_rgba;
	moby.mode_bits = header.mode_bits;
	moby.type = header.type;
	moby.mode_bits2 = header.mode_bits2;
	mystery_data_ofs = src.read<s32>(0x48, "moby sequences");
	
	MobyFormat format;
	switch(game) {
		case Game::RAC1:
			format = MobyFormat::RAC1;
			break;
		case Game::RAC2:
			if(header.rac12_byte_b == 0) {
				format = MobyFormat::RAC2;
			} else {
				format = MobyFormat::RAC1;
				moby.force_rac1_format = true;
			}
			break;
		case Game::RAC3:
		case Game::DL:
			format = MobyFormat::RAC3DL;
			break;
		default:
			assert_not_reached("Bad game enum.");
	}
	moby.header_end_offset = 0x48;
	for(s32 seq_offset : src.read_multiple<s32>(0x48, header.sequence_count, "sequence offsets")) {
		if(seq_offset != 0) {
			moby.header_end_offset = seq_offset;
			break;
		}
	}
	if(header.bangles != 0) {
		moby.bangles = read_moby_bangles(src.subbuf(header.bangles * 0x10));
		moby.header_end_offset = std::min(moby.header_end_offset, header.bangles * 0x10);
	}
	if(game == Game::RAC1)  {
		moby.rac1_short_2e = header.corncob;
	} else if(header.corncob != 0) {
		moby.corncob = read_moby_corncob(src.subbuf(header.corncob * 0x10));
		moby.header_end_offset = std::min(moby.header_end_offset, header.corncob * 0x10);
	}
	if(game != Game::DL) { // TODO: Get this working.
		moby.sequences = read_moby_sequences(src, header.sequence_count, header.joint_count, game);
	}
	verify(header.sequence_count >= 1, "Moby class has no sequences.");
	if(header.collision != 0) {
		moby.collision = read_moby_collision(src.subbuf(header.collision));
		s64 coll_size = 0x10 + moby.collision->first_part.size() + moby.collision->second_part.size() * 8 + moby.collision->third_part.size();
		mystery_data_ofs = std::max(mystery_data_ofs, header.collision + coll_size);
	}
	if(header.skeleton != 0) {
		moby.skeleton = src.read_multiple<Mat4>(header.skeleton, header.joint_count, "skeleton").copy();
	}
	if(header.common_trans != 0) {
		moby.common_trans = src.read_multiple<MobyTrans>(header.common_trans, header.joint_count, "skeleton trans").copy();
	}
	if(game != Game::DL) { // TODO: Get this working.
		moby.joints = read_moby_joints(src, header.joints);
	}
	moby.sound_defs = src.read_multiple<MobySoundDef>(header.sound_defs, header.sound_count, "moby sound defs").copy();
	if(header.submesh_table_offset != 0) {
		moby.has_submesh_table = true;
		moby.submesh_table_offset = header.submesh_table_offset;
		moby.submeshes = read_moby_submeshes(src, header.submesh_table_offset, header.submesh_count, format);
		moby.low_lod_submeshes = read_moby_submeshes(src, header.submesh_table_offset + header.submesh_count * 0x10, header.low_lod_submesh_count, format);
		s64 metal_table_ofs = header.submesh_table_offset + header.metal_submesh_begin * 0x10;
		moby.metal_submeshes = read_moby_metal_submeshes(src, metal_table_ofs, header.metal_submesh_count);
		if(header.bangles != 0) {
			MobyBangle& first_bangle = moby.bangles->bangles.at(0);
			s64 bangles_submesh_table_ofs = header.submesh_table_offset + first_bangle.submesh_begin * 0x10;
			moby.bangles->submeshes = read_moby_submeshes(src, bangles_submesh_table_ofs, first_bangle.submesh_count, format);
			mystery_data_ofs = std::max(mystery_data_ofs, bangles_submesh_table_ofs + first_bangle.submesh_count * 0x10);
		} else {
			mystery_data_ofs = std::max(mystery_data_ofs, metal_table_ofs + header.metal_submesh_count * 0x10);
		}
	}
	if(header.skeleton != 0) {
		moby.mystery_data = src.read_bytes(mystery_data_ofs, header.skeleton - mystery_data_ofs, "moby mystery data");
	}
	if(header.rac3dl_team_textures != 0 && (game == Game::RAC3 || game == Game::DL)) {
		verify(header.gif_usage != 0, "Moby with team palettes but no gif table.");
		moby.palettes_per_texture = header.rac3dl_team_textures & 0xf;
		s32 texture_count = (header.rac3dl_team_textures & 0xf0) >> 4;
		for(s32 i = moby.palettes_per_texture * texture_count; i > 0; i--) {
			std::array<u32, 256>& dest = moby.team_palettes.emplace_back();
			auto palette = src.read_multiple<u8>(header.gif_usage - i * 1024, 1024, "team palette");
			memcpy(dest.data(), palette.lo, 1024);
		}
	}
	return moby;
}

static s64 class_header_ofs;

void write_moby_class(OutBuffer dest, const MobyClassData& moby, Game game) {
	MobyClassHeader header = {0};
	class_header_ofs = dest.alloc<MobyClassHeader>();
	assert(class_header_ofs % 0x40 == 0);
	
	MobyFormat format;
	switch(game) {
		case Game::RAC1:
			format = MobyFormat::RAC1;
			break;
		case Game::RAC2:
			format = moby.force_rac1_format > 0 ? MobyFormat::RAC1 : MobyFormat::RAC2;
			break;
		case Game::RAC3:
		case Game::DL:
			format = MobyFormat::RAC3DL;
			break;
		default:
			assert_not_reached("Bad game enum.");
	}
	
	assert(!moby.has_submesh_table | (moby.submeshes.size() == moby.submesh_count));
	verify(moby.submeshes.size() < 256, "Moby class has too many submeshes.");
	header.submesh_count = moby.submesh_count;
	assert(!moby.has_submesh_table | (moby.low_lod_submeshes.size() == moby.low_lod_submesh_count));
	verify(moby.low_lod_submeshes.size() < 256, "Moby class has too many low detail submeshes.");
	header.low_lod_submesh_count = moby.low_lod_submesh_count;
	assert(!moby.has_submesh_table | (moby.metal_submeshes.size() == moby.metal_submesh_count));
	verify(moby.metal_submeshes.size() < 256, "Moby class has too many metal submeshes.");
	header.metal_submesh_count = moby.metal_submesh_count;
	header.metal_submesh_begin = moby.submesh_count + moby.low_lod_submesh_count;
	if(format == MobyFormat::RAC1) {
		header.rac1_byte_a = moby.rac1_byte_a;
		header.rac12_byte_b = moby.rac1_byte_b;
	}
	header.joint_count = moby.joint_count;
	header.unknown_9 = moby.unknown_9;
	header.lod_trans = moby.lod_trans;
	header.shadow = moby.shadow;
	header.scale = moby.scale;
	verify(moby.sound_defs.size() < 256, "Moby class has too many sounds.");
	header.sound_count = moby.sound_defs.size();
	header.mip_dist = moby.mip_dist;
	header.bounding_sphere = Vec4f::pack(moby.bounding_sphere);
	header.glow_rgba = moby.glow_rgba;
	header.mode_bits = moby.mode_bits;
	header.type = moby.type;
	header.mode_bits2 = moby.mode_bits2;
	
	verify(moby.sequences.size() < 256, "Moby class has too many sequences (max is 255).");
	header.sequence_count = moby.sequences.size();
	s64 sequence_list_ofs = dest.alloc_multiple<s32>(moby.sequences.size());
	while(dest.tell() - class_header_ofs < moby.header_end_offset) {
		dest.write<u8>(0);
	}
	if(moby.bangles.has_value()) {
		dest.pad(0x10);
		header.bangles = (write_moby_bangles(dest, *moby.bangles) - class_header_ofs) / 0x10;
	}
	if(game == Game::RAC1) {
		header.corncob = moby.rac1_short_2e;
	} else if(moby.corncob.has_value()) {
		dest.pad(0x10);
		header.corncob = (write_moby_corncob(dest, *moby.corncob) - class_header_ofs) / 0x10;
	}
	dest.pad(0x10);
	write_moby_sequences(dest, moby.sequences, sequence_list_ofs, moby.joint_count, game);
	dest.pad(0x10);
	while(dest.tell() < class_header_ofs + moby.submesh_table_offset) {
		dest.write<u8>(0);
	}
	s64 submesh_table_1_ofs = dest.alloc_multiple<MobySubMeshEntry>(moby.submeshes.size());
	s64 submesh_table_2_ofs = dest.alloc_multiple<MobySubMeshEntry>(moby.low_lod_submeshes.size());
	s64 metal_submesh_table_ofs = dest.alloc_multiple<MobySubMeshEntry>(moby.metal_submeshes.size());
	s64 bangles_submesh_table_ofs = 0;
	if(moby.bangles.has_value()) {
		bangles_submesh_table_ofs = dest.alloc_multiple<MobySubMeshEntry>(moby.bangles->submeshes.size());
	}
	if(moby.has_submesh_table) {
		header.submesh_table_offset = submesh_table_1_ofs - class_header_ofs;
	}
	if(moby.collision.has_value()) {
		header.collision = write_moby_collision(dest, *moby.collision) - class_header_ofs;
	}
	dest.write_multiple(moby.mystery_data);
	if(moby.skeleton.has_value()) {
		header.skeleton = dest.tell() - class_header_ofs;
		verify(moby.skeleton->size() < 255, "Moby class has too many joints.");
		dest.write_multiple(*moby.skeleton);
	}
	dest.pad(0x10);
	if(moby.common_trans.has_value()) {
		header.common_trans = dest.write_multiple(*moby.common_trans) - class_header_ofs;
	}
	header.joints = write_moby_joints(dest, moby.joints) - class_header_ofs;
	dest.pad(0x10);
	if(moby.sound_defs.size() > 0) {
		header.sound_defs = dest.write_multiple(moby.sound_defs) - class_header_ofs;
	}
	std::vector<MobyGifUsageTableEntry> gif_usage;
	write_moby_submeshes(dest, gif_usage, submesh_table_1_ofs, moby.submeshes, format);
	write_moby_submeshes(dest, gif_usage, submesh_table_2_ofs, moby.low_lod_submeshes, format);
	write_moby_metal_submeshes(dest, metal_submesh_table_ofs, moby.metal_submeshes);
	if(moby.bangles.has_value()) {
		write_moby_submeshes(dest, gif_usage, bangles_submesh_table_ofs, moby.bangles->submeshes, format);
	}
	if(moby.team_palettes.size() > 0 && (game == Game::RAC3 || game == Game::DL)) {
		dest.pad(0x10);
		s64 team_palettes_ofs = dest.tell();
		dest.write<u64>(0);
		dest.write<u64>(0);
		for(const std::array<u32, 256>& palette : moby.team_palettes) {
			dest.write_multiple(palette);
		}
		verify(moby.palettes_per_texture < 16, "Too many team palettes per texture (max is 15).");
		verify(moby.palettes_per_texture != 0, "Palettes per texture is zero.");
		s32 texture_count = moby.team_palettes.size() / moby.palettes_per_texture;
		verify(texture_count < 16, "Too many team textures (max is 15).");
		header.rac3dl_team_textures = moby.palettes_per_texture | (texture_count << 4);
		verify(gif_usage.size() > 0, "Team textures on a moby without a gif table.");
	}
	if(gif_usage.size() > 0) {
		gif_usage.back().offset_and_terminator |= 0x80000000;
		header.gif_usage = dest.write_multiple(gif_usage) - class_header_ofs;
	}
	dest.write(class_header_ofs, header);
}

static MobyBangles read_moby_bangles(Buffer src) {
	MobyBangles bangles;
	bangles.bangles = src.read_multiple<MobyBangle>(0, 16, "moby bangles").copy();
	s32 bangle_count = 0;
	for(MobyBangle& bangle : bangles.bangles) {
		if(bangle.submesh_begin != 0 || bangle.submesh_begin != 0) {
			bangle_count++;
		}
	}
	bangles.vertices = src.read_multiple<MobyVertexPosition>(0x40, 2 * (bangle_count - 1), "moby bangle vertices").copy();
	return bangles;
}

static s64 write_moby_bangles(OutBuffer dest, const MobyBangles& bangles) {
	s64 ofs = dest.tell();
	dest.write_multiple(bangles.bangles);
	dest.write_multiple(bangles.vertices);
	return ofs;
}

static MobyCornCob read_moby_corncob(Buffer src) {
	MobyCornCob corncob;
	auto header = src.read<MobyCornCobHeader>(0, "moby corncob");
	for(s32 i = 0; i < 16; i++) {
		if(header.kernels[i] != 0xff) {
			MobyCornKernel kernel;
			s64 kernel_ofs = header.kernels[i] * 0x10;
			kernel.vec = src.read<Vec4f>(kernel_ofs, "corn vec4");
			if(src.read<u64>(kernel_ofs, "corn") != 0 || src.read<u64>(kernel_ofs + 8, "corn") != 0) {
				s16 vertex_count = src.read<s16>(kernel_ofs + 0x16, "corn vertex count");
				kernel.vertices = src.read_multiple<MobyVertexPosition>(kernel_ofs + 0x10, vertex_count, "corn vertices").copy();
			}
			corncob.kernels[i] = std::move(kernel);
		}
	}
	return corncob;
}

static s64 write_moby_corncob(OutBuffer dest, const MobyCornCob& corncob) {
	s64 header_ofs = dest.alloc<MobyCornCobHeader>();
	MobyCornCobHeader header;
	for(s32 i = 0; i < 16; i++) {
		if(corncob.kernels[i].has_value()) {
			const MobyCornKernel& kernel = *corncob.kernels[i];
			dest.pad(0x10);
			s64 kernel_ofs = dest.tell();
			dest.write(kernel.vec);
			dest.write_multiple(kernel.vertices);
			if(kernel.vertices.size() > 0) {
				dest.write<s16>(kernel_ofs + 0x16, (s16) kernel.vertices.size());
			}
			header.kernels[i] = (kernel_ofs - header_ofs) / 0x10;
			
		} else {
			header.kernels[i] = 0xff;
		}
	}
	dest.write(header_ofs, header);
	return header_ofs;
}

static std::vector<Opt<MobySequence>> read_moby_sequences(Buffer src, s64 sequence_count, s32 joint_count, Game game) {
	std::vector<Opt<MobySequence>> sequences;
	auto sequence_offsets = src.read_multiple<s32>(sizeof(MobyClassHeader), sequence_count, "moby sequences");
	for(s32 seq_offset : sequence_offsets) {
		if(seq_offset == 0) {
			sequences.emplace_back(std::nullopt);
			continue;
		}
		
		sequences.emplace_back(read_moby_sequence(src, seq_offset, joint_count, game));
	}
	return sequences;
}

static void write_moby_sequences(OutBuffer dest, const std::vector<Opt<MobySequence>>& sequences, s64 list_ofs, s32 joint_count, Game game) {
	for(const Opt<MobySequence>& sequence_opt : sequences) {
		if(!sequence_opt.has_value()) {
			dest.write<s32>(list_ofs, 0);
			list_ofs += 4;
			continue;
		}
		
		const MobySequence& sequence = *sequence_opt;
		s64 seq_ofs = write_moby_sequence(dest, sequence, class_header_ofs, joint_count, game);
		dest.write<u32>(list_ofs, seq_ofs - class_header_ofs);
		list_ofs += 4;
	}
}

MobySequence read_moby_sequence(Buffer src, s64 seq_ofs, s32 joint_count, Game game) {
	auto seq_header = src.read<MobySequenceHeader>(seq_ofs, "moby sequence header");
	MobySequence sequence;
	sequence.bounding_sphere = seq_header.bounding_sphere.unpack();
	sequence.animation_info = seq_header.animation_info;
	sequence.sound_count = seq_header.sound_count;
	sequence.unknown_13 = seq_header.unknown_13;
	
	auto frame_table = src.read_multiple<s32>(seq_ofs + 0x1c, seq_header.frame_count, "moby sequence table");
	for(s32 frame_ofs_and_flag : frame_table) {
		if((frame_ofs_and_flag & 0xf0000000) != 0) {
			sequence.has_special_data = true;
		}
	}
	
	s64 after_frame_list = seq_ofs + 0x1c + seq_header.frame_count * 4;
	sequence.triggers = src.read_multiple<u32>(after_frame_list, seq_header.trigger_count, "moby sequence trigger list").copy();
	s64 after_trigger_list = after_frame_list + seq_header.trigger_count * 4;
	
	if(!sequence.has_special_data) { // Normal case.
		for(s32 frame_ofs_and_flag : frame_table) {
			MobyFrame frame;
			s32 flag = frame_ofs_and_flag & 0xf0000000;
			s32 frame_ofs = frame_ofs_and_flag & 0x0fffffff;
			
			auto frame_header = src.read<MobyFrameHeader>(frame_ofs, "moby frame header");
			frame.regular.unknown_0 = frame_header.unknown_0;
			frame.regular.unknown_4 = frame_header.unknown_4;
			frame.regular.unknown_c = frame_header.unknown_c;
			s32 data_ofs = frame_ofs + 0x10;
			frame.regular.joint_data = src.read_multiple<u64>(data_ofs, joint_count, "frame thing 1").copy();
			data_ofs += joint_count * 8;
			frame.regular.thing_1 = src.read_multiple<u64>(data_ofs, frame_header.thing_1_count, "frame thing 1").copy();
			data_ofs += frame_header.thing_1_count * 8;
			frame.regular.thing_2 = src.read_multiple<u64>(data_ofs, frame_header.thing_2_count, "frame thing 2").copy();
			
			s64 end_of_frame = frame_ofs + 0x10 + frame_header.data_size_qwords * 0x10;
			mystery_data_ofs = std::max(mystery_data_ofs, end_of_frame);
			sequence.frames.emplace_back(std::move(frame));
		}
		
		sequence.triggers = src.read_multiple<u32>(after_frame_list, seq_header.trigger_count, "moby sequence trigger list").copy();
	} else { // For Ratchet and a handful of other mobies.
		u32 packed_vals = src.read<u32>(after_trigger_list, "special anim data offsets");
		u32 second_part_ofs = 4 + ((packed_vals & 0b00000000000000000000001111111111) >> 0);
		u32 third_part_ofs = 4 + ((packed_vals & 0b00000000000111111111110000000000) >> 10);
		u32 fourth_part_ofs = 4 + (((packed_vals & 0b11111111111000000000000000000000) >> 21));
		
		sequence.special.joint_data = src.read_multiple<u16>(after_trigger_list + 4, joint_count * 3, "").copy();
		s64 thing_ofs = after_trigger_list + 4 + joint_count * 6;
		
		u8 thing_1_count = src.read<u8>(thing_ofs + 0, "special anim data thing 1 count");
		u8 thing_2_count = src.read<u8>(thing_ofs + 1, "special anim data thing 2 count");
		sequence.special.thing_1 = src.read_multiple<u64>(thing_ofs + 2, thing_1_count, "special anim data thing 1").copy();
		s64 thing_2_ofs = thing_ofs + 2 + thing_1_count * 8;
		sequence.special.thing_2 = src.read_multiple<u64>(thing_2_ofs, thing_2_count, "special anim data thing 2").copy();
		
		for(s32 frame_ofs_and_flag : frame_table) {
			s32 frame_ofs = frame_ofs_and_flag & 0xfffffff;
			
			MobyFrame frame;
			
			frame.special.inverse_unknown_0 = src.read<u16>(frame_ofs, "special anim data unknown 0");
			frame.special.unknown_4 = src.read<u16>(frame_ofs + 2, "special anim data unknown 1");
			frame.special.first_part = src.read_multiple<u8>(frame_ofs + 4, second_part_ofs - 4, "special anim data first part").copy();
			s64 second_part_size = third_part_ofs - second_part_ofs;
			frame.special.second_part = src.read_multiple<u8>(frame_ofs + second_part_ofs, second_part_size, "special anim data second part").copy();
			s64 third_part_size = fourth_part_ofs - third_part_ofs;
			frame.special.third_part = src.read_multiple<u8>(frame_ofs + third_part_ofs, third_part_size, "special anim data third part").copy();
			
			s32 fourth_part_size = joint_count;
			while(fourth_part_size % 8 != 0) fourth_part_size++;
			fourth_part_size /= 8;
			frame.special.fourth_part = src.read_multiple<u8>(frame_ofs + fourth_part_ofs, fourth_part_size, "special anim data fourth part").copy();
			s64 ofs = frame_ofs + fourth_part_ofs + fourth_part_size;
			
			auto read_fifth_part = [&](s32 count) {
				std::vector<u8> part;
				for(s32 i = 0; i < count; i++) {
					u8 packed_flag = src.read<u8>(ofs++, "special anim data flag");
					part.push_back(packed_flag);
					s32 flag_1 = ((packed_flag & 0b00000011) >> 0);
					if(flag_1 == 3) flag_1 = 0;
					for(s32 j = 0; j < flag_1; j++) {
						part.push_back(src.read<u8>(ofs++, "special anim data fifth part"));
					}
					s32 flag_2 = ((packed_flag & 0b00001100) >> 2);
					if(flag_2 == 3) flag_2 = 0;
					for(s32 j = 0; j < flag_2; j++) {
						part.push_back(src.read<u8>(ofs++, "special anim data fifth part"));
					}
					s32 flag_3 = ((packed_flag & 0b00110000) >> 4);
					if(flag_3 == 3) flag_3 = 0;
					for(s32 j = 0; j <flag_3; j++) {
						part.push_back(src.read<u8>(ofs++, "special anim data fifth part"));
					}
				}
				return part;
			};
			
			frame.special.fifth_part_1 = read_fifth_part(thing_1_count);
			frame.special.fifth_part_2 = read_fifth_part(thing_2_count);
			
			mystery_data_ofs = std::max(mystery_data_ofs, ofs);
			sequence.frames.emplace_back(std::move(frame));
		}
	}
	
	if(seq_header.triggers != 0) {
		s64 trigger_data_ofs = seq_ofs + seq_header.triggers;
		if(game == Game::RAC1) {
			trigger_data_ofs = seq_header.triggers;
		} else {
			trigger_data_ofs = seq_ofs + seq_header.triggers;
		}
		sequence.trigger_data = src.read<MobyTriggerData>(trigger_data_ofs, "moby sequence trigger data");
	}
	
	return sequence;
}

s64 write_moby_sequence(OutBuffer dest, const MobySequence& sequence, s64 header_ofs, s32 joint_count, Game game) {
	dest.pad(0x10);
	s64 seq_header_ofs = dest.alloc<MobySequenceHeader>();
	
	MobySequenceHeader seq_header = {0};
	seq_header.bounding_sphere = Vec4f::pack(sequence.bounding_sphere);
	seq_header.frame_count = sequence.frames.size();
	seq_header.sound_count = sequence.sound_count;
	seq_header.trigger_count = sequence.triggers.size();
	seq_header.unknown_13 = sequence.unknown_13;
	
	s64 frame_pointer_ofs = dest.alloc_multiple<s32>(sequence.frames.size());
	dest.write_multiple(sequence.triggers);
	
	if(sequence.has_special_data) {
		s32 first_part_size = 0;
		s32 second_part_size = 0;
		s32 third_part_size = 0;
		
		if(sequence.frames.size() >= 1) {
			const MobyFrame& frame = sequence.frames[0];
			first_part_size = (s32) frame.special.first_part.size();
			second_part_size = (s32) frame.special.second_part.size();
			third_part_size = (s32) frame.special.third_part.size();
		}
		
		u32 second_part_ofs = first_part_size;
		u32 third_part_ofs = second_part_ofs + second_part_size;
		u32 fourth_part_ofs = third_part_ofs + third_part_size;
		verify(second_part_ofs <= 0b1111111111, "Animation frame too big.");
		verify(third_part_ofs <= 0b11111111111, "Animation frame too big.");
		verify(fourth_part_ofs <= 0b11111111111, "Animation frame too big.");
		dest.write<u32>(second_part_ofs | (third_part_ofs << 10) | (fourth_part_ofs << 21));
		
		dest.pad(0x2);
		dest.write_multiple(sequence.special.joint_data);
		
		verify(sequence.special.thing_1.size() < 256, "Animation frame too big.");
		verify(sequence.special.thing_2.size() < 256, "Animation frame too big.");
		dest.write<u8>(sequence.special.thing_1.size());
		dest.write<u8>(sequence.special.thing_2.size());
		dest.write_multiple(sequence.special.thing_1);
		dest.write_multiple(sequence.special.thing_2);
	}
	
	if(sequence.trigger_data.has_value()) {
		if(game == Game::RAC1) {
			dest.pad(0x10);
		}
		s64 trigger_data_ofs = dest.write(*sequence.trigger_data);
		if(game == Game::RAC1) {
			seq_header.triggers = trigger_data_ofs - header_ofs;
		} else {
			seq_header.triggers = trigger_data_ofs - seq_header_ofs;
		}
	}
	seq_header.animation_info = sequence.animation_info;
	
	for(const MobyFrame& frame : sequence.frames) {
		if(!sequence.has_special_data) {
			s32 data_size_bytes = (joint_count + frame.regular.thing_1.size() + frame.regular.thing_2.size()) * 8;
			while(data_size_bytes % 0x10 != 0) data_size_bytes++;
			
			MobyFrameHeader frame_header = {0};
			frame_header.unknown_0 = frame.regular.unknown_0;
			frame_header.unknown_4 = frame.regular.unknown_4;
			verify(data_size_bytes / 0x10 < 65536, "Frame data too big.");
			frame_header.data_size_qwords = data_size_bytes / 0x10;
			frame_header.joint_data_size = joint_count * 8;
			verify(frame.regular.thing_1.size() < 65536, "Frame data too big.");
			frame_header.thing_1_count = (u16) frame.regular.thing_1.size();
			frame_header.unknown_c = frame.regular.unknown_c;
			verify(frame.regular.thing_2.size() < 65536, "Frame data too big.");
			frame_header.thing_2_count = (u16) frame.regular.thing_2.size();
			dest.pad(0x10);
			dest.write<u32>(frame_pointer_ofs, (dest.write(frame_header) - header_ofs));
			dest.write_multiple(frame.regular.joint_data);
			dest.write_multiple(frame.regular.thing_1);
			dest.write_multiple(frame.regular.thing_2);
		} else {
			dest.pad(0x4);
			dest.write<u32>(frame_pointer_ofs, (dest.tell() - header_ofs) | 0xf0000000);
			
			dest.write<u16>(frame.special.inverse_unknown_0);
			dest.write<u16>(frame.special.unknown_4);
			dest.write_multiple(frame.special.first_part);
			dest.write_multiple(frame.special.second_part);
			dest.write_multiple(frame.special.third_part);
			dest.write_multiple(frame.special.fourth_part);
			dest.write_multiple(frame.special.fifth_part_1);
			dest.write_multiple(frame.special.fifth_part_2);
		}
		frame_pointer_ofs += 4;
	}
	dest.write(seq_header_ofs, seq_header);
	
	return seq_header_ofs;
}

static MobyCollision read_moby_collision(Buffer src) {
	auto header = src.read<MobyCollisionHeader>(0, "moby collision header");
	MobyCollision collision;
	collision.unknown_0 = header.unknown_0;
	collision.unknown_2 = header.unknown_2;
	s64 ofs = 0x10;
	collision.first_part = src.read_bytes(ofs, header.first_part_size, "moby collision data");
	ofs += header.first_part_size;
	verify(header.second_part_size % 8 == 0, "Bad moby collision.");
	auto second_part = src.read_multiple<s16>(ofs, header.second_part_size / 2, "moby collision second part");
	ofs += header.second_part_size;
	for(s64 i = 0; i < second_part.size() / 4; i++) {
		Vec3f vec;
		vec.x = second_part[i * 4 + 0] / 1024.f;
		vec.y = second_part[i * 4 + 1] / 1024.f;
		vec.z = second_part[i * 4 + 2] / 1024.f;
		collision.second_part.push_back(vec);
	}
	collision.third_part = src.read_bytes(ofs, header.third_part_size, "moby collision third part");
	return collision;
}

static s64 write_moby_collision(OutBuffer dest, const MobyCollision& collision) {
	MobyCollisionHeader header;
	header.unknown_0 = collision.unknown_0;
	header.unknown_2 = collision.unknown_2;
	header.first_part_size = collision.first_part.size();
	header.third_part_size = collision.third_part.size();
	header.second_part_size = collision.second_part.size() * 8;
	dest.pad(0x10);
	s64 ofs = dest.write(header);
	dest.write_multiple(collision.first_part);
	for(const Vec3f& vec : collision.second_part) {
		dest.write<s16>(vec.x * 1024.f);
		dest.write<s16>(vec.y * 1024.f);
		dest.write<s16>(vec.z * 1024.f);
		dest.write<s16>(0);
	}
	dest.write_multiple(collision.third_part);
	return ofs;
}

static std::vector<MobyJointEntry> read_moby_joints(Buffer src, s64 joints_ofs) {
	std::vector<MobyJointEntry> joints;
	s32 list_count = src.read<s32>(joints_ofs, "joint list count");
	for(s32 i = 0; i < list_count; i++) {
		MobyJointEntry joint;
		s32 list_ofs = src.read<s32>(joints_ofs + (i + 1) * 4, "joint list");
		s16 thing_one_count = src.read<s16>(list_ofs, "joint count 1");
		list_ofs += 2;
		s16 thing_two_count = src.read<s16>(list_ofs, "joint count 2");
		list_ofs += 2;
		joint.thing_one = src.read_multiple<u8>(list_ofs, thing_one_count, "joint thing ones").copy();
		list_ofs += thing_one_count;
		joint.thing_two = src.read_multiple<u8>(list_ofs, thing_two_count, "joint thing twos").copy();
		list_ofs += thing_two_count;
		verify(src.read<u8>(list_ofs, "joint list terminator") == 0xff, "Bad joint data.");
		joints.emplace_back(std::move(joint));
	}
	return joints;
}

static s64 write_moby_joints(OutBuffer dest, const std::vector<MobyJointEntry>& joints) {
	dest.pad(0x10);
	s64 base_ofs = dest.tell();
	dest.write<s32>(joints.size());
	s64 outer_list_ofs = dest.alloc_multiple<s32>(joints.size());
	for(const MobyJointEntry& joint : joints) {
		dest.pad(0x4);
		dest.write<s32>(outer_list_ofs, dest.tell() - class_header_ofs);
		outer_list_ofs += 4;
		dest.write<s16>(joint.thing_one.size());
		dest.write<s16>(joint.thing_two.size());
		dest.write_multiple(joint.thing_one);
		dest.write_multiple(joint.thing_two);
		dest.write<u8>(0xff);
	}
	return base_ofs;
}

static std::vector<MobySubMesh> read_moby_submeshes(Buffer src, s64 table_ofs, s64 count, MobyFormat format) {
	std::vector<MobySubMesh> submeshes;
	for(auto& entry : src.read_multiple<MobySubMeshEntry>(table_ofs, count, "moby submesh table")) {
		MobySubMesh submesh;
		
		// Read VIF command list.
		Buffer command_buffer = src.subbuf(entry.vif_list_offset, entry.vif_list_size * 0x10);
		auto command_list = read_vif_command_list(command_buffer);
		auto unpacks = filter_vif_unpacks(command_list);
		Buffer st_data(unpacks.at(0).data);
		submesh.sts = st_data.read_multiple<MobyTexCoord>(0, st_data.size() / 4, "moby st unpack").copy();
		
		Buffer index_data(unpacks.at(1).data);
		auto index_header = index_data.read<MobyIndexHeader>(0, "moby index unpack header");
		submesh.index_header_first_byte = index_header.unknown_0;
		verify(index_header.pad == 0, "Moby has bad index buffer.");
		submesh.secret_indices.push_back(index_header.secret_index);
		submesh.indices = index_data.read_bytes(4, index_data.size() - 4, "moby index unpack data");
		if(unpacks.size() >= 3) {
			Buffer texture_data(unpacks.at(2).data);
			verify(texture_data.size() % 0x40 == 0, "Moby has bad texture unpack.");
			for(size_t i = 0; i < texture_data.size() / 0x40; i++) {
				submesh.secret_indices.push_back(texture_data.read<s32>(i * 0x10 + 0xc, "extra index"));
				auto prim = texture_data.read<MobyTexturePrimitive>(i * 0x40, "moby texture primitive");
				verify(prim.d3_tex0.data_lo >= MOBY_TEX_NONE, "Regular moby submesh has a texture index that is too low.");
				submesh.textures.push_back(prim);
			}
		}
		
		// Read vertex table.
		MobyVertexTableHeaderRac1 vertex_header;
		s64 array_ofs = entry.vertex_offset;
		if(format == MobyFormat::RAC1) {
			vertex_header = src.read<MobyVertexTableHeaderRac1>(entry.vertex_offset, "moby vertex header");
			array_ofs += sizeof(MobyVertexTableHeaderRac1);
		} else {
			auto compact_vertex_header = src.read<MobyVertexTableHeaderRac23DL>(entry.vertex_offset, "moby vertex header");
			vertex_header.matrix_transfer_count = compact_vertex_header.matrix_transfer_count;
			vertex_header.two_way_blend_vertex_count = compact_vertex_header.two_way_blend_vertex_count;
			vertex_header.three_way_blend_vertex_count = compact_vertex_header.three_way_blend_vertex_count;
			vertex_header.main_vertex_count = compact_vertex_header.main_vertex_count;
			vertex_header.duplicate_vertex_count = compact_vertex_header.duplicate_vertex_count;
			vertex_header.transfer_vertex_count = compact_vertex_header.transfer_vertex_count;
			vertex_header.vertex_table_offset = compact_vertex_header.vertex_table_offset;
			vertex_header.unknown_e = compact_vertex_header.unknown_e;
			array_ofs += sizeof(MobyVertexTableHeaderRac23DL);
		}
		if(vertex_header.vertex_table_offset / 0x10 > entry.vertex_data_size) {
			printf("warning: Bad vertex table offset or size.\n");
			continue;
		}
		if(entry.transfer_vertex_count != vertex_header.transfer_vertex_count) {
			printf("warning: Conflicting vertex counts.\n");
		}
		if(entry.unknown_d != (0xf + entry.transfer_vertex_count * 6) / 0x10) {
			printf("warning: Weird value in submodel table entry at field 0xd.\n");
			continue;
		}
		if(entry.unknown_e != (3 + entry.transfer_vertex_count) / 4) {
			printf("warning: Weird value in submodel table entry at field 0xe.\n");
			continue;
		}
		submesh.preloop_matrix_transfers = src.read_multiple<MobyMatrixTransfer>(array_ofs, vertex_header.matrix_transfer_count, "vertex table").copy();
		array_ofs += vertex_header.matrix_transfer_count * 2;
		if(array_ofs % 4 != 0) {
			array_ofs += 2;
		}
		if(array_ofs % 8 != 0) {
			array_ofs += 4;
		}
		for(u16 dupe : src.read_multiple<u16>(array_ofs, vertex_header.duplicate_vertex_count, "vertex table")) {
			submesh.duplicate_vertices.push_back(dupe >> 7);
		}
		s64 vertex_ofs = entry.vertex_offset + vertex_header.vertex_table_offset;
		s32 in_file_vertex_count = vertex_header.two_way_blend_vertex_count + vertex_header.three_way_blend_vertex_count + vertex_header.main_vertex_count;
		submesh.vertices = src.read_multiple<MobyVertex>(vertex_ofs, in_file_vertex_count, "vertex table").copy();
		vertex_ofs += in_file_vertex_count * 0x10;
		submesh.two_way_blend_vertex_count = vertex_header.two_way_blend_vertex_count;
		submesh.three_way_blend_vertex_count = vertex_header.three_way_blend_vertex_count;
		submesh.unknown_e = vertex_header.unknown_e;
		if(format == MobyFormat::RAC1) {
			s32 unknown_e_size = entry.vertex_data_size * 0x10 - vertex_header.unknown_e;
			submesh.unknown_e_data = src.read_bytes(entry.vertex_offset + vertex_header.unknown_e, unknown_e_size, "vertex table unknown_e data");
		}
		
		// Fix vertex indices (see comment in write_moby_submeshes).
		for(size_t i = 7; i < submesh.vertices.size(); i++) {
			submesh.vertices[i - 7].low_halfword = (submesh.vertices[i - 7].low_halfword & ~0x1ff) | (submesh.vertices[i].low_halfword & 0x1ff);
		}
		s32 trailing_vertex_count = 0;
		if(format == MobyFormat::RAC1) {
			trailing_vertex_count = (vertex_header.unknown_e - vertex_header.vertex_table_offset) / 0x10 - in_file_vertex_count;
		} else {
			trailing_vertex_count = entry.vertex_data_size - vertex_header.vertex_table_offset / 0x10 - in_file_vertex_count;
		}
		verify(trailing_vertex_count < 7, "Bad moby vertex table.");
		s64 trailing;
		vertex_ofs += std::max(7 - in_file_vertex_count, 0) * 0x10;
		for(s64 i = std::max(7 - in_file_vertex_count, 0); i < trailing_vertex_count; i++) {
			MobyVertex vertex = src.read<MobyVertex>(vertex_ofs, "vertex table");
			vertex_ofs += 0x10;
			s64 dest_index = in_file_vertex_count + i - 7;
			submesh.vertices.at(dest_index).low_halfword = (submesh.vertices[dest_index].low_halfword & ~0x1ff) | (vertex.low_halfword & 0x1ff);
		}
		MobyVertex last_vertex = src.read<MobyVertex>(vertex_ofs - 0x10, "vertex table");
		for(s32 i = std::max(7 - in_file_vertex_count - trailing_vertex_count, 0); i < 6; i++) {
			s64 dest_index = in_file_vertex_count + trailing_vertex_count + i - 7;
			if(dest_index < submesh.vertices.size()) {
				submesh.vertices[dest_index].low_halfword = (submesh.vertices[dest_index].low_halfword & ~0x1ff) | (last_vertex.trailing.vertex_indices[i] & 0x1ff);
			}
		}
		
		submeshes.emplace_back(std::move(submesh));
	}
	return submeshes;
}

static void write_moby_submeshes(OutBuffer dest, GifUsageTable& gif_usage, s64 table_ofs, const std::vector<MobySubMesh>& submeshes, MobyFormat format) {
	static const s32 ST_UNPACK_ADDR_QUADWORDS = 0xc2;
	
	for(const MobySubMesh& submesh : submeshes) {
		MobySubMeshEntry entry = {0};
		
		// Write VIF command list.
		dest.pad(0x10);
		s64 vif_list_ofs = dest.tell();
		entry.vif_list_offset = vif_list_ofs - class_header_ofs;
		
		VifPacket st_unpack;
		st_unpack.code.interrupt = 0;
		st_unpack.code.cmd = (VifCmd) 0b1110000; // UNPACK
		st_unpack.code.num = submesh.sts.size();
		st_unpack.code.unpack.vnvl = VifVnVl::V2_16;
		st_unpack.code.unpack.flg = VifFlg::USE_VIF1_TOPS;
		st_unpack.code.unpack.usn = VifUsn::SIGNED;
		st_unpack.code.unpack.addr = ST_UNPACK_ADDR_QUADWORDS;
		st_unpack.data.resize(submesh.sts.size() * 4);
		memcpy(st_unpack.data.data(), submesh.sts.data(), submesh.sts.size() * 4);
		write_vif_packet(dest, st_unpack);
		
		s64 tex_unpack = write_shared_moby_vif_packets(dest, &gif_usage, submesh);
		
		entry.vif_list_texture_unpack_offset = tex_unpack;
		dest.pad(0x10);
		entry.vif_list_size = (dest.tell() - vif_list_ofs) / 0x10;
		
		// Umm.. "adjust" vertex indices (see comment below).
		std::vector<MobyVertex> vertices = submesh.vertices;
		std::vector<u16> trailing_vertex_indices(std::max(7 - (s32) vertices.size(), 0), 0);
		for(s32 i = std::max((s32) vertices.size() - 7, 0); i < vertices.size(); i++) {
			trailing_vertex_indices.push_back(vertices[i].low_halfword & 0x1ff);
		}
		for(s32 i = vertices.size() - 1; i >= 7; i--) {
			vertices[i].low_halfword = (vertices[i].low_halfword & ~0x1ff) | (vertices[i - 7].low_halfword & 0xff);
		}
		for(s32 i = 0; i < std::min(7, (s32) vertices.size()); i++) {
			vertices[i].low_halfword = vertices[i].low_halfword & ~0x1ff;
		}
		
		// Write vertex table.
		s64 vertex_header_ofs;
		if(format == MobyFormat::RAC1) {
			vertex_header_ofs = dest.alloc<MobyVertexTableHeaderRac1>();
		} else {
			vertex_header_ofs = dest.alloc<MobyVertexTableHeaderRac23DL>();
		}
		MobyVertexTableHeaderRac1 vertex_header;
		vertex_header.matrix_transfer_count = submesh.preloop_matrix_transfers.size();
		vertex_header.two_way_blend_vertex_count = submesh.two_way_blend_vertex_count;
		vertex_header.three_way_blend_vertex_count = submesh.three_way_blend_vertex_count;
		vertex_header.main_vertex_count =
			submesh.vertices.size() -
			submesh.two_way_blend_vertex_count -
			submesh.three_way_blend_vertex_count;
		vertex_header.duplicate_vertex_count = submesh.duplicate_vertices.size();
		vertex_header.transfer_vertex_count =
			vertex_header.two_way_blend_vertex_count +
			vertex_header.three_way_blend_vertex_count +
			vertex_header.main_vertex_count +
			vertex_header.duplicate_vertex_count;
		vertex_header.unknown_e = submesh.unknown_e;
		dest.write_multiple(submesh.preloop_matrix_transfers);
		dest.pad(0x8);
		for(u16 dupe : submesh.duplicate_vertices) {
			dest.write<u16>(dupe << 7);
		}
		dest.pad(0x10);
		vertex_header.vertex_table_offset = dest.tell() - vertex_header_ofs;
		
		// Write out the remaining vertex indices after the rest of the proper
		// vertices (since the vertex index stored in each vertex corresponds to
		// the vertex 7 vertices prior for some reason). The remaining indices
		// are written out into the padding vertices and then when that space
		// runs out they're written into the second part of the last padding
		// vertex (hence there is at least one padding vertex). Now I see why
		// they call it Insomniac Games.
		s32 trailing = 0;
		for(; vertices.size() % 4 != 2 && trailing < trailing_vertex_indices.size(); trailing++) {
			MobyVertex vertex = {0};
			if(submesh.vertices.size() + trailing >= 7) {
				vertex.low_halfword = trailing_vertex_indices[trailing];
			}
			vertices.push_back(vertex);
		}
		assert(trailing < trailing_vertex_indices.size());
		MobyVertex last_vertex = {0};
		if(submesh.vertices.size() + trailing >= 7) {
			last_vertex.low_halfword = trailing_vertex_indices[trailing];
		}
		for(s32 i = trailing + 1; i < trailing_vertex_indices.size(); i++) {
			if(submesh.vertices.size() + i >= 7) {
				last_vertex.trailing.vertex_indices[i - trailing - 1] = trailing_vertex_indices[i];
			}
		}
		vertices.push_back(last_vertex);
		dest.write_multiple(vertices);
		
		if(format == MobyFormat::RAC1) {
			vertex_header.unknown_e = dest.tell() - vertex_header_ofs;
			dest.write_multiple(submesh.unknown_e_data);
			dest.write(vertex_header_ofs, vertex_header);
		} else {
			MobyVertexTableHeaderRac23DL compact_vertex_header;
			compact_vertex_header.matrix_transfer_count = vertex_header.matrix_transfer_count;
			compact_vertex_header.two_way_blend_vertex_count = vertex_header.two_way_blend_vertex_count;
			compact_vertex_header.three_way_blend_vertex_count = vertex_header.three_way_blend_vertex_count;
			compact_vertex_header.main_vertex_count = vertex_header.main_vertex_count;
			compact_vertex_header.duplicate_vertex_count = vertex_header.duplicate_vertex_count;
			compact_vertex_header.transfer_vertex_count = vertex_header.transfer_vertex_count;
			compact_vertex_header.vertex_table_offset = vertex_header.vertex_table_offset;
			compact_vertex_header.unknown_e = vertex_header.unknown_e;
			dest.write(vertex_header_ofs, compact_vertex_header);
		}
		entry.vertex_offset = vertex_header_ofs - class_header_ofs;
		dest.pad(0x10);
		entry.vertex_data_size = (dest.tell() - vertex_header_ofs) / 0x10;
		entry.unknown_d = (0xf + vertex_header.transfer_vertex_count * 6) / 0x10;
		entry.unknown_e = (3 + vertex_header.transfer_vertex_count) / 4;
		entry.transfer_vertex_count = vertex_header.transfer_vertex_count;
		
		vertex_header.unknown_e = 0;
		dest.pad(0x10);
		dest.write(table_ofs, entry);
		table_ofs += 0x10;
	}
}

static std::vector<MobyMetalSubMesh> read_moby_metal_submeshes(Buffer src, s64 table_ofs, s64 count) {
	std::vector<MobyMetalSubMesh> submeshes;
	for(auto& entry : src.read_multiple<MobySubMeshEntry>(table_ofs, count, "moby metal submesh table")) {
		MobyMetalSubMesh submesh;
		
		// Read VIF command list.
		Buffer command_buffer = src.subbuf(entry.vif_list_offset, entry.vif_list_size * 0x10);
		auto command_list = read_vif_command_list(command_buffer);
		auto unpacks = filter_vif_unpacks(command_list);
		Buffer index_data(unpacks.at(0).data);
		auto index_header = index_data.read<MobyIndexHeader>(0, "moby index unpack header");
		submesh.index_header_first_byte = index_header.unknown_0;
		verify(index_header.pad == 0, "Moby has bad index buffer.");
		submesh.secret_indices.push_back(index_header.secret_index);
		submesh.indices = index_data.read_bytes(4, index_data.size() - 4, "moby index unpack data");
		if(unpacks.size() >= 2) {
			Buffer texture_data(unpacks.at(1).data);
			verify(texture_data.size() % 0x40 == 0, "Moby has bad texture unpack.");
			for(size_t i = 0; i < texture_data.size() / 0x40; i++) {
				submesh.secret_indices.push_back(texture_data.read<s32>(i * 0x10 + 0xc, "extra index"));
				auto prim = texture_data.read<MobyTexturePrimitive>(i * 0x40, "moby texture primitive");
				verify(prim.d3_tex0.data_lo == MOBY_TEX_CHROME || prim.d3_tex0.data_lo == MOBY_TEX_GLASS,
					"Metal moby submesh has a bad texture index.");
				submesh.textures.push_back(prim);
			}
		}
		
		// Read vertex table.
		auto vertex_header = src.read<MobyMetalVertexTableHeader>(entry.vertex_offset, "metal vertex table header");
		submesh.vertices = src.read_multiple<MobyMetalVertex>(entry.vertex_offset + 0x10, vertex_header.vertex_count, "metal vertex table").copy();
		submesh.unknown_4 = vertex_header.unknown_4;
		submesh.unknown_8 = vertex_header.unknown_8;
		submesh.unknown_c = vertex_header.unknown_c;
		
		submeshes.emplace_back(std::move(submesh));
	}
	return submeshes;
}

static void write_moby_metal_submeshes(OutBuffer dest, s64 table_ofs, const std::vector<MobyMetalSubMesh>& submeshes) {
	for(const MobyMetalSubMesh& submesh : submeshes) {
		MobySubMeshEntry entry = {0};
		
		// Write VIF command list.
		dest.pad(0x10);
		s64 vif_list_ofs = dest.tell();
		entry.vif_list_offset = vif_list_ofs - class_header_ofs;
		s64 tex_unpack = write_shared_moby_vif_packets(dest, nullptr, submesh);
		entry.vif_list_texture_unpack_offset = tex_unpack;
		dest.pad(0x10);
		entry.vif_list_size = (dest.tell() - vif_list_ofs) / 0x10;
		
		// Write vertex table.
		MobyMetalVertexTableHeader vertex_header;
		vertex_header.vertex_count = submesh.vertices.size();
		vertex_header.unknown_4 = submesh.unknown_4;
		vertex_header.unknown_8 = submesh.unknown_8;
		vertex_header.unknown_c = submesh.unknown_c;
		s64 vertex_header_ofs = dest.write(vertex_header);
		dest.write_multiple(submesh.vertices);
		entry.vertex_offset = vertex_header_ofs - class_header_ofs;
		dest.pad(0x10);
		entry.vertex_data_size = (dest.tell() - vertex_header_ofs) / 0x10;
		entry.unknown_d = (0xf + vertex_header.vertex_count * 6) / 0x10;
		entry.unknown_e = (3 + vertex_header.vertex_count) / 4;
		entry.transfer_vertex_count = vertex_header.vertex_count;
		
		dest.write(table_ofs, entry);
		table_ofs += 0x10;
	}
}

static s64 write_shared_moby_vif_packets(OutBuffer dest, GifUsageTable* gif_usage, const MobySubMeshBase& submesh) {
	static const s32 INDEX_UNPACK_ADDR_QUADWORDS = 0x12d;
	
	std::vector<u8> indices;
	OutBuffer index_buffer(indices);
	s64 index_header_ofs = index_buffer.alloc<MobyIndexHeader>();
	index_buffer.write_multiple(submesh.indices);
	
	MobyIndexHeader index_header = {0};
	index_header.unknown_0 = submesh.index_header_first_byte;
	if(submesh.textures.size() > 0) {
		index_header.texture_unpack_offset_quadwords = indices.size() / 4;
	}
	if(submesh.secret_indices.size() >= 1) {
		index_header.secret_index = submesh.secret_indices[0];
	}
	index_buffer.write(index_header_ofs, index_header);
	
	VifPacket index_unpack;
	index_unpack.code.interrupt = 0;
	index_unpack.code.cmd = (VifCmd) 0b1100000; // UNPACK
	index_unpack.code.num = indices.size() / 4;
	index_unpack.code.unpack.vnvl = VifVnVl::V4_8;
	index_unpack.code.unpack.flg = VifFlg::USE_VIF1_TOPS;
	index_unpack.code.unpack.usn = VifUsn::SIGNED;
	index_unpack.code.unpack.addr = INDEX_UNPACK_ADDR_QUADWORDS;
	index_unpack.data = std::move(indices);
	write_vif_packet(dest, index_unpack);
	
	s64 rel_texture_unpack_ofs = 0;
	if(submesh.textures.size() > 0) {
		while(dest.tell() % 0x10 != 0xc) {
			dest.write<u8>(0);
		}
		
		VifPacket texture_unpack;
		texture_unpack.code.interrupt = 0;
		texture_unpack.code.cmd = (VifCmd) 0b1100000; // UNPACK
		texture_unpack.code.num = submesh.textures.size() * 4;
		texture_unpack.code.unpack.vnvl = VifVnVl::V4_32;
		texture_unpack.code.unpack.flg = VifFlg::USE_VIF1_TOPS;
		texture_unpack.code.unpack.usn = VifUsn::SIGNED;
		texture_unpack.code.unpack.addr = INDEX_UNPACK_ADDR_QUADWORDS + index_unpack.code.num;
		
		assert(submesh.secret_indices.size() >= submesh.textures.size());
		for(size_t i = 0; i < submesh.textures.size(); i++) {
			MobyTexturePrimitive primitive = submesh.textures[i];
			OutBuffer(texture_unpack.data).write(primitive);
		}
		for(size_t i = 1; i < submesh.secret_indices.size(); i++) {
			OutBuffer(texture_unpack.data).write((i - 1) * 0x10 + 0xc, submesh.secret_indices[i]);
		}
		s32 abs_texture_unpack_ofs = dest.tell();
		write_vif_packet(dest, texture_unpack);
		
		if(gif_usage != nullptr) {
			MobyGifUsageTableEntry gif_entry;
			gif_entry.offset_and_terminator = abs_texture_unpack_ofs - 0xc - class_header_ofs;
			s32 gif_index = 0;
			for(const MobyTexturePrimitive& prim : submesh.textures) {
				assert(gif_index < 12);
				gif_entry.texture_indices[gif_index++] = prim.d3_tex0.data_lo;
			}
			for(s32 i = gif_index; i < 12; i++) {
				gif_entry.texture_indices[i] = 0xff;
			}
			gif_usage->push_back(gif_entry);
		}
		
		dest.pad(0x10);
		rel_texture_unpack_ofs = (dest.tell() - abs_texture_unpack_ofs + 0x4) / 0x10;
	}
	
	return rel_texture_unpack_ofs;
}

ColladaScene recover_moby_class(const MobyClassData& moby, s32 o_class, s32 texture_count) {
	ColladaScene scene;
	
	// Used for when the texture index stored in a GS primitive is -1.
	Material& none = scene.materials.emplace_back();
	none.name = "none";
	none.colour = glm::vec4(1, 1, 1, 1);
	// Used for when there're more textures referenced than are listed in the
	// moby class table. This happens for R&C2 ship parts.
	Material& dummy = scene.materials.emplace_back();
	dummy.name = "dummy";
	dummy.colour = glm::vec4(0.5, 0.5, 0.5, 1);
	
	for(s32 texture = 0; texture < texture_count; texture++) {
		Material& mat = scene.materials.emplace_back();
		mat.name = "mat_" + std::to_string(texture);
		mat.texture = texture;
	}
	for(s32 texture = 0; texture < texture_count; texture++) {
		Material& chrome = scene.materials.emplace_back();
		chrome.name = "chrome_" + std::to_string(texture);
		chrome.texture = texture;
	}
	for(s32 texture = 0; texture < texture_count; texture++) {
		Material& glass = scene.materials.emplace_back();
		glass.name = "glass_" + std::to_string(texture);
		glass.texture = texture;
	}
	
	if(MOBY_EXPORT_SUBMESHES_SEPERATELY) {
		for(s32 i = 0; i < (s32) moby.submeshes.size(); i++) {
			std::string name = "high_lod_" + std::to_string(i);
			scene.meshes.emplace_back(recover_moby_mesh(moby.submeshes, name.c_str(), o_class, texture_count, moby.scale, i));
		}
		for(s32 i = 0; i < (s32) moby.low_lod_submeshes.size(); i++) {
			std::string name = "low_lod_" + std::to_string(i);
			scene.meshes.emplace_back(recover_moby_mesh(moby.low_lod_submeshes, name.c_str(), o_class, texture_count, moby.scale, i));
		}
		if(moby.bangles.has_value()) {
			for(s32 i = 0; i < (s32) moby.bangles->submeshes.size(); i++) {
				std::string name = "bangles_" + std::to_string(i);
				scene.meshes.emplace_back(recover_moby_mesh(moby.bangles->submeshes, name.c_str(), o_class, texture_count, moby.scale, i));
			}
		}
	} else {
		scene.meshes.emplace_back(recover_moby_mesh(moby.submeshes, "high_lod", o_class, texture_count, moby.scale, NO_SUBMESH_FILTER));
		scene.meshes.emplace_back(recover_moby_mesh(moby.low_lod_submeshes, "low_lod", o_class, texture_count, moby.scale, NO_SUBMESH_FILTER));
		if(moby.bangles.has_value()) {
			scene.meshes.emplace_back(recover_moby_mesh(moby.bangles->submeshes, "bangles", o_class, texture_count, moby.scale, NO_SUBMESH_FILTER));
		}
	}
	
	if(moby.joints.size() != 0) {
		scene.joints = recover_moby_joints(moby);
	}
	
	return scene;
}

#define VERIFY_SUBMESH(cond, message) verify(cond, "Moby class %d, submesh %d has bad " message ".", o_class, i);

static Mesh recover_moby_mesh(const std::vector<MobySubMesh>& submeshes, const char* name, s32 o_class, s32 texture_count, f32 scale, s32 submesh_filter) {
	Mesh mesh;
	mesh.name = name;
	mesh.flags = MESH_HAS_NORMALS | MESH_HAS_TEX_COORDS;
	
	Opt<BlendAttributes> blend_buffer[64]; // The game stores this in VU0 memory.
	Opt<MobyVertex> intermediate_buffer[512]; // The game stores this on the end of the VU1 chain.
	
	SubMesh dest;
	dest.material = 0;
	
	for(s32 i = 0; i < submeshes.size(); i++) {
		bool lift_submesh = !MOBY_EXPORT_SUBMESHES_SEPERATELY || submesh_filter == -1 || i == submesh_filter; // This is just for debugging.
		
		const MobySubMesh& src = submeshes[i];
		
		for(const MobyMatrixTransfer& transfer : src.preloop_matrix_transfers) {
			verify(transfer.vu0_dest_addr % 4 == 0, "Unaligned pre-loop joint address 0x%llx.", transfer.vu0_dest_addr);
			blend_buffer[transfer.vu0_dest_addr / 4] = BlendAttributes{1, {transfer.scratchpad_matrix_index}, {1.f}};
		}
		
		s32 vertex_base = mesh.vertices.size();
		
		for(size_t j = 0; j < src.vertices.size(); j++) {
			const MobyVertex& mv = src.vertices[j];
			
			BlendAttributes blend = recover_blend_attributes(blend_buffer, mv, j, src.two_way_blend_vertex_count, src.three_way_blend_vertex_count);
			
			auto& st = src.sts.at(mesh.vertices.size() - vertex_base);
			mesh.vertices.emplace_back(recover_vertex(mv, st, scale));
			
			intermediate_buffer[mv.low_halfword & 0x1ff] = mv;
		}
		
		for(u16 dupe : src.duplicate_vertices) {
			auto mv = intermediate_buffer[dupe];
			VERIFY_SUBMESH(mv.has_value(), "vertex table");
			
			auto& st = src.sts.at(mesh.vertices.size() - vertex_base);
			mesh.vertices.emplace_back(recover_vertex(*mv, st, scale));
		}
		
		s32 index_queue[3] = {0};
		s32 index_pos = 0;
		s32 max_index = 0;
		s32 texture_index = 0;
		bool reverse_winding_order = true;
		for(u8 index : src.indices) {
			VERIFY_SUBMESH(index != 0x80, "index buffer");
			if(index == 0) {
				// There's an extra index stored in the index header, in
				// addition to an index stored in some 0x10 byte texture unpack
				// blocks. When a texture is applied, the next index from this
				// list is used as the next vertex in the queue, but the
				// triangle with it as its last index is not actually drawn.
				u8 secret_index = src.secret_indices.at(texture_index);
				if(secret_index == 0) {
					if(lift_submesh) {
						VERIFY_SUBMESH(dest.faces.size() >= 3, "index buffer");
						// The VU1 microprogram has multiple vertices in flight
						// at a time, so we need to remove the ones that
						// wouldn't have been written to the GS packet.
						dest.faces.pop_back();
						dest.faces.pop_back();
						dest.faces.pop_back();
					}
					break;
				} else {
					index = secret_index + 0x80;
					if(dest.faces.size() > 0) {
						mesh.submeshes.emplace_back(std::move(dest));
					}
					dest = SubMesh();
					s32 texture = src.textures.at(texture_index).d3_tex0.data_lo;
					assert(texture >= -1);
					if(texture == -1) {
						dest.material = 0; // none
					} else if(texture >= texture_count) {
						dest.material = 1; // dummy
					} else {
						dest.material = 2 + texture; // mat[texture]
					}
					texture_index++;
				}
			}
			if(index < 0x80) {
				VERIFY_SUBMESH(vertex_base + index - 1 < mesh.vertices.size(), "index buffer");
				index_queue[index_pos] = vertex_base + index - 1;
				if(lift_submesh) {
					if(reverse_winding_order) {
						s32 v0 = index_queue[(index_pos + 3) % 3];
						s32 v1 = index_queue[(index_pos + 2) % 3];
						s32 v2 = index_queue[(index_pos + 1) % 3];
						dest.faces.emplace_back(v0, v1, v2);
					} else {
						s32 v0 = index_queue[(index_pos + 1) % 3];
						s32 v1 = index_queue[(index_pos + 2) % 3];
						s32 v2 = index_queue[(index_pos + 3) % 3];
						dest.faces.emplace_back(v0, v1, v2);
					}
				}
			} else {
				index_queue[index_pos] = vertex_base + index - 0x81;
			}
			max_index = std::max(max_index, index_queue[index_pos]);
			VERIFY_SUBMESH(index_queue[index_pos] < mesh.vertices.size(), "index buffer");
			index_pos = (index_pos + 1) % 3;
			reverse_winding_order = !reverse_winding_order;
		}
	}
	if(dest.faces.size() > 0) {
		mesh.submeshes.emplace_back(std::move(dest));
	}
	mesh = deduplicate_vertices(std::move(mesh));
	return mesh;
}

static std::vector<Joint> recover_moby_joints(const MobyClassData& moby) {
	assert(opt_size(moby.skeleton) == opt_size(moby.common_trans));
	
	std::vector<Joint> joints;
	joints.reserve(1 + opt_size(moby.skeleton));
	
	for(size_t i = 0; i < opt_size(moby.skeleton); i++) {
		const Mat4& skeleton = (*moby.skeleton)[i];
		const MobyTrans& trans = (*moby.common_trans)[i];
		Joint j;
		j.matrix = skeleton.unpack();
		j.matrix[0][3] *= moby.scale / 1024.f;
		j.matrix[1][3] *= moby.scale / 1024.f;
		j.matrix[2][3] *= moby.scale / 1024.f;
		j.matrix[3] = glm::vec4(0, 0, 0, 1);
		s32 parent;
		if(i > 0) {
			parent = trans.parent_offset / 0x40;
		} else {
			parent = -1;
		}
		verify(parent < (s32) joints.size(), "Bad moby joints.");
		add_joint(joints, j, parent);
	}
	
	return joints;
}

MobyClassData build_moby_class(const ColladaScene& scene) {
	const Mesh* high_lod_mesh = nullptr;
	const Mesh* low_lod_mesh = nullptr;
	for(const Mesh& mesh : scene.meshes) {
		if(mesh.name == "high_lod") {
			high_lod_mesh = &mesh;
		}
		if(mesh.name == "low_lod") {
			low_lod_mesh = &mesh;
		}
	}
	verify(high_lod_mesh, "Collada file doesn't contain a 'high_lod' node.");
	
	MobyClassData moby;
	moby.submeshes = build_moby_submeshes(*high_lod_mesh, scene.materials, 0.25);
	moby.submesh_count = moby.submeshes.size();
	if(low_lod_mesh) {
		moby.low_lod_submeshes = build_moby_submeshes(*low_lod_mesh, scene.materials, 0.25);
		moby.low_lod_submesh_count = moby.low_lod_submeshes.size();
	}
	moby.skeleton = {};
	moby.common_trans = {};
	moby.unknown_9 = 0;
	moby.lod_trans = 0x20;
	moby.shadow = 0;
	moby.scale = 0.25;
	moby.mip_dist = 0x8;
	moby.bounding_sphere = glm::vec4(0.f, 0.f, 0.f, 10.f); // Arbitrary for now.
	moby.glow_rgba = 0;
	moby.mode_bits = 0x5000;
	moby.type = 0;
	moby.mode_bits2 = 0;
	moby.header_end_offset = 0;
	moby.submesh_table_offset = 0;
	moby.rac1_byte_a = 0;
	moby.rac1_byte_b = 0;
	moby.rac1_short_2e = 0;
	moby.has_submesh_table = true;
	
	MobySequence dummy_seq;
	dummy_seq.bounding_sphere = glm::vec4(0.f, 0.f, 0.f, 10.f); // Arbitrary for now.
	dummy_seq.frames.emplace_back();
	moby.sequences.emplace_back(std::move(dummy_seq));
	
	return moby;
}

struct RichIndex {
	u32 index : 30;
	u32 restart : 1;
	u32 is_dupe : 1 = 0;
};

static std::vector<RichIndex> fake_tristripper(const std::vector<Face>& faces) {
	std::vector<RichIndex> indices;
	for(const Face& face : faces) {
		indices.push_back({(u32) face.v0, 1u});
		indices.push_back({(u32) face.v1, 1u});
		indices.push_back({(u32) face.v2, 0u});
	}
	return indices;
}


struct MidLevelTexture {
	s32 texture;
	s32 starting_index;
};

struct MidLevelVertex {
	s32 canonical;
	s32 tex_coord;
	s32 id = 0xff;
};

struct MidLevelDuplicateVertex {
	s32 index;
	s32 tex_coord;
};

// Intermediate data structure used so the submeshes can be built in two
// seperate passes.
struct MidLevelSubMesh {
	std::vector<MidLevelVertex> vertices;
	std::vector<RichIndex> indices;
	std::vector<MidLevelTexture> textures;
	std::vector<MidLevelDuplicateVertex> duplicate_vertices;
};

static std::vector<MobySubMesh> build_moby_submeshes(const Mesh& mesh, const std::vector<Material>& materials, f32 scale) {
	static const s32 MAX_SUBMESH_TEXTURE_COUNT = 4;
	static const s32 MAX_SUBMESH_STORED_VERTEX_COUNT = 97;
	static const s32 MAX_SUBMESH_TOTAL_VERTEX_COUNT = 0x7f;
	static const s32 MAX_SUBMESH_INDEX_COUNT = 196;
	
	std::vector<IndexMappingRecord> index_mappings(mesh.vertices.size());
	find_duplicate_vertices(index_mappings, mesh.vertices);
	
	f32 inverse_scale = 1024.f / scale;
	
	// *************************************************************************
	// First pass
	// *************************************************************************
	
	std::vector<MidLevelSubMesh> mid_submeshes;
	MidLevelSubMesh mid;
	s32 next_id = 0;
	for(s32 i = 0; i < (s32) mesh.submeshes.size(); i++) {
		const SubMesh& high = mesh.submeshes[i];
		
		auto indices = fake_tristripper(high.faces);
		if(indices.size() < 1) {
			continue;
		}
		
		const Material& material = materials.at(high.material);
		s32 texture = -1;
		if(material.name.size() > 4 && memcmp(material.name.data(), "mat_", 4) == 0) {
			texture = strtol(material.name.c_str() + 4, nullptr, 10);
		} else {
			fprintf(stderr, "Invalid material '%s'.", material.name.c_str());
			continue;
		}
		
		if(mid.textures.size() >= MAX_SUBMESH_TEXTURE_COUNT || mid.indices.size() >= MAX_SUBMESH_INDEX_COUNT) {
			mid_submeshes.emplace_back(std::move(mid));
			mid = MidLevelSubMesh{};
		}
		
		mid.textures.push_back({texture, (s32) mid.indices.size()});
		
		for(size_t j = 0; j < indices.size(); j++) {
			auto new_submesh = [&]() {
				mid_submeshes.emplace_back(std::move(mid));
				mid = MidLevelSubMesh{};
				// Handle splitting the strip up between moby submeshes.
				if(j - 2 >= 0) {
					if(!indices[j].restart) {
						j -= 3;
						indices[j + 1].restart = 1;
						indices[j + 2].restart = 1;
					} else if(!indices[j + 1].restart) {
						j -= 2;
						indices[j + 1].restart = 1;
						indices[j + 2].restart = 1;
					} else {
						j -= 1;
					}
				} else {
					// If we tried to start a tristrip at the end of the last
					// submesh but didn't push any non-restarting indices, go
					// back to the beginning of the strip.
					j = -1;
				}
			};
			
			RichIndex& r = indices[j];
			IndexMappingRecord& mapping = index_mappings[r.index];
			size_t canonical_index = r.index;
			//if(mapping.dedup_out_edge != -1) {
			//	canonical_index = mapping.dedup_out_edge;
			//}
			IndexMappingRecord& canonical = index_mappings[canonical_index];
			
			if(canonical.submesh != mid_submeshes.size()) {
				if(mid.vertices.size() >= MAX_SUBMESH_STORED_VERTEX_COUNT) {
					new_submesh();
					continue;
				}
				
				canonical.submesh = mid_submeshes.size();
				canonical.index = mid.vertices.size();
				
				mid.vertices.push_back({(s32) r.index, (s32) r.index});
			} else if(mapping.submesh != mid_submeshes.size()) {
				if(canonical.id == -1) {
					canonical.id = next_id++;
					mid.vertices.at(canonical.index).id = canonical.id;
				}
				mid.duplicate_vertices.push_back({canonical.id, (s32) r.index});
			}
			
			if(mid.indices.size() >= MAX_SUBMESH_INDEX_COUNT - 4) {
				new_submesh();
				continue;
			}
			
			mid.indices.push_back({(u32) canonical.index, r.restart, r.is_dupe});
		}
	}
	if(mid.indices.size() > 0) {
		mid_submeshes.emplace_back(std::move(mid));
	}
	
	// *************************************************************************
	// Second pass
	// *************************************************************************
	
	std::vector<MobySubMesh> low_submeshes;
	for(const MidLevelSubMesh& mid : mid_submeshes) {
		MobySubMesh low;
		
		for(const MidLevelVertex& vertex : mid.vertices) {
			const Vertex& high_vert = mesh.vertices[vertex.canonical];
			low.vertices.emplace_back(build_vertex(high_vert, vertex.id, inverse_scale));
			
			const glm::vec2& tex_coord = mesh.vertices[vertex.tex_coord].tex_coord;
			s16 s = tex_coord.x * (INT16_MAX / 8.f);
			s16 t = tex_coord.y * (INT16_MAX / 8.f);
			low.sts.push_back({s, t});
		}
		
		s32 texture_index = 0;
		for(size_t i = 0; i < mid.indices.size(); i++) {
			RichIndex cur = mid.indices[i];
			u8 out;
			if(cur.is_dupe) {
				out = mid.vertices.size() + cur.index;
			} else {
				out = cur.index;
			}
			if(texture_index < mid.textures.size() && mid.textures.at(texture_index).starting_index >= i) {
				assert(cur.restart);
				low.indices.push_back(0);
				low.secret_indices.push_back(out + 1);
				texture_index++;
			} else {
				low.indices.push_back(cur.restart ? (out + 0x81) : (out + 1));
			}
		}
		
		// These fake indices are required to signal to the microprogram that it
		// should terminate.
		low.indices.push_back(1);
		low.indices.push_back(1);
		low.indices.push_back(1);
		low.indices.push_back(0);
		
		for(const MidLevelTexture& tex : mid.textures) {
			MobyTexturePrimitive primitive = {0};
			primitive.d1_xyzf2.data_lo = 0xff92; // Not sure.
			primitive.d1_xyzf2.data_hi = 0x4;
			primitive.d1_xyzf2.address = 0x4;
			primitive.d1_xyzf2.pad_a = 0x41a0;
			primitive.d2_clamp.address = 0x08;
			primitive.d3_tex0.address = 0x06;
			primitive.d3_tex0.data_lo = tex.texture;
			primitive.d4_xyzf2.address = 0x34;
			low.textures.push_back(primitive);
		}
		
		for(const MidLevelDuplicateVertex& dupe : mid.duplicate_vertices) {
			low.duplicate_vertices.push_back(dupe.index);
			
			const glm::vec2& tex_coord = mesh.vertices[dupe.tex_coord].tex_coord;
			s16 s = tex_coord.x * (INT16_MAX / 8.f);
			s16 t = tex_coord.y * (INT16_MAX / 8.f);
			low.sts.push_back({s, t});
		}
		
		low_submeshes.emplace_back(std::move(low));
	}
	
	return low_submeshes;
}

static Vertex recover_vertex(const MobyVertex& vertex, const MobyTexCoord& tex_coord, f32 scale) {
	f32 px = vertex.v.x * (scale / 1024.f);
	f32 py = vertex.v.y * (scale / 1024.f);
	f32 pz = vertex.v.z * (scale / 1024.f);
	f32 normal_azimuth_radians = vertex.v.normal_angle_azimuth * (WRENCH_PI / 128.f);
	f32 normal_elevation_radians = vertex.v.normal_angle_elevation * (WRENCH_PI / 128.f);
	// There's a cosine/sine lookup table at the top of the scratchpad, this is
	// done on the EE core.
	f32 cos_azimuth = cosf(normal_azimuth_radians);
	f32 sin_azimuth = sinf(normal_azimuth_radians);
	f32 cos_elevation = cosf(normal_elevation_radians);
	f32 sin_elevation = sinf(normal_elevation_radians);
	// This bit is done on VU0.
	f32 nx = sin_azimuth * cos_elevation;
	f32 ny = cos_azimuth * cos_elevation;
	f32 nz = sin_elevation;
	f32 s = tex_coord.s / (INT16_MAX / 8.f);
	f32 t = -tex_coord.t / (INT16_MAX / 8.f);
	while(s < 0) s += 1;
	while(t < 0) t += 1;
	return Vertex(glm::vec3(px, py, pz), glm::vec3(nx, ny, nz), glm::vec2(s, t));
}

static MobyVertex build_vertex(const Vertex& src, s32 id, f32 inverse_scale) {
	MobyVertex dest = {0};
	dest.low_halfword = id;
	dest.v.x = src.pos.x * inverse_scale;
	dest.v.y = src.pos.y * inverse_scale;
	dest.v.z = src.pos.z * inverse_scale;
	f32 normal_angle_azimuth_radians;
	if(src.normal.x != 0) {
		normal_angle_azimuth_radians = acotf(src.normal.y / src.normal.x);
		if(src.normal.x < 0) {
			normal_angle_azimuth_radians += WRENCH_PI;
		}
	} else {
		normal_angle_azimuth_radians = WRENCH_PI / 2;
	}
	f32 normal_angle_elevation_radians = asinf(src.normal.z);
	dest.v.normal_angle_azimuth = normal_angle_azimuth_radians * (128.f / WRENCH_PI);
	dest.v.normal_angle_elevation = normal_angle_elevation_radians * (128.f / WRENCH_PI);
	return dest;
}

static BlendAttributes recover_blend_attributes(Opt<BlendAttributes> blend_buffer[64], const MobyVertex& mv, s32 ind, s32 two_way_count, s32 three_way_count) {
	BlendAttributes attribs;
	
	u8 joint = (mv.low_halfword & 0b1111111000000000) >> 9;
	
	auto load_blend_attribs = [&](s32 addr) {
		verify(blend_buffer[addr / 4].has_value(), "Matrix load from uninitialised VU0 address 0x%llx.", addr);
		return *blend_buffer[addr / 4];
	};
	
	if(ind < two_way_count) {
		u8 transfer_addr = mv.v.two_way_blend.vu0_transferred_matrix_store_addr;
		verify(transfer_addr % 4 == 0, "Unaligned joint address 0x%llx.", transfer_addr);
		blend_buffer[transfer_addr / 4] = BlendAttributes{1, {joint}, {1.f}};
		
		BlendAttributes src_1 = load_blend_attribs(mv.v.two_way_blend.vu0_matrix_load_addr_1);
		BlendAttributes src_2 = load_blend_attribs(mv.v.two_way_blend.vu0_matrix_load_addr_2);
		verify(src_1.count == 1 && src_2.count == 1, "Input to two-way matrix blend operation has already been blended.");
		
		u8 blend_addr = mv.v.two_way_blend.vu0_blended_matrix_store_addr;
		verify(blend_addr % 4 == 0, "Unaligned joint address 0x%llx.", blend_addr);
		attribs = src_1; // TODO: Handle blending correctly.
		blend_buffer[blend_addr / 4] = attribs;
	} else if(ind < two_way_count + three_way_count) {
		BlendAttributes src_0 = {1, {joint}, 1.f};
		BlendAttributes src_1 = load_blend_attribs(mv.v.two_way_blend.vu0_matrix_load_addr_1);
		BlendAttributes src_2 = load_blend_attribs(mv.v.two_way_blend.vu0_matrix_load_addr_2);
		verify(src_1.count == 1 && src_2.count == 1, "Input to three-way matrix blend operation has already been blended.");
		u8 blend_addr = mv.v.three_way_blend.vu0_blended_matrix_store_addr;
		verify(blend_addr % 4 == 0, "Unaligned joint address 0x%llx.", blend_addr);
		attribs = src_0; // TODO: Handle blending correctly.
		blend_buffer[blend_addr / 4] = attribs;
	} else {
		u8 transfer_addr = mv.v.regular.vu0_transferred_matrix_store_addr;
		verify(transfer_addr % 4 == 0, "Unaligned joint address 0x%llx.", transfer_addr);
		blend_buffer[transfer_addr / 4] = BlendAttributes{1, {joint}, {1.f}};
		
		attribs = load_blend_attribs(mv.v.regular.vu0_matrix_load_addr);
	}
	
	return attribs;
}

static void find_duplicate_vertices(std::vector<IndexMappingRecord>& index_mapping, const std::vector<Vertex>& vertices) {
	std::vector<size_t> indices(vertices.size());
	for(size_t i = 0; i < vertices.size(); i++) {
		indices[i] = i;
	}
	std::sort(BEGIN_END(indices), [&](size_t l, size_t r) {
		return vertices[l] < vertices[r];
	});
	
	for(size_t i = 1; i < indices.size(); i++) {
		const Vertex& prev = vertices[indices[i - 1]];
		const Vertex& cur = vertices[indices[i]];
		if(vec3_equal_eps(prev.pos, cur.pos) && vec3_equal_eps(prev.normal, cur.normal)) {
			size_t vert = indices[i - 1];
			if(index_mapping[vert].dedup_out_edge != -1) {
				vert = index_mapping[vert].dedup_out_edge;
			}
			index_mapping[indices[i]].dedup_out_edge = vert;
		}
	}
}

static f32 acotf(f32 x) {
	return WRENCH_PI / 2 - atanf(x);
}
