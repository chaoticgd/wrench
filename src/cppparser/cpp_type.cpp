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

static void parse_struct_or_union(CppType& dest, const std::vector<CppToken>& tokens, size_t& pos);
static CppType parse_type_name(const std::vector<CppToken>& tokens, size_t& pos);
static bool check_eof(const std::vector<CppToken>& tokens, size_t& pos, const char* thing = nullptr);

std::vector<CppType> parse_cpp_types(const std::vector<CppToken>& tokens) {
	std::vector<CppType> types;
	size_t pos = 0;
	while(pos < tokens.size()) {
		if(tokens[pos].type == CPP_KEYWORD && (tokens[pos].keyword == CPP_KEYWORD_struct || tokens[pos].keyword == CPP_KEYWORD_union)) {
			if(pos + 1 < tokens.size() && tokens[pos + 1].type == CPP_IDENTIFIER) {
				if(pos + 2 < tokens.size() && tokens[pos + 2].type == CPP_OPERATOR && tokens[pos + 2].op == CPP_OP_OPENING_CURLY) {
					CppType& type = types.emplace_back(CPP_STRUCT_OR_UNION);
					type.struct_or_union.is_union = tokens[pos].keyword == CPP_KEYWORD_union;
					type.name = std::string(tokens[pos + 1].str_begin, tokens[pos + 1].str_end);
					pos += 3;
					parse_struct_or_union(type, tokens, pos);
					continue;
				}
			}
		}
		pos++;
	}
	return types;
}

static void parse_struct_or_union(CppType& dest, const std::vector<CppToken>& tokens, size_t& pos) {
	while(check_eof(tokens, pos) && !(tokens[pos].type == CPP_OPERATOR && tokens[pos].op == CPP_OP_CLOSING_CURLY)) {
		CppType field_type = parse_type_name(tokens, pos);
		
		// Parse pointers.
		while(check_eof(tokens, pos) && tokens[pos].type == CPP_OPERATOR && (tokens[pos].op == CPP_OP_STAR || tokens[pos].op == CPP_OP_AMPERSAND)) {
			CppType type(CPP_POINTER_OR_REFERENCE);
			type.pointer_or_reference.is_reference = tokens[pos].op == CPP_OP_AMPERSAND;
			type.pointer_or_reference.value_type = std::make_unique<CppType>(std::move(field_type));
			field_type = std::move(type);
			pos++;
		}
		
		check_eof(tokens, pos);
		std::string name = std::string(tokens[pos].str_begin, tokens[pos].str_end);
		pos++;
		
		// Parse array subscripts.
		std::vector<s32> array_indices;
		while(check_eof(tokens, pos) && tokens[pos].type == CPP_OPERATOR && tokens[pos].op == CPP_OP_OPENING_SQUARE) {
			pos++;
			check_eof(tokens, pos);
			verify(tokens[pos].type == CPP_INTEGER_LITERAL, "Expected integer literal.");
			array_indices.emplace_back(tokens[pos].i);
			pos++;
			check_eof(tokens, pos);
			verify(tokens[pos].type == CPP_OPERATOR && tokens[pos].op == CPP_OP_CLOSING_SQUARE, "Expected ']'.");
			pos++;
		}
		for(size_t i = array_indices.size(); i > 0; i--) {
			CppType array_type(CPP_ARRAY);
			array_type.array.element_count = array_indices[i - 1];
			array_type.array.element_type = std::make_unique<CppType>(std::move(field_type));
			field_type = std::move(array_type);
		}
		
		field_type.name = name;
		dest.struct_or_union.fields.emplace_back(std::move(field_type));
		
		check_eof(tokens, pos);
		verify(tokens[pos].type == CPP_OPERATOR && tokens[pos].type == CPP_OPERATOR && tokens[pos].op == CPP_OP_SEMICOLON, "Expected ';'.");
		pos++;
	}
	pos++; // '}'
}

static CppType parse_type_name(const std::vector<CppToken>& tokens, size_t& pos) {
	if(tokens[pos].type == CPP_KEYWORD) {
		s32 sign = 0;
		if(tokens[pos].keyword == CPP_KEYWORD_signed) {
			sign = 1;
			pos++;
			check_eof(tokens, pos, "'signed' keyword");
		} else if(tokens[pos].keyword == CPP_KEYWORD_unsigned) {
			sign = -1;
			pos++;
			check_eof(tokens, pos, "'unsigned' keyword");
		}
		
		CppType field(CPP_BUILT_IN);
		switch(tokens[pos].keyword) {
			case CPP_KEYWORD_char: field.built_in = (sign == -1) ? CppBuiltIn::UCHAR : CppBuiltIn::SCHAR; break;
			case CPP_KEYWORD_char8_t: field.built_in = CppBuiltIn::S8; break;
			case CPP_KEYWORD_char16_t: field.built_in = CppBuiltIn::S16; break;
			case CPP_KEYWORD_char32_t: field.built_in = CppBuiltIn::S32; break;
			case CPP_KEYWORD_short: field.built_in = (sign == -1) ? CppBuiltIn::USHORT : CppBuiltIn::SHORT; break;
			case CPP_KEYWORD_int: field.built_in = (sign == -1) ? CppBuiltIn::UINT : CppBuiltIn::INT; break;
			case CPP_KEYWORD_long: {
				if(pos + 1 < tokens.size() && tokens[pos + 1].type == CPP_KEYWORD && tokens[pos + 1].keyword == CPP_KEYWORD_long) {
					field.built_in = (sign == -1) ? CppBuiltIn::ULONGLONG : CppBuiltIn::LONGLONG;
					pos++;
				} else {
					field.built_in = (sign == -1) ? CppBuiltIn::ULONG : CppBuiltIn::LONG;
				}
				break;
			}
			case CPP_KEYWORD_float: field.built_in = CppBuiltIn::FLOAT; break;
			case CPP_KEYWORD_double: field.built_in = CppBuiltIn::DOUBLE; break;
			default: verify_not_reached("Expected type name.");
		}
		
		pos++;
		
		return field;
	} else if(tokens[pos].type == CPP_IDENTIFIER) {
		verify_not_reached("Not yet implemented.");
	}
	verify_not_reached("Expected type name.");
}

static bool check_eof(const std::vector<CppToken>& tokens, size_t& pos, const char* thing) {
	if(thing) {
		verify(pos < tokens.size(), "Unexpected end of file after %s.", thing);
	} else {
		verify(pos < tokens.size(), "Unexpected end of file.");
	}
	return true;
}

void layout_pvar_type(CppType& type) {
	
}

// **** Evil code beyond this point!!! ****

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
