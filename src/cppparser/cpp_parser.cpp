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

#include "cpp_parser.h"

struct CppParserState {
	const std::vector<CppToken>& tokens;
	size_t pos = 0;
	
	const CppToken& cur() const {
		verify(pos < tokens.size(), "Unexpected end of file.");
		return tokens[pos];
	}
	
	void advance() {
		verify(pos < tokens.size(), "Unexpected end of file.");
		pos = tokens[pos].next;
	}
	
	const CppToken& peek() const {
		verify(pos < tokens.size() && tokens[pos].next < tokens.size(), "Unexpected end of file.");
		return tokens[tokens[pos].next];
	}
	
	bool eof() const {
		return pos < tokens.size();
	}
};

static void parse_struct_or_union(CppType& dest, CppParserState& parser);
static CppType parse_type_name(CppParserState& parser);

std::vector<CppType> parse_cpp_types(const std::vector<CppToken>& tokens) {
	CppParserState parser{tokens};
	bool enabled = false;
	
	std::vector<CppType> types;
	while(parser.pos < tokens.size()) {
		if(tokens[parser.pos].type == CPP_COMMENT) {
			std::string str(tokens[parser.pos].str_begin, tokens[parser.pos].str_end);
			if(str.find("wrench parser on") != std::string::npos) {
				enabled = true;
			} else if(str.find("wrench parser off") != std::string::npos) {
				enabled = false;
			}
		}
		
		if(enabled && tokens[parser.pos].type == CPP_KEYWORD && (tokens[parser.pos].keyword == CPP_KEYWORD_struct || tokens[parser.pos].keyword == CPP_KEYWORD_union)) {
			if(parser.pos + 1 < tokens.size() && tokens[parser.pos + 1].type == CPP_IDENTIFIER) {
				if(parser.pos + 2 < tokens.size() && tokens[parser.pos + 2].type == CPP_OPERATOR && tokens[parser.pos + 2].op == CPP_OP_OPENING_CURLY) {
					CppType& type = types.emplace_back(CPP_STRUCT_OR_UNION);
					type.struct_or_union.is_union = tokens[parser.pos].keyword == CPP_KEYWORD_union;
					type.name = std::string(tokens[parser.pos + 1].str_begin, tokens[parser.pos + 1].str_end);
					parser.pos += 3;
					parse_struct_or_union(type, parser);
					continue;
				}
			}
		}
		
		parser.pos++;
	}
	return types;
}

static void parse_struct_or_union(CppType& dest, CppParserState& parser) {
	while(true) {
		const CppToken& terminator = parser.cur();
		if(terminator.type == CPP_OPERATOR && terminator.op == CPP_OP_CLOSING_CURLY) {
			parser.advance();
			break;
		}
		
		CppType field_type = parse_type_name(parser);
		
		// Parse pointers.
		while(true) {
			const CppToken& token = parser.cur();
			if(token.type != CPP_OPERATOR || (token.op != CPP_OP_STAR && token.op != CPP_OP_AMPERSAND)) {
				break;
			}
			
			CppType type(CPP_POINTER_OR_REFERENCE);
			type.pointer_or_reference.is_reference = token.op == CPP_OP_AMPERSAND;
			type.pointer_or_reference.value_type = std::make_unique<CppType>(std::move(field_type));
			field_type = std::move(type);
			
			parser.advance();
		}
		
		const CppToken& name_token = parser.cur();
		std::string name = std::string(name_token.str_begin, name_token.str_end);
		parser.advance();
		
		// Parse array subscripts.
		std::vector<s32> array_indices;
		while(true) {
			const CppToken& opening_bracket_token = parser.cur();
			if(opening_bracket_token.type != CPP_OPERATOR || opening_bracket_token.op != CPP_OP_OPENING_SQUARE) {
				break;
			}
			parser.advance();
			
			const CppToken& literal = parser.cur();
			verify(literal.type == CPP_INTEGER_LITERAL, "Expected integer literal.");
			array_indices.emplace_back(literal.i);
			parser.advance();
			
			const CppToken closing_bracket_token = parser.cur();
			verify(closing_bracket_token.type == CPP_OPERATOR && closing_bracket_token.op == CPP_OP_CLOSING_SQUARE, "Expected ']'.");
			parser.advance();
		}
		for(size_t i = array_indices.size(); i > 0; i--) {
			CppType array_type(CPP_ARRAY);
			array_type.array.element_count = array_indices[i - 1];
			array_type.array.element_type = std::make_unique<CppType>(std::move(field_type));
			field_type = std::move(array_type);
		}
		
		field_type.name = name;
		dest.struct_or_union.fields.emplace_back(std::move(field_type));
		
		const CppToken& semicolon = parser.cur();
		verify(semicolon.type == CPP_OPERATOR && semicolon.type == CPP_OPERATOR && semicolon.op == CPP_OP_SEMICOLON, "Expected ';'.");
		parser.advance();
	}
	parser.advance();
}

static CppType parse_type_name(CppParserState& parser) {
	const CppToken& first = parser.cur();
	
	if(first.type == CPP_KEYWORD) {
		s32 sign = 0;
		if(first.keyword == CPP_KEYWORD_signed) {
			sign = 1;
			parser.advance();
		} else if(first.keyword == CPP_KEYWORD_unsigned) {
			sign = -1;
			parser.advance();
		}
		
		const CppToken& type = parser.cur();
		
		CppType field(CPP_BUILT_IN);
		switch(type.keyword) {
			case CPP_KEYWORD_char: field.built_in = (sign == -1) ? CPP_UCHAR : CPP_SCHAR; break;
			case CPP_KEYWORD_char8_t: field.built_in = CPP_S8; break;
			case CPP_KEYWORD_char16_t: field.built_in = CPP_S16; break;
			case CPP_KEYWORD_char32_t: field.built_in = CPP_S32; break;
			case CPP_KEYWORD_short: field.built_in = (sign == -1) ? CPP_USHORT : CPP_SHORT; break;
			case CPP_KEYWORD_int: field.built_in = (sign == -1) ? CPP_UINT : CPP_INT; break;
			case CPP_KEYWORD_long: {
				const CppToken& after_long = parser.peek();
				if(after_long.type == CPP_KEYWORD && after_long.keyword == CPP_KEYWORD_long) {
					field.built_in = (sign == -1) ? CPP_ULONGLONG : CPP_LONGLONG;
					parser.advance();
				} else {
					field.built_in = (sign == -1) ? CPP_ULONG : CPP_LONG;
				}
				break;
			}
			case CPP_KEYWORD_float: field.built_in = CPP_FLOAT; break;
			case CPP_KEYWORD_double: field.built_in = CPP_DOUBLE; break;
			default: verify_not_reached("Expected type name.");
		}
		
		parser.advance();
		
		return field;
	} else if(first.type == CPP_IDENTIFIER) {
		std::string str(first.str_begin, first.str_end);
		verify_not_reached("Parsing identifiers in type names no yet implemented: '%s'.", str.c_str());
	}
	verify_not_reached("Expected type name, got token of type %s.", cpp_token_type(first.type));
}
