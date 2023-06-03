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

#ifndef CPPPARSER_CPP_TYPE_H
#define CPPPARSER_CPP_TYPE_H

#include <memory>

#include <cppparser/cpp_lexer.h>

enum CppTypeDescriptor {
	CPP_ARRAY,
	CPP_BUILT_IN,
	CPP_STRUCT_OR_UNION,
	CPP_TYPE_NAME,
	CPP_POINTER_OR_REFERENCE
};

struct CppType;

struct CppArray {
	s32 element_count = 0;
	std::unique_ptr<CppType> element_type;
};

enum CppBuiltIn {
	CHAR, UCHAR, SCHAR, SHORT, USHORT, INT, UINT, LONG, ULONG, LONGLONG, ULONGLONG,
	S8, U8, S16, U16, S32, U32, S64, U64, S128, U128,
	FLOAT, DOUBLE, BOOL
};

struct CppStructOrUnion {
	bool is_union = false;
	std::vector<CppType> fields;
};

struct CppUnion {
	std::vector<CppType> fields;
};

struct CppTypeName {
	std::string string;
};

struct CppPointerOrReference {
	bool is_reference = false;
	std::unique_ptr<CppType> value_type;
};

struct CppType {
	std::string name;
	s32 offset = -1;
	s32 size = -1;
	s32 alignment = -1;
	CppTypeDescriptor descriptor;
	union {
		CppArray array;
		CppBuiltIn built_in;
		CppStructOrUnion struct_or_union;
		CppTypeName type_name;
		CppPointerOrReference pointer_or_reference;
	};
	
	CppType(CppTypeDescriptor d);
	CppType(const CppType& rhs) = delete;
	CppType(CppType&& rhs);
	~CppType();
	CppType& operator=(const CppType& rhs) = delete;
	CppType& operator=(CppType&& rhs);
};

std::vector<CppType> parse_cpp_types(const std::vector<CppToken>& tokens);
void layout_cpp_type(CppType& type);

#endif
