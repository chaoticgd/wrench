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

#ifndef INSTANCEMGR_GAMEPLAY_IMPL_COMMON_H
#define INSTANCEMGR_GAMEPLAY_IMPL_COMMON_H

#include <engine/basic_types.h>
#include <instancemgr/gameplay.h>

template <typename Packed>
static void swap_matrix(Instance& inst, Packed& packed)
{
	glm::mat4 write_matrix = inst.transform().matrix();
	write_matrix[3][3] = 0.01f;
	glm::mat4 read_matrix = packed.matrix.unpack();
	read_matrix[3][3] = 1.f;
	inst.transform().set_from_matrix(&read_matrix);
	packed.matrix = Mat4::pack(write_matrix);
}

template <typename Packed>
static void swap_matrix_inverse_rotation(Instance& inst, Packed& packed)
{
	glm::mat4 write_matrix = inst.transform().matrix();
	write_matrix[3][3] = 0.01f;
	glm::mat4 write_inverse_matrix = inst.transform().inverse_matrix();
	write_inverse_matrix[3][3] = 100.f;
	glm::vec3 write_rotation = inst.transform().rot();
	glm::mat4 read_matrix = packed.matrix.unpack();
	read_matrix[3][3] = 1.f;
	glm::mat4 read_inverse_matrix = packed.inverse_matrix.unpack();
	read_inverse_matrix[3][3] = 1.f;
	glm::vec3 read_rot = packed.rotation.unpack();
	glm::mat4 computed_inverse_matrix = glm::inverse(read_matrix);
	glm::mat4 stored_inverse_matrix = {
		read_inverse_matrix[0],
		read_inverse_matrix[1],
		read_inverse_matrix[2],
		computed_inverse_matrix[3]
	};
	inst.transform().set_from_matrix(&read_matrix, &stored_inverse_matrix, &read_rot);
	packed.matrix = Mat4::pack(write_matrix);
	packed.inverse_matrix = Mat3::pack(write_inverse_matrix);
	packed.rotation = Vec3f::pack(write_rotation);
}

template <typename Packed>
static void swap_position(Instance& instance, Packed& packed)
{
	glm::vec3 pos = instance.transform().pos();
	instance.transform().set_from_pos_rot_scale(packed.position.unpack());
	packed.position = Vec3f::pack(pos);
}

template <typename Packed>
static void swap_position_rotation(Instance& instance, Packed& packed)
{
	glm::vec3 pos = instance.transform().pos();
	glm::vec3 rot = instance.transform().rot();
	instance.transform().set_from_pos_rot_scale(packed.position.unpack(), packed.rotation.unpack());
	packed.position = Vec3f::pack(pos);
	packed.rotation = Vec3f::pack(rot);
}

template <typename Packed>
static void swap_position_rotation_scale(Instance& instance, Packed& packed)
{
	glm::vec3 pos = instance.transform().pos();
	glm::vec3 rot = instance.transform().rot();
	f32 scale = instance.transform().scale();
	instance.transform().set_from_pos_rot_scale(packed.position.unpack(), packed.rotation.unpack(), packed.scale);
	packed.position = Vec3f::pack(pos);
	packed.rotation = Vec3f::pack(rot);
	packed.scale = scale;
}

packed_struct(Rgb24,
	/* 0x0 */ u8 r;
	/* 0x1 */ u8 g;
	/* 0x2 */ u8 b;
)

packed_struct(Rgb32,
	/* 0x0 */ u8 r;
	/* 0x1 */ u8 g;
	/* 0x2 */ u8 b;
	/* 0x3 */ u8 pad;
)

packed_struct(Rgb96,
	/* 0x0 */ s32 r;
	/* 0x4 */ s32 g;
	/* 0xc */ s32 b;
)

#define SWAP_COLOUR(vector, packed) \
	{ \
		glm::vec3 temp = vector; \
		(vector).r = (packed).r * (1.f / 255.f); \
		(vector).g = (packed).g * (1.f / 255.f); \
		(vector).b = (packed).b * (1.f / 255.f); \
		(packed).r = (u8) roundf(temp.r * 255.f); \
		(packed).g = (u8) roundf(temp.g * 255.f); \
		(packed).b = (u8) roundf(temp.b * 255.f); \
	}

#define SWAP_COLOUR_OPT(vector, packed) \
	{ \
		Opt<glm::vec3> temp = (vector); \
		if((packed).r != -1) { \
			(vector).emplace(); \
			(vector)->r = (packed).r * (1.f / 255.f); \
			(vector)->g = (packed).g * (1.f / 255.f); \
			(vector)->b = (packed).b * (1.f / 255.f); \
		} else { \
			vector = std::nullopt; \
		} \
		if(temp.has_value()) { \
			(packed).r = (u8) roundf(temp->r * 255.f); \
			(packed).g = (u8) roundf(temp->g * 255.f); \
			(packed).b = (u8) roundf(temp->b * 255.f); \
		} else { \
			(packed).r = -1; \
			(packed).g = 0; \
			(packed).b = 0; \
		} \
	}

packed_struct(TableHeader,
	s32 count_1;
	s32 pad[3];
)

template <typename T>
struct TableBlock
{
	static void read(std::vector<T>& dest, Buffer src, Game game)
	{
		auto header = src.read<TableHeader>(0, "table header");
		verify(header.pad[0] == 0, "TableBlock contains more than one table.");
		dest = src.read_multiple<T>(0x10, header.count_1, "table body").copy();
	}
	
	static void write(OutBuffer dest, const std::vector<T>& src, Game game) {
		TableHeader header = {(s32) src.size()};
		dest.write(header);
		for(const T& elem : src) {
			dest.write(elem);
		}
	}
};

template <typename ThisInstance, typename Packed>
struct InstanceBlock
{
	static void read(std::vector<ThisInstance>& dest, Buffer src, Game game)
	{
		TableHeader header = src.read<TableHeader>(0, "instance block header");
		auto entries = src.read_multiple<Packed>(0x10, header.count_1, "instances");
		s32 index = 0;
		for(Packed packed : entries) {
			ThisInstance inst;
			inst.set_id_value(index++);
			swap_instance(inst, packed);
			dest.push_back(inst);
		}
	}
	
	static void write(OutBuffer dest, const std::vector<ThisInstance>& src, Game game)
	{
		TableHeader header = {(s32) src.size()};
		dest.write(header);
		for(ThisInstance instance : src) {
			Packed packed;
			swap_instance(instance, packed);
			dest.write(packed);
		}
	}
};

#endif
