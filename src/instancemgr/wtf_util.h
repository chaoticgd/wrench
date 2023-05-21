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

#ifndef INSTANCEMGR_WTF_UTIL_H
#define INSTANCEMGR_WTF_UTIL_H

#include <core/util.h>
#include <wtf/wtf.h>
#include <wtf/wtf_writer.h>
#include <engine/basic_types.h>

static bool read_inst_bool(const WtfNode* node, const char* name) {
	const WtfAttribute* attrib = wtf_attribute_of_type(node, name, WTF_BOOLEAN);
	verify(attrib, "Missing '%s' attribute.", name);
	return (bool) attrib->boolean;
}

static s32 read_inst_int(const WtfNode* node, const char* name) {
	const WtfAttribute* attrib = wtf_attribute_of_type(node, name, WTF_NUMBER);
	verify(attrib, "Missing '%s' attribute.", name);
	return attrib->number.i;
}

static f32 read_inst_float(const WtfNode* node, const char* name) {
	const WtfAttribute* attrib = wtf_attribute_of_type(node, name, WTF_NUMBER);
	verify(attrib, "Missing '%s' attribute.", name);
	return attrib->number.f;
}

template <typename T>
static T read_inst_float_list(const WtfAttribute* attrib, const char* name) {
	T dest;
	s32 index = 0;
	for(const WtfAttribute* value = attrib->first_array_element; value != nullptr; value = value->next) {
		verify(value->type == WTF_NUMBER && index < (sizeof(T) / sizeof(f32)), "Invalid '%s' attribute.", name);
		*((f32*) &dest + index++) = value->number.f;
	}
	verify(index == sizeof(T) / sizeof(f32), "Invalid '%s' attribute.", name);
	return dest;
}

template <typename T>
static T read_inst_float_list(const WtfNode* node, const char* name) {
	const WtfAttribute* attrib = wtf_attribute_of_type(node, name, WTF_ARRAY);
	verify(attrib, "Missing '%s' attribute.", name);
	return read_inst_float_list<T>(attrib, name);
}

template <typename T>
static void write_inst_float_list(WtfWriter* ctx, const char* name, const T& value) {
	wtf_begin_attribute(ctx, name);
	wtf_write_floats(ctx, (f32*) &value, sizeof(T) / sizeof(f32));
	wtf_end_attribute(ctx);
}

static std::vector<u8> read_inst_byte_list(const WtfNode* node, const char* name) {
	const WtfAttribute* attrib = wtf_attribute_of_type(node, name, WTF_ARRAY);
		verify(attrib, "Missing '%s' attribute.", name);
	std::vector<u8> dest;
	for(const WtfAttribute* value = attrib->first_array_element; value != nullptr; value = value->next) {
		verify(value->type == WTF_NUMBER, "Invalid %s.", name);
		dest.emplace_back((u8) value->number.i);
	}
	return dest;
}

static void write_inst_byte_list(WtfWriter* ctx, const char* name, const std::vector<u8>& bytes) {
	wtf_begin_attribute(ctx, name);
	wtf_write_bytes(ctx, bytes.data(), bytes.size());
	wtf_end_attribute(ctx);
}

template <typename T>
static void read_inst_field(T& dest, const WtfNode* src, const char* name) {
	if constexpr(std::is_same_v<T, bool>) {
		dest = read_inst_bool(src, name);
	} else if constexpr(std::is_integral_v<T>) {
		dest = read_inst_int(src, name);
	} else if constexpr(std::is_floating_point_v<T>) {
		dest = read_inst_float(src, name);
	} else if constexpr(std::is_same_v<T, glm::vec3> || std::is_same_v<T, glm::vec4> || std::is_same_v<T, glm::vec4>) {
		dest = read_inst_float_list<T>(src, name);
	}
}

template <typename T>
static void write_inst_field(WtfWriter* ctx, const char* name, const T& value) {
	if constexpr(std::is_same_v<T, bool>) {
		wtf_write_boolean_attribute(ctx, name, value);
	} else if constexpr(std::is_integral_v<T>) {
		wtf_write_integer_attribute(ctx, name, value);
	} else if constexpr(std::is_floating_point_v<T>) {
		wtf_write_float_attribute(ctx, name, value);
	} else if constexpr(std::is_same_v<T, glm::vec3> || std::is_same_v<T, glm::vec4> || std::is_same_v<T, glm::vec4>) {
		write_inst_float_list(ctx, name, value);
	}
}

#endif
