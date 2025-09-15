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

#include "cpp_type.h"

static void create_pvar_type(CppType& type);
static void move_assign_pvar_type(CppType& lhs, CppType& rhs);
static void destroy_pvar_type(CppType& type);

CppType::CppType(CppTypeDescriptor d)
	: descriptor(d)
{
	create_pvar_type(*this);
}

CppType::CppType(CppType&& rhs)
{
	name = std::move(rhs.name);
	offset = rhs.offset;
	size = rhs.size;
	alignment = rhs.alignment;
	precedence = rhs.precedence;
	preprocessor_directives = std::move(rhs.preprocessor_directives);
	descriptor = rhs.descriptor;
	create_pvar_type(*this);
	move_assign_pvar_type(*this, rhs);
}

CppType::~CppType()
{
	destroy_pvar_type(*this);
}

CppType& CppType::operator=(CppType&& rhs)
{
	if (this == &rhs) {
		return *this;
	}
	
	destroy_pvar_type(*this);
	
	name = std::move(rhs.name);
	offset = rhs.offset;
	size = rhs.size;
	alignment = rhs.alignment;
	precedence = rhs.precedence;
	preprocessor_directives = std::move(rhs.preprocessor_directives);
	descriptor = rhs.descriptor;
	create_pvar_type(*this);
	move_assign_pvar_type(*this, rhs);
	
	return *this;
}

static void create_pvar_type(CppType& type)
{
	switch (type.descriptor) {
		case CPP_ARRAY: {
			new (&type.array) CppArray;
			break;
		}
		case CPP_BITFIELD: {
			new (&type.bitfield) CppBitField;
			break;
		}
		case CPP_BUILT_IN: {
			new (&type.built_in) CppBuiltIn;
			break;
		}
		case CPP_ENUM: {
			new (&type.enumeration) CppEnum;
			break;
		}
		case CPP_STRUCT_OR_UNION: {
			new (&type.struct_or_union) CppStructOrUnion;
			break;
		}
		case CPP_TYPE_NAME: {
			new (&type.type_name) CppTypeName;
			break;
		}
		case CPP_POINTER_OR_REFERENCE: {
			new (&type.pointer_or_reference) CppPointerOrReference;
			break;
		}
	}
}

static void move_assign_pvar_type(CppType& lhs, CppType& rhs)
{
	switch (lhs.descriptor) {
		case CPP_ARRAY: {
			lhs.array = std::move(rhs.array);
			break;
		}
		case CPP_BITFIELD: {
			lhs.bitfield = std::move(rhs.bitfield);
			break;
		}
		case CPP_BUILT_IN: {
			lhs.built_in = std::move(rhs.built_in);
			break;
		}
		case CPP_ENUM: {
			lhs.enumeration = std::move(rhs.enumeration);
			break;
		}
		case CPP_STRUCT_OR_UNION: {
			lhs.struct_or_union = std::move(rhs.struct_or_union);
			break;
		}
		case CPP_TYPE_NAME: {
			lhs.type_name = std::move(rhs.type_name);
			break;
		}
		case CPP_POINTER_OR_REFERENCE: {
			lhs.pointer_or_reference = std::move(rhs.pointer_or_reference);
			break;
		}
	}
}

static void destroy_pvar_type(CppType& type)
{
	switch (type.descriptor) {
		case CPP_ARRAY: {
			type.array.~CppArray();
			break;
		}
		case CPP_BITFIELD: {
			type.bitfield.~CppBitField();
			break;
		}
		case CPP_BUILT_IN: {
			type.built_in.~CppBuiltIn();
			break;
		}
		case CPP_ENUM: {
			type.enumeration.~CppEnum();
			break;
		}
		case CPP_STRUCT_OR_UNION: {
			type.struct_or_union.~CppStructOrUnion();
			break;
		}
		case CPP_TYPE_NAME: {
			type.type_name.~CppTypeName();
			break;
		}
		case CPP_POINTER_OR_REFERENCE: {
			type.pointer_or_reference.~CppPointerOrReference();
			break;
		}
	}
}

// *****************************************************************************

