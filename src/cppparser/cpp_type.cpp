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

CppType::CppType(CppTypeDescriptor d) : descriptor(d) {
	create_pvar_type(*this);
}

CppType::CppType(CppType&& rhs) {
	name = std::move(rhs.name);
	offset = rhs.offset;
	size = rhs.size;
	alignment = rhs.alignment;
	descriptor = rhs.descriptor;
	create_pvar_type(*this);
	move_assign_pvar_type(*this, rhs);
}

CppType::~CppType() {
	destroy_pvar_type(*this);
}

CppType& CppType::operator=(CppType&& rhs) {
	if(this == &rhs) {
		return *this;
	}
	
	destroy_pvar_type(*this);
	
	name = std::move(rhs.name);
	offset = rhs.offset;
	size = rhs.size;
	alignment = rhs.alignment;
	descriptor = rhs.descriptor;
	create_pvar_type(*this);
	move_assign_pvar_type(*this, rhs);
	
	return *this;
}

static void create_pvar_type(CppType& type) {
	switch(type.descriptor) {
		case CPP_ARRAY: {
			new (&type.array) CppArray;
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

static void move_assign_pvar_type(CppType& lhs, CppType& rhs) {
	switch(lhs.descriptor) {
		case CPP_ARRAY: {
			lhs.array = std::move(rhs.array);
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

static void destroy_pvar_type(CppType& type) {
	switch(type.descriptor) {
		case CPP_ARRAY: {
			type.array.~CppArray();
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

void layout_cpp_type(CppType& type, const CppABI& abi) {
	switch(type.descriptor) {
		case CPP_ARRAY: {
			verify_fatal(type.array.element_type.get());
			layout_cpp_type(*type.array.element_type, abi);
			type.size = type.array.element_type->size * type.array.element_count;
			type.alignment = type.array.element_type->alignment;
			break;
		}
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
			s32 offset = 0;
			type.alignment = 1;
			for(CppType& field : type.struct_or_union.fields) {
				layout_cpp_type(field, abi);
				type.alignment = std::max(field.alignment, type.alignment);
				field.offset = align32(offset, field.alignment);
				if(!type.struct_or_union.is_union) {
					offset = field.offset + field.size;
				}
			}
			type.size = align32(offset, type.alignment);
			break;
		}
		case CPP_TYPE_NAME: {
			verify_not_reached_fatal("Cannot determine layout from type name.");
			break;
		}
		case CPP_POINTER_OR_REFERENCE: {
			type.size = abi.pointer_size;
			type.alignment = abi.pointer_alignment;
			break;
		}
	}
}

CppABI CPP_PS2_ABI = {
	/* built_in_sizes = */ {
		/* [CPP_CHAR] = */ 1,
		/* [CPP_UCHAR] = */ 1,
		/* [CPP_SCHAR] = */ 1,
		/* [CPP_SHORT] = */ 2,
		/* [CPP_USHORT] = */ 2,
		/* [CPP_INT] = */ 4,
		/* [CPP_UINT] = */ 4,
		/* [CPP_LONG] = */ 8,
		/* [CPP_ULONG] = */ 8,
		/* [CPP_LONGLONG] = */ 16, // Some weirdness here with these games.
		/* [CPP_ULONGLONG] = */ 16,
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
		/* [CPP_CHAR] = */ 1,
		/* [CPP_UCHAR] = */ 1,
		/* [CPP_SCHAR] = */ 1,
		/* [CPP_SHORT] = */ 2,
		/* [CPP_USHORT] = */ 2,
		/* [CPP_INT] = */ 4,
		/* [CPP_UINT] = */ 4,
		/* [CPP_LONG] = */ 8,
		/* [CPP_ULONG] = */ 8,
		/* [CPP_LONGLONG] = */ 16,
		/* [CPP_ULONGLONG] = */ 16,
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
