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

#include "wtf_glue.h"

bool read_inst_bool(const WtfNode* node, const char* name)
{
	const WtfAttribute* attrib = wtf_attribute_of_type(node, name, WTF_BOOLEAN);
	verify(attrib, "Missing '%s' field.", name);
	return (bool) attrib->boolean;
}

s32 read_inst_int(const WtfNode* node, const char* name)
{
	const WtfAttribute* attrib = wtf_attribute_of_type(node, name, WTF_NUMBER);
	verify(attrib, "Missing '%s' field.", name);
	return attrib->number.i;
}

f32 read_inst_float(const WtfNode* node, const char* name)
{
	const WtfAttribute* attrib = wtf_attribute_of_type(node, name, WTF_NUMBER);
	verify(attrib, "Missing '%s' field.", name);
	return attrib->number.f;
}

template <typename T>
T read_inst_float_list(const WtfAttribute* attrib, const char* name)
{
	verify(attrib->type == WTF_ARRAY, "Invalid '%s' field.", name);
	T dest;
	s32 index = 0;
	for (const WtfAttribute* value = attrib->first_array_element; value != nullptr; value = value->next) {
		verify(value->type == WTF_NUMBER && index < (sizeof(T) / sizeof(f32)), "Invalid '%s' attribute.", name);
		*((f32*) &dest + index++) = value->number.f;
	}
	verify(index == sizeof(T) / sizeof(f32), "Invalid '%s' field.", name);
	return dest;
}

template glm::vec3 read_inst_float_list<glm::vec3>(const WtfAttribute* attrib, const char* name);
template glm::vec4 read_inst_float_list<glm::vec4>(const WtfAttribute* attrib, const char* name);
template glm::mat3x4 read_inst_float_list<glm::mat3x4>(const WtfAttribute* attrib, const char* name);
template glm::mat4 read_inst_float_list<glm::mat4>(const WtfAttribute* attrib, const char* name);

std::vector<u8> read_inst_byte_list(const WtfAttribute* attrib, const char* name)
{
	verify(attrib, "Missing '%s' attribute.", name);
	verify(attrib->type == WTF_ARRAY, "Invalid '%s' field.", name);
	std::vector<u8> dest;
	for (const WtfAttribute* value = attrib->first_array_element; value != nullptr; value = value->next) {
		verify(value->type == WTF_NUMBER, "Invalid '%s' field.", name);
		dest.emplace_back((u8) value->number.i);
	}
	return dest;
}
