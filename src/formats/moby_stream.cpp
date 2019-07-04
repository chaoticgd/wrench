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

#include "moby_stream.h"

moby_stream::moby_stream(stream* moby_table, uint32_t moby_offset)
	: proxy_stream(moby_table, moby_offset, 0x88, "Moby") {}

std::string moby_stream::label() const {
	return class_name();
}

uint32_t moby_stream::uid() const {
	auto data = read_c<fmt::moby>(0);
	return data.uid;
}

void moby_stream::set_uid(uint32_t uid_) {
	auto data = read<fmt::moby>(0);
	data.uid = uid_;
	write<fmt::moby>(0, data);
}

uint16_t moby_stream::class_num() const {
	auto data = read_c<fmt::moby>(0);
	return data.class_num;
}

void moby_stream::set_class_num(uint16_t class_num_) {
	auto data = read<fmt::moby>(0);
	data.class_num = class_num_;
	write<fmt::moby>(0, data);
}


glm::vec3 moby_stream::position() const {
	auto data = read_c<fmt::moby>(0);
	return data.position.glm();
}

void moby_stream::set_position(glm::vec3 position_) {
	auto data = read<fmt::moby>(0);
	data.position = fmt::vec3f(position_);
	write<fmt::moby>(0, data);
}

glm::vec3 moby_stream::rotation() const {
	auto data = read_c<fmt::moby>(0);
	return { data.rotation.x, data.rotation.y, data.rotation.z };
}

void moby_stream::set_rotation(glm::vec3 rotation_) {
	auto data = read<fmt::moby>(0);
	data.rotation = fmt::vec3f(rotation_);
	write<fmt::moby>(0, data);
}

std::string moby_stream::class_name() const {
	auto data = read_c<fmt::moby>(0);
	if(class_names.find(data.class_num) != class_names.end()) {
		return class_names.at(data.class_num);
	}
	return std::to_string(data.class_num);
}

const std::map<uint16_t, const char*> moby_stream::class_names {
	{ 0x1f4, "crate" },
	{ 0x2f6, "swingshot_grapple" },
	{ 0x323, "swingshot_swinging" }
};

moby_provider::moby_provider(stream* moby_segment, uint32_t moby_table_offset)
	: proxy_stream(moby_segment, moby_table_offset, -1, "Mobies") {}

void moby_provider::populate(app* a) {
	auto header = read<fmt::table_header>(0);
	
	for(uint32_t i = 0; i < header.num_mobies; i++) {
		emplace_child<moby_stream>(this, sizeof(fmt::table_header) + i * 0x88);
	}
	stream::populate(a);
}
