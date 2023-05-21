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
static void swap_matrix(Instance& inst, Packed& packed) {
	glm::mat4 matrix = inst.transform().matrix();
	inst.transform().set_from_matrix(packed.matrix.unpack());
	packed.matrix = Mat4::pack(matrix);
}

template <typename Packed>
static void swap_matrix_inverse_rotation(Instance& inst, Packed& packed) {
	glm::mat4 matrix = inst.transform().matrix();
	glm::mat3 inverse_matrix = inst.transform().inverse_matrix();
	glm::vec3 rotation = inst.transform().rot();
	inst.transform().set_from_matrix(packed.matrix.unpack());
	packed.matrix = Mat4::pack(matrix);
	packed.inverse_matrix = Mat3::pack(inverse_matrix);
	packed.rotation = Vec3f::pack(rotation);
}

template <typename Packed>
static void swap_position(Instance& instance, Packed& packed) {
	glm::vec3 pos = instance.transform().pos();
	instance.transform().set_from_pos_rot_scale(packed.position.unpack());
	packed.position = Vec3f::pack(pos);
}

template <typename Packed>
static void swap_position_rotation(Instance& instance, Packed& packed) {
	glm::vec3 pos = instance.transform().pos();
	glm::vec3 rot = instance.transform().rot();
	instance.transform().set_from_pos_rot_scale(packed.position.unpack(), packed.rotation.unpack());
	packed.position = Vec3f::pack(pos);
	packed.rotation = Vec3f::pack(rot);
}

template <typename Packed>
static void swap_position_rotation_scale(Instance& instance, Packed& packed) {
	glm::vec3 pos = instance.transform().pos();
	glm::vec3 rot = instance.transform().rot();
	f32 scale = instance.transform().scale();
	instance.transform().set_from_pos_rot_scale(packed.position.unpack(), packed.rotation.unpack(), packed.scale);
	packed.position = Vec3f::pack(pos);
	packed.rotation = Vec3f::pack(rot);
	packed.scale = scale;
}

packed_struct(TableHeader,
	s32 count_1;
	s32 pad[3];
)

template <typename T>
struct TableBlock {
	static void read(std::vector<T>& dest, Buffer src, Game game) {
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
struct InstanceBlock {
	static void read(std::vector<ThisInstance>& dest, Buffer src, Game game) {
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
	
	static void write(OutBuffer dest, const std::vector<ThisInstance>& src, Game game) {
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
