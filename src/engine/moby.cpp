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

#include "moby.h"

static MobyBangles read_moby_bangles(Buffer src);
static s64 write_moby_bangles(OutBuffer dest, const MobyBangles& bangles);
static MobyCornCob read_moby_corncob(Buffer src);
static s64 write_moby_corncob(OutBuffer dest, const MobyCornCob& corncob);
static MobyCollision read_moby_collision(Buffer src);
static s64 write_moby_collision(OutBuffer dest, const MobyCollision& collision);
static std::vector<MobyJointEntry> read_moby_joints(Buffer src, s64 joints_ofs);
static s64 write_moby_joints(OutBuffer dest, const std::vector<MobyJointEntry>& joints);
static std::vector<Joint> recover_moby_joints(const MobyClassData& moby, f32 scale);

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
	moby.scale = header.scale;
	moby.mip_dist = header.mip_dist;
	moby.bounding_sphere = header.bounding_sphere.unpack();
	moby.glow_rgba = header.glow_rgba;
	moby.mode_bits = header.mode_bits;
	moby.type = header.type;
	moby.mode_bits2 = header.mode_bits2;
	
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
	moby.sequences = read_moby_sequences(src, header.sequence_count, header.joint_count, game);
	verify(header.sequence_count >= 1, "Moby class has no sequences.");
	if(header.collision != 0) {
		moby.collision = read_moby_collision(src.subbuf(header.collision));
		s64 coll_size = 0x10 + moby.collision->first_part.size() + moby.collision->second_part.size() * 8 + moby.collision->third_part.size();
	}
	if(header.skeleton != 0) {
		moby.shadow = src.read_bytes(header.skeleton - header.shadow * 16, header.shadow * 16, "shadow");
		if(game == Game::DL) {
			moby.skeleton.emplace();
			for(const Mat3& src_mat : src.read_multiple<Mat3>(header.skeleton, header.joint_count, "skeleton")) {
				Mat4 dest_mat;
				dest_mat.m_0 = src_mat.m_0;
				dest_mat.m_1 = src_mat.m_1;
				dest_mat.m_2 = src_mat.m_2;
				dest_mat.m_3 = {0, 0, 0, 0};
				moby.skeleton->emplace_back(dest_mat);
			}
		} else {
			moby.skeleton = src.read_multiple<Mat4>(header.skeleton, header.joint_count, "skeleton").copy();
		}
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
		moby.submeshes = read_moby_submeshes(src, header.submesh_table_offset, header.submesh_count, moby.scale, moby.joint_count, format);
		s64 low_lod_table_ofs = header.submesh_table_offset + header.submesh_count * 0x10;
		moby.low_lod_submeshes = read_moby_submeshes(src, low_lod_table_ofs, header.low_lod_submesh_count, moby.scale, moby.joint_count, format);
		s64 metal_table_ofs = header.submesh_table_offset + header.metal_submesh_begin * 0x10;
		moby.metal_submeshes = read_moby_metal_submeshes(src, metal_table_ofs, header.metal_submesh_count);
		if(header.bangles != 0) {
			s64 bangles_submesh_table_ofs = header.submesh_table_offset + moby.bangles->header.submesh_begin * 0x10;
			moby.bangles->submeshes = read_moby_submeshes(src, bangles_submesh_table_ofs, moby.bangles->header.submesh_count, moby.scale, moby.joint_count, format);
		}
	}
	if(header.rac3dl_team_textures != 0 && (game == Game::RAC3 || game == Game::DL)) {
		//verify(header.gif_usage != 0, "Moby with team palettes but no gif table.");
		moby.palettes_per_texture = header.rac3dl_team_textures & 0xf;
		s32 texture_count = (header.rac3dl_team_textures & 0xf0) >> 4;
		for(s32 i = moby.palettes_per_texture * texture_count; i > 0; i--) {
			//std::array<u32, 256>& dest = moby.team_palettes.emplace_back();
			//auto palette = src.read_multiple<u8>(header.gif_usage - i * 1024, 1024, "team palette");
			//memcpy(dest.data(), palette.lo, 1024);
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
	header.shadow = moby.shadow.size() / 16;
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
	write_moby_sequences(dest, moby.sequences, class_header_ofs, sequence_list_ofs, moby.joint_count, game);
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
	dest.write<s32>(0);
	if(moby.collision.has_value()) {
		header.collision = write_moby_collision(dest, *moby.collision) - class_header_ofs;
	}
	if(moby.skeleton.has_value()) {
		dest.pad(0x10);
		dest.write_multiple(moby.shadow);
		header.skeleton = dest.tell() - class_header_ofs;
		verify(moby.skeleton->size() < 255, "Moby class has too many joints.");
		if(game == Game::DL) {
			for(const Mat4& mat : *moby.skeleton) {
				dest.write(mat.m_0);
				dest.write(mat.m_1);
				dest.write(mat.m_2);
			}
		} else {
			dest.write_multiple(*moby.skeleton);
		}
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
#define DATA_SIZE(vec) vec.data(), vec.size()
	write_moby_submeshes(dest, gif_usage, submesh_table_1_ofs, DATA_SIZE(moby.submeshes), moby.scale, format, class_header_ofs);
	write_moby_submeshes(dest, gif_usage, submesh_table_2_ofs, DATA_SIZE(moby.low_lod_submeshes), moby.scale, format, class_header_ofs);
	write_moby_metal_submeshes(dest, metal_submesh_table_ofs, moby.metal_submeshes, class_header_ofs);
#undef DATA_SIZE
	if(moby.bangles.has_value()) {
		write_moby_bangle_submeshes(dest, gif_usage, bangles_submesh_table_ofs, *moby.bangles, moby.scale, format, class_header_ofs);
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
	bangles.header = src.read<MobyBangleHeader>(0, "moby bangle header");
	bangles.bangles = src.read_multiple<MobyBangle>(4, 15, "moby bangles").copy();
	s32 bangle_count = 0;
	for(MobyBangle& bangle : bangles.bangles) {
		if(bangle.high_lod_submesh_begin != 0 || bangle.high_lod_submesh_begin != 0) {
			bangle_count++;
		}
	}
	bangles.vertices = src.read_multiple<MobyVertexPosition>(0x40, 2 * bangle_count, "moby bangle vertices").copy();
	return bangles;
}

static s64 write_moby_bangles(OutBuffer dest, const MobyBangles& bangles) {
	s64 ofs = dest.tell();
	dest.write(bangles.header);
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
			scene.meshes.emplace_back(recover_moby_mesh(moby.submeshes, name.c_str(), o_class, texture_count, i));
		}
		for(s32 i = 0; i < (s32) moby.low_lod_submeshes.size(); i++) {
			std::string name = "low_lod_" + std::to_string(i);
			scene.meshes.emplace_back(recover_moby_mesh(moby.low_lod_submeshes, name.c_str(), o_class, texture_count, i));
		}
		if(moby.bangles.has_value()) {
			for(s32 i = 0; i < (s32) moby.bangles->submeshes.size(); i++) {
				std::string name = "bangles_" + std::to_string(i);
				scene.meshes.emplace_back(recover_moby_mesh(moby.bangles->submeshes, name.c_str(), o_class, texture_count, i));
			}
		}
	} else {
		scene.meshes.emplace_back(recover_moby_mesh(moby.submeshes, "high_lod", o_class, texture_count, NO_SUBMESH_FILTER));
		scene.meshes.emplace_back(recover_moby_mesh(moby.low_lod_submeshes, "low_lod", o_class, texture_count, NO_SUBMESH_FILTER));
		if(moby.bangles.has_value()) {
			scene.meshes.emplace_back(recover_moby_mesh(moby.bangles->submeshes, "bangles", o_class, texture_count, NO_SUBMESH_FILTER));
		}
	}
	
	if(moby.joint_count != 0) {
		scene.joints = recover_moby_joints(moby, moby.scale);
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
	moby.submeshes = build_moby_submeshes(*high_lod_mesh, scene.materials);
	moby.submesh_count = moby.submeshes.size();
	if(low_lod_mesh) {
		moby.low_lod_submeshes = build_moby_submeshes(*low_lod_mesh, scene.materials);
		moby.low_lod_submesh_count = moby.low_lod_submeshes.size();
	}
	moby.skeleton = {};
	moby.common_trans = {};
	moby.unknown_9 = 0;
	moby.lod_trans = 0x20;
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

static std::vector<Joint> recover_moby_joints(const MobyClassData& moby, f32 scale) {
	assert(opt_size(moby.skeleton) == opt_size(moby.common_trans));
	return {};
	std::vector<Joint> joints;
	joints.reserve(opt_size(moby.common_trans));
	
	for(size_t i = 0; i < opt_size(moby.common_trans); i++) {
		const MobyTrans& trans = (*moby.common_trans)[i];
		
		auto matrix = (*moby.skeleton)[i].unpack();
		auto mat3 = glm::mat3(matrix);
		mat3 = glm::inverse(mat3);
		glm::mat4 inv_bind_mat {
			{mat3[0], 0.f},
			{mat3[1], 0.f},
			{mat3[2], 0.f},
			{glm::vec3(matrix[3] * (scale / 1024.f)), 1.f}
		};
		
		glm::vec3 tip = matrix[3];
		tip *= (scale / 1024.f);
		tip = -tip * mat3;
		
		f32 tip_length = glm::length(tip);
		if(tip_length * tip_length < 0.000001f) {
			tip = glm::vec3(0, 0, 0.001f);
		}
		
		Joint j;
		j.inverse_bind_matrix = inv_bind_mat;
		j.tip = tip;
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