void layout_cpp_type(CppType& type, std::map<std::string, CppType>& types, const CppABI& abi)
{
	switch (type.descriptor) {
		case CPP_ARRAY: {
			verify_fatal(type.array.element_type.get());
			layout_cpp_type(*type.array.element_type, types, abi);
			type.size = type.array.element_type->size * type.array.element_count;
			type.alignment = type.array.element_type->alignment;
			break;
		}
		case CPP_BITFIELD: {
			verify_fatal(type.bitfield.storage_unit_type.get());
			layout_cpp_type(*type.bitfield.storage_unit_type, types, abi);
			type.size = type.bitfield.storage_unit_type->size;
			type.alignment = type.bitfield.storage_unit_type->alignment;
			break;
		};
		case CPP_BUILT_IN: {
			verify_fatal(type.built_in < CPP_BUILT_IN_COUNT);
			type.size = abi.built_in_sizes[type.built_in];
			type.alignment = abi.built_in_alignments[type.built_in];
			break;
		}
		case CPP_ENUM: {
			type.size = abi.enum_size;
			type.alignment = abi.enum_alignment;
			break;
		}
		case CPP_STRUCT_OR_UNION: {
			for (CppType& field : type.struct_or_union.fields) {
				if (field.descriptor == CPP_BITFIELD) {
					verify(!type.struct_or_union.is_union, "Union '%s' contains a bitfield.", type.name.c_str());
				}
			}
			
			s32 bit_offset = 0;
			
			bool has_custom_alignment = type.alignment > -1;
			s32 offset = 0;
			if (!has_custom_alignment) {
				type.alignment = 1;
			}
			for (size_t i = 0; i < type.struct_or_union.fields.size(); i++) {
				CppType& field = type.struct_or_union.fields[i];
				
				layout_cpp_type(field, types, abi);
				if (!has_custom_alignment) {
					type.alignment = std::max(field.alignment, type.alignment);
				}
				field.offset = align32(offset, field.alignment);
				
				bool add_offset = !type.struct_or_union.is_union;
				if (field.descriptor == CPP_BITFIELD) {
					// Check if this is the last bitfield for the storage unit.
					bool end_of_group = true;
					if (i + 1 < type.struct_or_union.fields.size()) {
						CppType& next_field = type.struct_or_union.fields[i + 1];
						end_of_group = field.descriptor != next_field.descriptor
							|| (field.descriptor == CPP_BUILT_IN && field.built_in != next_field.built_in)
							|| (bit_offset + field.bitfield.bit_size >= field.bitfield.storage_unit_type->size * 8);
					}
					
					field.bitfield.bit_offset = bit_offset;
					bit_offset += field.bitfield.bit_size;
					
					if (end_of_group) {
						verify(bit_offset == field.bitfield.storage_unit_type->size * 8,
							"Sum of bitfield sizes (%d) not equal to size of storage unit (%d) for type '%s'.",
							bit_offset, field.bitfield.storage_unit_type->size * 8, type.name.c_str());
						bit_offset = 0;
					}
					
					add_offset &= end_of_group;
				}
				
				if (add_offset) {
					offset = field.offset + field.size;
				}
			}
			type.size = std::max(align32(offset, type.alignment), 1);
			break;
		}
		case CPP_TYPE_NAME: {
			auto iter = types.find(type.type_name.string);
			verify(iter != types.end(), "Failed to lookup type '%s'.", type.type_name.string.c_str());
			if (iter->second.size < 0 || iter->second.alignment < 0) {
				layout_cpp_type(iter->second, types, abi);
			}
			type.size = iter->second.size;
			type.alignment = iter->second.alignment;
			break;
		}
		case CPP_POINTER_OR_REFERENCE: {
			type.size = abi.pointer_size;
			type.alignment = abi.pointer_alignment;
			break;
		}
	}
}

// State passed down to recursive dump_cpp_type calls.
struct CppDumpContext
{
	const char* name = nullptr;
	std::vector<char> pointers;
	std::vector<s32> array_subscripts;
	s32 depth = 0;
	s32 indentation = 0;
	s32 offset = 0;
	s32 digits_for_offset = 3;
};

static void dump_cpp_type_impl(
	OutBuffer& dest, const CppType& type, const CppDumpContext& parent_context);
