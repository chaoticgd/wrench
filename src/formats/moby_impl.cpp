/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019 chaoticgd

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

#include "moby_impl.h"

#include "../shapes.h"

moby_impl::moby_impl(stream* backing, uint32_t offset)
	: _backing(backing, offset, 0x88) {}

std::string moby_impl::label() const {
	return class_name();
}

uint32_t moby_impl::uid() const {
	auto data = _backing.peek<fmt::moby>(0);
	return data.uid;
}

void moby_impl::set_uid(uint32_t uid_) {
	auto data = _backing.read<fmt::moby>(0);
	data.uid = uid_;
	_backing.write<fmt::moby>(0, data);
}

uint16_t moby_impl::class_num() const {
	auto data = _backing.peek<fmt::moby>(0);
	return data.class_num;
}

void moby_impl::set_class_num(uint16_t class_num_) {
	auto data = _backing.read<fmt::moby>(0);
	data.class_num = class_num_;
	_backing.write<fmt::moby>(0, data);
}


glm::vec3 moby_impl::position() const {
	auto data = _backing.peek<fmt::moby>(0);
	return data.position.glm();
}

void moby_impl::set_position(glm::vec3 position_) {
	auto data = _backing.read<fmt::moby>(0);
	data.position = vec3f(position_);
	_backing.write<fmt::moby>(0, data);
}

glm::vec3 moby_impl::rotation() const {
	auto data = _backing.peek<fmt::moby>(0);
	return data.rotation.glm();
}

void moby_impl::set_rotation(glm::vec3 rotation_) {
	auto data = _backing.read<fmt::moby>(0);
	data.rotation = vec3f(rotation_);
	_backing.write<fmt::moby>(0, data);
}

std::string moby_impl::class_name() const {
	auto data = _backing.peek<fmt::moby>(0);
	if(class_names.find(data.class_num) != class_names.end()) {
		return class_names.at(data.class_num);
	}
	return std::to_string(data.class_num);
}

const model& moby_impl::object_model() const {
	static cube_model c;
	return c;
}

const std::map<uint16_t, const char*> moby_impl::class_names {
	{ 0x1f4, "crate" },
	{ 0x2f6, "swingshot_grapple" },
	{ 0x323, "swingshot_swinging" }
};
