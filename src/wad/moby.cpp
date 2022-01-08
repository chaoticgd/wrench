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
static std::vector<Joint> recover_moby_joints(const MobyClassData& moby);

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
	verify(moby.joint_count <= 0x6f, "Max joint count is 0x6f.");
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
	write_moby_submeshes(dest, gif_usage, submesh_table_1_ofs, moby.submeshes, format, class_header_ofs);
	write_moby_submeshes(dest, gif_usage, submesh_table_2_ofs, moby.low_lod_submeshes, format, class_header_ofs);
	write_moby_metal_submeshes(dest, metal_submesh_table_ofs, moby.metal_submeshes, class_header_ofs);
	if(moby.bangles.has_value()) {
		write_moby_submeshes(dest, gif_usage, bangles_submesh_table_ofs, moby.bangles->submeshes, format, class_header_ofs);
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
			scene.meshes.emplace_back(recover_moby_mesh(moby.submeshes, name.c_str(), o_class, texture_count, moby.joint_count, moby.scale, i));
		}
		for(s32 i = 0; i < (s32) moby.low_lod_submeshes.size(); i++) {
			std::string name = "low_lod_" + std::to_string(i);
			scene.meshes.emplace_back(recover_moby_mesh(moby.low_lod_submeshes, name.c_str(), o_class, texture_count, moby.joint_count, moby.scale, i));
		}
		if(moby.bangles.has_value()) {
			for(s32 i = 0; i < (s32) moby.bangles->submeshes.size(); i++) {
				std::string name = "bangles_" + std::to_string(i);
				scene.meshes.emplace_back(recover_moby_mesh(moby.bangles->submeshes, name.c_str(), o_class, texture_count, moby.joint_count, moby.scale, i));
			}
		}
	} else {
		scene.meshes.emplace_back(recover_moby_mesh(moby.submeshes, "high_lod", o_class, texture_count, moby.joint_count, moby.scale, NO_SUBMESH_FILTER));
		scene.meshes.emplace_back(recover_moby_mesh(moby.low_lod_submeshes, "low_lod", o_class, texture_count, moby.joint_count, moby.scale, NO_SUBMESH_FILTER));
		if(moby.bangles.has_value()) {
			scene.meshes.emplace_back(recover_moby_mesh(moby.bangles->submeshes, "bangles", o_class, texture_count, moby.joint_count, moby.scale, NO_SUBMESH_FILTER));
		}
	}
	
	if(moby.joint_count != 0) {
		scene.joints = recover_moby_joints(moby);
	}
	
	return scene;
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

static std::vector<Joint> recover_moby_joints(const MobyClassData& moby) {
	assert(opt_size(moby.skeleton) == opt_size(moby.common_trans));
	
	std::vector<Joint> joints;
	joints.reserve(opt_size(moby.common_trans));
	
	for(size_t i = 0; i < opt_size(moby.common_trans); i++) {
		const MobyTrans& trans = (*moby.common_trans)[i];
		
		Joint j;
		j.inverse_bind_matrix = glm::mat4(1.f);
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