static void dump_pointers_name_and_subscripts(OutBuffer& dest, CppDumpContext& context);
static void indent_cpp(OutBuffer& dest, const CppDumpContext& context);

void dump_cpp_type(OutBuffer& dest, const CppType& type)
{
	CppDumpContext context;
	if (type.size > -1) {
		context.digits_for_offset = (s32) ceilf(log2(type.size) / 4.f);
	}
	dump_cpp_type_impl(dest, type, context);
}

static void dump_cpp_type_impl(
	OutBuffer& dest, const CppType& type, const CppDumpContext& parent_context)
{
	CppDumpContext context = parent_context;
	if (!type.name.empty()) {
		context.name = type.name.c_str();
	}
	context.depth++;
	
	switch (type.descriptor) {
		case CPP_ARRAY: {
			context.array_subscripts.emplace_back(type.array.element_count);
			verify_fatal(type.array.element_type.get());
			dump_cpp_type_impl(dest, *type.array.element_type, context);
			break;
		}
		case CPP_BITFIELD: {
			verify_not_reached("Dumping bitfields not yet supported.");
			break;
		}
		case CPP_BUILT_IN: {
			verify_fatal(type.built_in < CPP_BUILT_IN_COUNT);
			dest.writesf("%s", cpp_built_in(type.built_in));
			dump_pointers_name_and_subscripts(dest, context);
			dest.writesf(";");
			break;
		}
		case CPP_ENUM: {
			if (context.depth == 1 && context.name) {
				dest.writelf("enum %s {", context.name);
			} else {
				dest.writelf("enum {");
			}
			context.indentation++;
			for (size_t i = 0; i < type.enumeration.constants.size(); i++) {
				auto& [number, name] = type.enumeration.constants[i];
				indent_cpp(dest, context);
				dest.writelf("%s = %d%s", name.c_str(), number, i + 1 == type.enumeration.constants.size());
			}
			context.indentation--;
			indent_cpp(dest, context);
			if (context.depth == 1 || !context.name) {
				dest.writelf("};");
			} else {
				dest.writelf("} %s;", context.name);
			}
			break;
		}
		case CPP_STRUCT_OR_UNION: {
			const char* keyword = type.struct_or_union.is_union ? "union" : "struct";
			if (context.depth == 1 && context.name) {
				dest.writesf("%s %s {", keyword, context.name);
			} else {
				dest.writesf("%s {", keyword);
			}
			if (type.size > -1) {
				dest.writelf(" // 0x%x", type.size);
			} else {
				dest.write<char>('\n');
			}
			context.indentation++;
			for (const CppType& field : type.struct_or_union.fields) {
				indent_cpp(dest, context);
				if (field.offset > -1) {
					dest.writesf("/* 0x%0*x */ ", context.digits_for_offset, context.offset + field.offset);
				}
				dump_cpp_type_impl(dest, field, context);
				dest.writesf("\n");
			}
			context.indentation--;
			indent_cpp(dest, context);
			if (context.depth == 1 || !context.name) {
				dest.writelf("};");
			} else {
				dest.writelf("} %s;", context.name);
			}
			break;
		}
		case CPP_TYPE_NAME: {
			dest.writesf("%s", type.type_name.string.c_str());
			dump_pointers_name_and_subscripts(dest, context);
			dest.writesf(";");
			break;
		}
		case CPP_POINTER_OR_REFERENCE: {
			context.pointers.emplace_back(type.pointer_or_reference.is_reference ? '&' : '*');
			verify_fatal(type.array.element_type.get());
			dump_cpp_type_impl(dest, *type.array.element_type, context);
			break;
		}
	}
}

static void dump_pointers_name_and_subscripts(OutBuffer& dest, CppDumpContext& context)
{
	dest.writesf(" ");
	for (size_t i = context.pointers.size(); i > 0; i--) {
		dest.writesf("%c", context.pointers[i - 1]);
	}
	context.pointers.clear();
	if (context.name) {
		dest.writesf("%s", context.name);
		context.name = nullptr;
	}
	for (s32 subscript : context.array_subscripts) {
		dest.writesf("[%d]", subscript);
	}
	context.array_subscripts.clear();
}

