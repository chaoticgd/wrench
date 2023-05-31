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

#ifndef INSTANCEMGR_PVAR_H
#define INSTANCEMGR_PVAR_H

#include <memory>

#include <instancemgr/cpp_lexer.h>

enum PvarTypeDescriptor {
	PTD_ARRAY,
	PTD_BUILT_IN,
	PTD_STRUCT_OR_UNION,
	PTD_TYPE_NAME,
	PTD_POINTER_OR_REFERENCE
};

struct PvarType;

struct PvarArray {
	s32 element_count = 0;
	std::unique_ptr<PvarType> element_type;
};

enum PvarBuiltIn {
	CHAR, UCHAR, SCHAR, SHORT, USHORT, INT, UINT, LONG, ULONG, LONGLONG, ULONGLONG,
	S8, U8, S16, U16, S32, U32, S64, U64, S128, U128,
	FLOAT, DOUBLE, BOOL,
	VEC2, VEC3, VEC4,
	// Create built-in types for links to each instance type.
#define DEF_INSTANCE(inst_type, inst_variable) inst_type##Link,
#define GENERATED_INSTANCE_MACRO_CALLS
#include "_generated_instance_types.inl"
#undef GENERATED_INSTANCE_MACRO_CALLS
#undef DEF_INSTANCE
};

struct PvarStructOrUnion {
	bool is_union = false;
	std::vector<PvarType> fields;
};

struct PvarUnion {
	std::vector<PvarType> fields;
};

struct PvarTypeName {
	std::string string;
};

struct PvarPointerOrReference {
	bool is_reference = false;
	std::unique_ptr<PvarType> value_type;
};

struct PvarType {
	std::string name;
	s32 offset = -1;
	s32 size = -1;
	s32 alignment = -1;
	PvarTypeDescriptor descriptor;
	union {
		PvarArray array;
		PvarBuiltIn built_in;
		PvarStructOrUnion struct_or_union;
		PvarTypeName type_name;
		PvarPointerOrReference pointer_or_reference;
	};
	
	PvarType(PvarTypeDescriptor d);
	PvarType(const PvarType& rhs) = delete;
	PvarType(PvarType&& rhs);
	~PvarType();
	PvarType& operator=(const PvarType& rhs) = delete;
	PvarType& operator=(PvarType&& rhs);
};

std::vector<PvarType> parse_pvar_types(const std::vector<CppToken>& tokens);
void layout_pvar_type(PvarType& type);

#endif
