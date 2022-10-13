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

static std::vector<MobyBangle> read_moby_bangles(Buffer src, s32 bangles_ofs, s32 submesh_table_offset, f32 scale, MobyFormat format, bool animated);
static s64 write_moby_bangles(OutBuffer dest, s64 bangles_ofs, s64 submesh_table_ofs, s32 submesh, const std::vector<MobyBangle>& bangles, f32 scale, MobyFormat format);
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
	moby.animation.joint_count = header.joint_count;
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
		case Game::RAC:
			format = MobyFormat::RAC1;
			break;
		case Game::GC:
			if(header.rac12_byte_b == 0) {
				format = MobyFormat::RAC2;
			} else {
				format = MobyFormat::RAC1;
				moby.force_rac1_format = true;
			}
			break;
		case Game::UYA:
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
		moby.header_end_offset = std::min(moby.header_end_offset, header.bangles * 0x10);
	}
	if(game == Game::RAC)  {
		moby.rac1_short_2e = header.corncob;
	} else if(header.corncob != 0) {
		moby.corncob = read_moby_corncob(src.subbuf(header.corncob * 0x10));
		moby.header_end_offset = std::min(moby.header_end_offset, header.corncob * 0x10);
	}
	moby.animation.sequences = read_moby_sequences(src, header.sequence_count, header.joint_count, game);
	verify(header.sequence_count >= 1, "Moby class has no sequences.");
	if(header.collision != 0) {
		moby.collision = read_moby_collision(src.subbuf(header.collision));
		s64 coll_size = 0x10 + moby.collision->first_part.size() + moby.collision->second_part.size() * 8 + moby.collision->third_part.size();
	}
	if(header.skeleton != 0) {
		moby.shadow = src.read_bytes(header.skeleton - header.shadow * 16, header.shadow * 16, "shadow");
		if (game == Game::DL) {
			moby.animation.skeleton.emplace();
			for (const Mat3& src_mat : src.read_multiple<Mat3>(header.skeleton, header.joint_count, "skeleton")) {
				Mat4 dest_mat;
				dest_mat.m_0 = src_mat.m_0;
				dest_mat.m_1 = src_mat.m_1;
				dest_mat.m_2 = src_mat.m_2;
				dest_mat.m_3 = { src_mat.m_0.w, src_mat.m_1.w, src_mat.m_2.w, 0 };
				moby.animation.skeleton->emplace_back(dest_mat);
			}
		}
		else {
			moby.animation.skeleton = src.read_multiple<Mat4>(header.skeleton, header.joint_count, "skeleton").copy();
		}
	}
	if(header.common_trans != 0) {
		moby.animation.common_trans = src.read_multiple<MobyTrans>(header.common_trans, header.joint_count, "skeleton trans").copy();
	}
	moby.animation.joints = read_moby_joints(src, header.joints);
	moby.sound_defs = src.read_multiple<MobySoundDef>(header.sound_defs, header.sound_count, "moby sound defs").copy();
	if(header.submesh_table_offset != 0) {
		moby.submesh_table_offset = header.submesh_table_offset;
		moby.mesh = read_moby_mesh_section(src, header.submesh_table_offset, header.mesh_info, header.scale, format, moby.animation.joint_count > 0);
		if(header.bangles != 0) {
			moby.bangles = read_moby_bangles(src, header.bangles * 0x10, header.submesh_table_offset, header.scale, format, moby.animation.joint_count > 0);
		}
	} else {
		moby.mesh.has_submesh_table = false;
	}
	if(header.rac3dl_team_textures != 0 && (game == Game::UYA || game == Game::DL)) {
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
		case Game::RAC:
			format = MobyFormat::RAC1;
			break;
		case Game::GC:
			format = moby.force_rac1_format > 0 ? MobyFormat::RAC1 : MobyFormat::RAC2;
			break;
		case Game::UYA:
		case Game::DL:
			format = MobyFormat::RAC3DL;
			break;
		default:
			assert_not_reached("Bad game enum.");
	}
	
	if(format == MobyFormat::RAC1) {
		header.rac1_byte_a = moby.rac1_byte_a;
		header.rac12_byte_b = moby.rac1_byte_b;
	}
	verify(moby.animation.joint_count <= 0x6f, "Max joint count is 0x6f.");
	header.joint_count = moby.animation.joint_count;
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
	
	verify(moby.animation.sequences.size() < 256, "Moby class has too many sequences (max is 255).");
	header.sequence_count = moby.animation.sequences.size();
	s64 sequence_list_ofs = dest.alloc_multiple<s32>(moby.animation.sequences.size());
	while(dest.tell() - class_header_ofs < moby.header_end_offset) {
		dest.write<u8>(0);
	}
	s64 bangles_ofs;
	if(!moby.bangles.empty()) {
		dest.pad(0x10);
		bangles_ofs = dest.alloc_multiple<u8>(64 + 16 * moby.bangles.size());
		header.bangles = (bangles_ofs - class_header_ofs) / 0x10;
	}
	if(game == Game::RAC) {
		header.corncob = moby.rac1_short_2e;
	} else if(moby.corncob.has_value()) {
		dest.pad(0x10);
		header.corncob = (write_moby_corncob(dest, *moby.corncob) - class_header_ofs) / 0x10;
	}
	dest.pad(0x10);
	write_moby_sequences(dest, moby.animation.sequences, class_header_ofs, sequence_list_ofs, moby.animation.joint_count, game);
	dest.pad(0x10);
	while(dest.tell() < class_header_ofs + moby.submesh_table_offset) {
		dest.write<u8>(0);
	}
	s64 submesh_table_ofs = allocate_submesh_table(dest, moby.mesh);
	s64 bangles_submesh_table_ofs = 0;
	if(!moby.bangles.empty()) {
		bangles_submesh_table_ofs = dest.alloc_multiple<MobySubMeshEntry>(moby.bangles.size());
	}
	if(moby.mesh.has_submesh_table) {
		header.submesh_table_offset = submesh_table_ofs - class_header_ofs;
	}
	dest.write<s32>(0);
	if(moby.collision.has_value()) {
		header.collision = write_moby_collision(dest, *moby.collision) - class_header_ofs;
	}
	if(moby.animation.skeleton.has_value()) {
		dest.pad(0x10);
		dest.write_multiple(moby.shadow);
		header.skeleton = dest.tell() - class_header_ofs;
		verify(moby.animation.skeleton->size() < 255, "Moby class has too many joints.");
		if(game == Game::DL) {
			for(const Mat4& mat : *moby.animation.skeleton) {
				dest.write(mat.m_0);
				dest.write(mat.m_1);
				dest.write(mat.m_2);
			}
		} else {
			dest.write_multiple(*moby.animation.skeleton);
		}
	}
	dest.pad(0x10);
	if(moby.animation.common_trans.has_value()) {
		header.common_trans = dest.write_multiple(*moby.animation.common_trans) - class_header_ofs;
	}
	header.joints = write_moby_joints(dest, moby.animation.joints) - class_header_ofs;
	dest.pad(0x10);
	if(moby.sound_defs.size() > 0) {
		header.sound_defs = dest.write_multiple(moby.sound_defs) - class_header_ofs;
	}
	std::vector<MobyGifUsage> gif_usage;
	header.mesh_info = write_moby_mesh_section(dest, gif_usage, submesh_table_ofs, moby.mesh, moby.scale, format);
	if(!moby.bangles.empty()) {
		write_moby_bangles(dest, bangles_ofs, submesh_table_ofs, header.mesh_info.metal_begin + header.mesh_info.metal_count, moby.bangles, moby.scale, format);
	}
	if(moby.team_palettes.size() > 0 && (game == Game::UYA || game == Game::DL)) {
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

// *****************************************************************************

MobyClassData read_armor_moby_class(Buffer src, Game game) {
	MobyFormat format;
	switch(game) {
		case Game::RAC:
			format = MobyFormat::RAC1;
			break;
		case Game::GC:
			format = MobyFormat::RAC2;
			break;
		case Game::UYA:
		case Game::DL:
			format = MobyFormat::RAC3DL;
			break;
		default:
			assert_not_reached("Bad game enum.");
	}
	
	auto header = src.read<MobyArmorHeader>(0, "moby armor header");
	MobyClassData moby;
	moby.mesh = read_moby_mesh_section(src, header.submesh_table_offset, header.info, 1.f, format, true);
	return moby;
}

void write_armor_moby_class(OutBuffer dest, const MobyClassData& moby, Game game) {
	MobyFormat format;
	switch(game) {
		case Game::RAC:
			format = MobyFormat::RAC1;
			break;
		case Game::GC:
			format = MobyFormat::RAC2;
			break;
		case Game::UYA:
		case Game::DL:
			format = MobyFormat::RAC3DL;
			break;
		default:
			assert_not_reached("Bad game enum.");
	}
	
	MobyArmorHeader header = {};
	class_header_ofs = dest.alloc<MobyArmorHeader>();
	s64 table_ofs = allocate_submesh_table(dest, moby.mesh);
	std::vector<MobyGifUsage> gif_usage;
	header.info = write_moby_mesh_section(dest, gif_usage, table_ofs, moby.mesh, moby.scale, format);
	if(gif_usage.size() > 0) {
		gif_usage.back().offset_and_terminator |= 0x80000000;
		header.gif_usage = dest.write_multiple(gif_usage) - class_header_ofs;
	}
	dest.write(class_header_ofs, header);
}

// *****************************************************************************

MobyMeshSection read_moby_mesh_section(Buffer src, s64 table_ofs, MobyMeshInfo info, f32 scale, MobyFormat format, bool animated) {
	MobyMeshSection mesh;
	mesh.high_lod_count = info.high_lod_count;
	mesh.low_lod_count = info.low_lod_count;
	mesh.metal_count = info.metal_count;
	mesh.high_lod = read_moby_submeshes(src, table_ofs, info.high_lod_count, scale, animated, format);
	s64 low_lod_table_ofs = table_ofs + info.high_lod_count * 0x10;
	mesh.low_lod = read_moby_submeshes(src, low_lod_table_ofs, info.low_lod_count, scale, animated, format);
	s64 metal_table_ofs = table_ofs + info.metal_begin * 0x10;
	mesh.metal = read_moby_metal_submeshes(src, metal_table_ofs, info.metal_count);
	mesh.has_submesh_table = true;
	return mesh;
}

s64 allocate_submesh_table(OutBuffer& dest, const MobyMeshSection& mesh) {
	size_t count = mesh.high_lod.size() + mesh.low_lod.size() + mesh.metal.size();
	return dest.alloc_multiple<MobySubMeshEntry>(count);
}

MobyMeshInfo write_moby_mesh_section(OutBuffer& dest, std::vector<MobyGifUsage>& gif_usage, s64 table_ofs, const MobyMeshSection& mesh, f32 scale, MobyFormat format) {
	MobyMeshInfo info;
	
	assert(!mesh.has_submesh_table | (mesh.high_lod.size() == mesh.high_lod_count));
	verify(mesh.high_lod.size() < 256, "Moby class has too many submeshes.");
	info.high_lod_count = mesh.high_lod_count;
	assert(!mesh.has_submesh_table | (mesh.low_lod.size() == mesh.low_lod_count));
	verify(mesh.low_lod.size() < 256, "Moby class has too many low detail submeshes.");
	info.low_lod_count = mesh.low_lod_count;
	assert(!mesh.has_submesh_table | (mesh.metal.size() == mesh.metal_count));
	verify(mesh.metal.size() < 256, "Moby class has too many metal submeshes.");
	info.metal_count = mesh.metal_count;
	info.metal_begin = mesh.high_lod_count + mesh.low_lod_count;
	
	write_moby_submeshes(dest, gif_usage, table_ofs, mesh.high_lod.data(), mesh.high_lod.size(), scale, format, class_header_ofs);
	table_ofs += mesh.high_lod.size() * sizeof(MobySubMeshEntry);
	write_moby_submeshes(dest, gif_usage, table_ofs, mesh.low_lod.data(), mesh.low_lod.size(), scale, format, class_header_ofs);
	table_ofs += mesh.low_lod.size() * sizeof(MobySubMeshEntry);
	write_moby_metal_submeshes(dest, table_ofs, mesh.metal, class_header_ofs);
	
	return info;
}

// *****************************************************************************

static std::vector<MobyBangle> read_moby_bangles(Buffer src, s32 bangles_ofs, s32 submesh_table_offset, f32 scale, MobyFormat format, bool animated) {
	std::vector<MobyBangle> bangles;
	auto indices = src.read_multiple<MobyBangleIndices>(bangles_ofs + 4, 15, "bangle indices");
	s32 i = 0;
	for(const MobyBangleIndices& ind : indices) {
		MobyBangle bangle;
		if(ind.high_lod_submesh_begin != 0) {
			s32 high_lod_ofs = submesh_table_offset + ind.high_lod_submesh_begin * 0x10;
			bangle.submeshes = read_moby_submeshes(src, high_lod_ofs, ind.high_lod_submesh_count, scale, animated, format);
		}
		if(ind.low_lod_submesh_begin != 0) {
			s32 low_lod_ofs = submesh_table_offset + ind.low_lod_submesh_begin * 0x10;
			bangle.low_lod_submeshes = read_moby_submeshes(src, low_lod_ofs, ind.low_lod_submesh_count, scale, animated, format);
		}
		if(!bangle.submeshes.empty() || !bangle.low_lod_submeshes.empty()) {
			bangle.vectors[0] = src.read<MobyVec4>(bangles_ofs + 64 + i * 16 + 0, "bangle vector 1");
			bangle.vectors[1] = src.read<MobyVec4>(bangles_ofs + 64 + i * 16 + 8, "bangle vector 2");
			bangles.emplace_back(std::move(bangle));
		}
		i++;
	}
	return bangles;
}

static s64 write_moby_bangles(OutBuffer dest, s64 bangles_ofs, s64 submesh_table_ofs, s32 submesh, const std::vector<MobyBangle>& bangles, f32 scale, MobyFormat format) {
	// TODO: Implement this.
	s64 ofs = dest.tell();
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
				kernel.vertices = src.read_multiple<MobyVec4>(kernel_ofs + 0x10, vertex_count, "corn vertices").copy();
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

// *****************************************************************************

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

// *****************************************************************************

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

// *****************************************************************************

ColladaScene recover_moby_class(const MobyClassData& moby, s32 o_class, s32 texture_count) {
	ColladaScene scene;
	
	// Used for when the texture index stored in a GS primitive is -1.
	ColladaMaterial& none = scene.materials.emplace_back();
	none.name = "none";
	none.surface = MaterialSurface(glm::vec4(1, 1, 1, 1));
	// Used for when there're more textures referenced than are listed in the
	// moby class table. This happens for R&C2 ship parts.
	ColladaMaterial& dummy = scene.materials.emplace_back();
	dummy.name = "dummy";
	dummy.surface = MaterialSurface(glm::vec4(0.5, 0.5, 0.5, 1));
	
	for(s32 texture = 0; texture < texture_count; texture++) {
		ColladaMaterial& mat = scene.materials.emplace_back();
		mat.name = "mat_" + std::to_string(texture);
		mat.surface = MaterialSurface(texture);
	}
	for(s32 texture = 0; texture < texture_count; texture++) {
		ColladaMaterial& chrome = scene.materials.emplace_back();
		chrome.name = "chrome_" + std::to_string(texture);
		chrome.surface = MaterialSurface(texture);
	}
	for(s32 texture = 0; texture < texture_count; texture++) {
		ColladaMaterial& glass = scene.materials.emplace_back();
		glass.name = "glass_" + std::to_string(texture);
		glass.surface = MaterialSurface(texture);
	}
	
	if(MOBY_EXPORT_SUBMESHES_SEPERATELY) {
		for(s32 i = 0; i < (s32) moby.mesh.high_lod.size(); i++) {
			std::string name = "high_lod_" + std::to_string(i);
			scene.meshes.emplace_back(recover_moby_mesh(moby.mesh.high_lod, name.c_str(), o_class, texture_count, i));
		}
		for(s32 i = 0; i < (s32) moby.mesh.low_lod.size(); i++) {
			std::string name = "low_lod_" + std::to_string(i);
			scene.meshes.emplace_back(recover_moby_mesh(moby.mesh.low_lod, name.c_str(), o_class, texture_count, i));
		}
	} else {
		scene.meshes.emplace_back(recover_moby_mesh(moby.mesh.high_lod, "high_lod", o_class, texture_count, NO_SUBMESH_FILTER));
		scene.meshes.emplace_back(recover_moby_mesh(moby.mesh.low_lod, "low_lod", o_class, texture_count, NO_SUBMESH_FILTER));
	}
	
	for(s32 i = 0; i < (s32) moby.bangles.size(); i++) {
		std::string name = stringf("bangle_%d", i);
		scene.meshes.emplace_back(recover_moby_mesh(moby.bangles[i].submeshes, name.c_str(), o_class, texture_count, NO_SUBMESH_FILTER));
		
		std::string low_lod_name = stringf("bangle_%d_low_lod", i);
		scene.meshes.emplace_back(recover_moby_mesh(moby.bangles[i].low_lod_submeshes, low_lod_name.c_str(), o_class, texture_count, NO_SUBMESH_FILTER));
	}
	
	if(moby.animation.joint_count != 0) {
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
	moby.mesh.high_lod = build_moby_submeshes(*high_lod_mesh, scene.materials);
	moby.mesh.high_lod_count = moby.mesh.high_lod.size();
	if(low_lod_mesh) {
		moby.mesh.low_lod = build_moby_submeshes(*low_lod_mesh, scene.materials);
		moby.mesh.low_lod_count = moby.mesh.low_lod.size();
	}
	moby.animation.skeleton = {};
	moby.animation.common_trans = {};
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
	moby.mesh.has_submesh_table = true;
	
	MobySequence dummy_seq;
	dummy_seq.bounding_sphere = glm::vec4(0.f, 0.f, 0.f, 10.f); // Arbitrary for now.
	dummy_seq.frames.emplace_back();
	moby.animation.sequences.emplace_back(std::move(dummy_seq));
	
	return moby;
}

static std::vector<Joint> recover_moby_joints(const MobyClassData& moby, f32 scale) {
	assert(opt_size(moby.animation.skeleton) == opt_size(moby.animation.common_trans));
	return {};
	std::vector<Joint> joints;
	joints.reserve(opt_size(moby.animation.common_trans));
	s32 parent;
	bool is_rc4_format = false; // rc4 has mobys in both formats so we need to dynamically determine format
	
	for(size_t i = 0; i < opt_size(moby.animation.common_trans); i++) {
		const MobyTrans& trans = (*moby.animation.common_trans)[i];
		
		auto matrix = (*moby.animation.skeleton)[i].unpack();
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

		// RC4 bone parents are stored as direct idx of parent bone
		// The highest bit is sometimes set for unknown reasons
		// A parent idx of 0x7F means no parent
		auto rc4_parent_idx = trans.parent_offset & ~0x80;
		if (rc4_parent_idx == 0x7F) {
			is_rc4_format = true;
			parent = -1;
		}
		else if (i > 0) {
			if (is_rc4_format) {
				parent = rc4_parent_idx; // parent joint id is stored in the lower bits
			}
			else {
				parent = trans.parent_offset / 0x40;
			}
		}
		else {
			parent = -1;
		}

		verify(parent < (s32) joints.size(), "Bad moby joints.");
		add_joint(joints, j, parent);
	}
	
	return joints;
}