static void indent_cpp(OutBuffer& dest, const CppDumpContext& context)
{
	for (s32 i = 0; i < context.indentation; i++) {
		dest.writesf("\t");
	}
}


void destructively_merge_cpp_structs(CppType& dest, CppType& src)
{
	verify_fatal(dest.name == src.name);
	verify_fatal(dest.descriptor == CPP_STRUCT_OR_UNION && !dest.struct_or_union.is_union);
	verify_fatal(src.descriptor == CPP_STRUCT_OR_UNION && !src.struct_or_union.is_union);
	for (CppType& dest_field : dest.struct_or_union.fields) {
		if (dest_field.name.starts_with("unknown")) {
			for (CppType& src_field : src.struct_or_union.fields) {
				if (src_field.offset == dest_field.offset && src_field.size == dest_field.size) {
					dest_field = std::move(src_field);
				}
			}
		}
	}
}

const char* cpp_built_in(CppBuiltIn built_in)
{
	switch (built_in) {
		case CPP_VOID: return "void";
		case CPP_CHAR: return "char";
		case CPP_UCHAR: return "unsigned char";
		case CPP_SCHAR: return "signed char";
		case CPP_SHORT: return "short";
		case CPP_USHORT: return "unsigned short";
		case CPP_INT: return "int";
		case CPP_UINT: return "unsigned int";
		case CPP_LONG: return "long";
		case CPP_ULONG: return "unsigned long";
		case CPP_LONGLONG: return "long long";
		case CPP_ULONGLONG: return "unsigned long long";
		case CPP_S8: return "s8";
		case CPP_U8: return "u8";
		case CPP_S16: return "s16";
		case CPP_U16: return "u16";
		case CPP_S32: return "s32";
		case CPP_U32: return "u32";
		case CPP_S64: return "s64";
		case CPP_U64: return "u64";
		case CPP_S128: return "s128";
		case CPP_U128: return "u128";
		case CPP_FLOAT: return "float";
		case CPP_DOUBLE: return "double";
		case CPP_BOOL: return "bool";
		default: {}
	}
	return "error";
}

const CppPreprocessorDirective* cpp_directive(
	const CppType& type, CppPreprocessorDirectiveType directive_type)
{
	for (const CppPreprocessorDirective& directive : type.preprocessor_directives) {
		if (directive.type == directive_type) {
			return &directive;
		}
	}
	return nullptr;
}

enum CppDummyEnum {};

CppABI NATIVE_ABI = {
	/* built_in_sizes = */ {
		/* [CPP_VOID] = */ 1,
		/* [CPP_CHAR] = */ sizeof(char),
		/* [CPP_UCHAR] = */ sizeof(unsigned char),
		/* [CPP_SCHAR] = */ sizeof(signed char),
		/* [CPP_SHORT] = */ sizeof(short),
		/* [CPP_USHORT] = */ sizeof(unsigned short),
		/* [CPP_INT] = */ sizeof(int),
		/* [CPP_UINT] = */ sizeof(unsigned int),
		/* [CPP_LONG] = */ sizeof(long),
		/* [CPP_ULONG] = */ sizeof(unsigned long),
		/* [CPP_LONGLONG] = */ sizeof(long long),
		/* [CPP_ULONGLONG] = */ sizeof(unsigned long long),
		/* [CPP_S8] = */ 1,
		/* [CPP_U8] = */ 1,
		/* [CPP_S16] = */ 2,
		/* [CPP_U16] = */ 2,
		/* [CPP_S32] = */ 4,
		/* [CPP_U32] = */ 4,
		/* [CPP_S64] = */ 8,
		/* [CPP_U64] = */ 8,
		/* [CPP_S128] = */ 16,
		/* [CPP_U128] = */ 16,
		/* [CPP_FLOAT] = */ sizeof(float),
		/* [CPP_DOUBLE] = */ sizeof(double),
		/* [CPP_BOOL] = */ sizeof(bool),
	},
	/* built_in_alignments */ {
		/* [CPP_VOID] = */ 1,
		/* [CPP_CHAR] = */ alignof(char),
		/* [CPP_UCHAR] = */ alignof(unsigned char),
		/* [CPP_SCHAR] = */ alignof(signed char),
		/* [CPP_SHORT] = */ alignof(short),
		/* [CPP_USHORT] = */ alignof(unsigned short),
		/* [CPP_INT] = */ alignof(int),
		/* [CPP_UINT] = */ alignof(unsigned int),
		/* [CPP_LONG] = */ alignof(long),
		/* [CPP_ULONG] = */ alignof(unsigned long),
		/* [CPP_LONGLONG] = */ alignof(long long),
		/* [CPP_ULONGLONG] = */ alignof(unsigned long long),
		/* [CPP_S8] = */ 1,
		/* [CPP_U8] = */ 1,
		/* [CPP_S16] = */ 2,
		/* [CPP_U16] = */ 2,
		/* [CPP_S32] = */ 4,
		/* [CPP_U32] = */ 4,
		/* [CPP_S64] = */ 8,
		/* [CPP_U64] = */ 8,
		/* [CPP_S128] = */ 16,
		/* [CPP_U128] = */ 16,
		/* [CPP_FLOAT] = */ alignof(float),
		/* [CPP_DOUBLE] = */ alignof(double),
		/* [CPP_BOOL] = */ alignof(bool),
	},
	/* enum_size = */ sizeof(CppDummyEnum),
	/* enum_alignment = */ alignof(CppDummyEnum),
	/* pointer_size = */ sizeof(void*),
	/* pointer_alignment = */ alignof(void*),
};

