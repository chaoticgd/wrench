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

#include <map>
#include <memory>

#include <core/buffer.h>
#include <cppparser/cpp_lexer.h>

enum CppTypeDescriptor
{
	CPP_ARRAY,
	CPP_BITFIELD,
	CPP_BUILT_IN,
	CPP_ENUM,
	CPP_STRUCT_OR_UNION,
	CPP_TYPE_NAME,
	CPP_POINTER_OR_REFERENCE
};

struct CppType;

struct CppArray
{
	s32 element_count = 0;
	std::unique_ptr<CppType> element_type;
};

struct CppBitField
{
	s32 bit_offset = 0;
	s32 bit_size = 0;
	std::unique_ptr<CppType> storage_unit_type;
};

inline u64 cpp_unpack_unsigned_bitfield(u64 storage_unit, s32 bit_offset, s32 bit_size)
{
	return (storage_unit >> bit_offset) & ((static_cast<u64>(1) << bit_size) - 1);
}

inline s64 cpp_unpack_signed_bitfield(u64 storage_unit, s32 bit_offset, s32 bit_size)
{
	return static_cast<s64>(storage_unit << (64 - (bit_offset + bit_size))) >> (64 - bit_size);
}

inline u64 cpp_pack_unsigned_bitfield(u64 bitfield, s32 bit_offset, s32 bit_size)
{
	return (bitfield & ((static_cast<u64>(1) << bit_size) - 1)) << bit_offset;
}

inline u64 cpp_pack_signed_bitfield(s64 bitfield, s32 bit_offset, s32 bit_size)
{
	return static_cast<u64>((bitfield & ((static_cast<u64>(1) << bit_size) - 1)) << (64 - bit_size)) >> (64 - bit_offset - bit_size);
}

inline u64 cpp_zero_bitfield(u64 storage_unit, s32 bit_offset, s32 bit_size)
{
	return storage_unit & ~(((static_cast<u64>(1) << bit_size) - 1) << bit_offset);
}

enum CppBuiltIn
{
	CPP_VOID,
	CPP_CHAR, CPP_UCHAR, CPP_SCHAR,
	CPP_SHORT, CPP_USHORT,
	CPP_INT, CPP_UINT,
	CPP_LONG, CPP_ULONG,
	CPP_LONGLONG, CPP_ULONGLONG,
	CPP_S8, CPP_U8,
	CPP_S16, CPP_U16,
	CPP_S32, CPP_U32,
	CPP_S64, CPP_U64,
	CPP_S128, CPP_U128,
	CPP_FLOAT, CPP_DOUBLE, CPP_BOOL,
	CPP_BUILT_IN_COUNT
};

inline bool cpp_is_built_in_integer(CppBuiltIn x)
{
	return x >= CPP_CHAR
		&& x <= CPP_U128;
}

inline bool cpp_is_built_in_float(CppBuiltIn x)
{
	return x == CPP_FLOAT
		|| x == CPP_DOUBLE;
}

inline bool cpp_is_built_in_signed(CppBuiltIn x)
{
	return x == CPP_CHAR
		|| x == CPP_SCHAR
		|| x == CPP_SHORT
		|| x == CPP_INT
		|| x == CPP_LONG
		|| x == CPP_LONGLONG
		|| x == CPP_S8
		|| x == CPP_S16
		|| x == CPP_S32
		|| x == CPP_S64
		|| x == CPP_S128;
	}

struct CppEnum
{
	std::vector<std::pair<s32, std::string>> constants;
};

struct CppStructOrUnion
{
	bool is_union = false;
	std::vector<CppType> fields;
};

struct CppUnion
{
	std::vector<CppType> fields;
};

struct CppTypeName
{
	std::string string;
};

struct CppPointerOrReference
{
	bool is_reference = false;
	std::unique_ptr<CppType> value_type;
};

enum CppPreprocessorDirectiveType
{
	CPP_DIRECTIVE_BCD,
	CPP_DIRECTIVE_BITFLAGS,
	CPP_DIRECTIVE_ELEMENTNAMES,
	CPP_DIRECTIVE_ENUM
};

struct CppPreprocessorDirective
{
	CppPreprocessorDirectiveType type;
	std::string string;
	
	bool operator==(const CppPreprocessorDirective& rhs) const { return type == rhs.type && string == rhs.string; }
};

struct CppType
{
	std::string name;
	s32 offset = -1;
	s32 size = -1;
	s32 alignment = -1;
	s32 precedence = -1; // Used decide if a type should be overwritten by a new type.
	std::vector<CppPreprocessorDirective> preprocessor_directives;
	CppTypeDescriptor descriptor;
	union {
		CppArray array;
		CppBitField bitfield;
		CppBuiltIn built_in;
		CppEnum enumeration;
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

struct CppABI
{
	s32 built_in_sizes[CPP_BUILT_IN_COUNT];
	s32 built_in_alignments[CPP_BUILT_IN_COUNT];
	s32 enum_size;
	s32 enum_alignment;
	s32 pointer_size;
	s32 pointer_alignment;
};

void layout_cpp_type(CppType& type, std::map<std::string, CppType>& types, const CppABI& abi);
void dump_cpp_type(OutBuffer& dest, const CppType& type);
void destructively_merge_cpp_structs(CppType& dest, CppType& src);
const char* cpp_built_in(CppBuiltIn built_in);
const CppPreprocessorDirective* cpp_directive(const CppType& type, CppPreprocessorDirectiveType directive_type);

extern CppABI NATIVE_ABI;
extern CppABI CPP_PS2_ABI;

#endif
