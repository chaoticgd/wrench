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

#ifndef INSTANCEMGR_WTF_GLUE_H
#define INSTANCEMGR_WTF_GLUE_H

#include <core/util.h>
#include <wtf/wtf.h>
#include <wtf/wtf_writer.h>
#include <engine/basic_types.h>
#include <instancemgr/instance.h>

bool read_inst_bool(const WtfNode* node, const char* name);
s32 read_inst_int(const WtfNode* node, const char* name);
f32 read_inst_float(const WtfNode* node, const char* name);
template <typename T> T read_inst_float_list(const WtfAttribute* attrib, const char* name);
std::vector<u8> read_inst_byte_list(const WtfAttribute* attrib, const char* name);

template <typename Dest>
static void read_inst_attrib(Dest& dest, const WtfAttribute* src, const char* name)
{
	if constexpr(std::is_same_v<Dest, bool>) {
		verify(src->type == WTF_BOOLEAN, "Invalid '%s' field.", name);
		dest = src->boolean;
	} else if constexpr(std::is_integral_v<Dest>) {
		verify(src->type == WTF_NUMBER, "Invalid '%s' field.", name);
		dest = src->number.i;
	} else if constexpr(std::is_floating_point_v<Dest>) {
		verify(src->type == WTF_NUMBER, "Invalid '%s' field.", name);
		dest = src->number.f;
	} else if constexpr(std::is_same_v<Dest, glm::vec3>
			|| std::is_same_v<Dest, glm::vec4>
			|| std::is_same_v<Dest, glm::mat3x4>
			|| std::is_same_v<Dest, glm::mat4>) {
		dest = read_inst_float_list<Dest>(src, name);
	} else if constexpr(std::is_same_v<Dest, std::vector<u8>>) {
		dest = read_inst_byte_list(src, name);
	} else if constexpr(IsVector<Dest>::value) {
		verify(src->type == WTF_ARRAY, "Invalid '%s' field.", name);
		dest.clear();
		for(WtfAttribute* element = src->first_array_element; element != nullptr; element = element->next) {
			auto& element_dest = dest.emplace_back();
			read_inst_attrib(element_dest, element, name);
		}
	} else if constexpr(std::is_base_of_v<InstanceLink, Dest>) {
		read_inst_attrib(dest.id, src, name);
	} else {
		verify_not_reached_fatal("Tried to read unknown type from the instances file.");
	}
}

template <typename T>
static void read_inst_field(T& dest, const WtfNode* src, const char* name)
{
	const WtfAttribute* attrib = wtf_attribute(src, name);
	verify(attrib, "Missing '%s' field.", name);
	read_inst_attrib(dest, attrib, name);
}

template <typename Value>
static void write_inst_attrib(WtfWriter* ctx, const Value& value)
{
	if constexpr(std::is_same_v<Value, bool>) {
		wtf_write_boolean(ctx, value);
	} else if constexpr(std::is_integral_v<Value>) {
		wtf_write_integer(ctx, value);
	} else if constexpr(std::is_floating_point_v<Value>) {
		wtf_write_float(ctx, value);
	} else if constexpr(std::is_same_v<Value, glm::vec3>
			|| std::is_same_v<Value, glm::vec4>
			|| std::is_same_v<Value, glm::mat3x4>
			|| std::is_same_v<Value, glm::mat4>) {
		wtf_write_floats(ctx, (f32*) &value, sizeof(Value) / sizeof(f32));
	} else if constexpr(std::is_same_v<Value, std::vector<u8>>) {
		wtf_write_bytes(ctx, value.data(), value.size());
	} else if constexpr(IsVector<Value>::value) {
		wtf_begin_array(ctx);
		for(const auto& element : value) {
			write_inst_attrib(ctx, element);
		}
		wtf_end_array(ctx);
	} else if constexpr(std::is_base_of_v<InstanceLink, Value>) {
		wtf_write_integer(ctx, value.id);
	} else {
		verify_not_reached_fatal("Tried to write unknown type to the instances file.");
	}
}

template <typename T>
static void write_inst_field(WtfWriter* ctx, const char* name, const T& value)
{
	wtf_begin_attribute(ctx, name);
	write_inst_attrib(ctx, value);
	wtf_end_attribute(ctx);
}

#endif