CppABI CPP_PS2_ABI = {
	/* built_in_sizes = */ {
		/* [CPP_VOID] = */ 1,
		/* [CPP_CHAR] = */ 1,
		/* [CPP_UCHAR] = */ 1,
		/* [CPP_SCHAR] = */ 1,
		/* [CPP_SHORT] = */ 2,
		/* [CPP_USHORT] = */ 2,
		/* [CPP_INT] = */ 4,
		/* [CPP_UINT] = */ 4,
		/* [CPP_LONG] = */ 8,
		/* [CPP_ULONG] = */ 8,
		/* [CPP_LONGLONG] = */ 8,
		/* [CPP_ULONGLONG] = */ 8,
		/* [CPP_S8] = */ 1,
		/* [CPP_U8] = */ 1,
		/* [CPP_S16] = */ 2,
		/* [CPP_U16] = */ 2,
		/* [CPP_S32] = */ 4,
		/* [CPP_U32] = */ 4,
		/* [CPP_S64] = */ 8,
		/* [CPP_U64] = */ 8,
		/* [CPP_S128] = */ 16,
		/* [CPP_U128] = */ 16,
		/* [CPP_FLOAT] = */ 4,
		/* [CPP_DOUBLE] = */ 8,
		/* [CPP_BOOL] = */ 1,
	},
	/* built_in_alignments */ {
		/* [CPP_VOID] = */ 1,
		/* [CPP_CHAR] = */ 1,
		/* [CPP_UCHAR] = */ 1,
		/* [CPP_SCHAR] = */ 1,
		/* [CPP_SHORT] = */ 2,
		/* [CPP_USHORT] = */ 2,
		/* [CPP_INT] = */ 4,
		/* [CPP_UINT] = */ 4,
		/* [CPP_LONG] = */ 8,
		/* [CPP_ULONG] = */ 8,
		/* [CPP_LONGLONG] = */ 8,
		/* [CPP_ULONGLONG] = */ 8,
		/* [CPP_S8] = */ 1,
		/* [CPP_U8] = */ 1,
		/* [CPP_S16] = */ 2,
		/* [CPP_U16] = */ 2,
		/* [CPP_S32] = */ 4,
		/* [CPP_U32] = */ 4,
		/* [CPP_S64] = */ 8,
		/* [CPP_U64] = */ 8,
		/* [CPP_S128] = */ 16,
		/* [CPP_U128] = */ 16,
		/* [CPP_FLOAT] = */ 4,
		/* [CPP_DOUBLE] = */ 8,
		/* [CPP_BOOL] = */ 1,
	},
	/* enum_size = */ 4,
	/* enum_alignment = */ 4,
	/* pointer_size = */ 4,
	/* pointer_alignment = */ 4,
};
