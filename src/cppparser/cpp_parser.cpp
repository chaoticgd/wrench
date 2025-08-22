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

struct CppParserState
{
	const std::vector<CppToken>& tokens;
	size_t pos = 0;
	
	const CppToken& cur() const
	{
		verify(pos < tokens.size(), "Unexpected end of file.");
		return tokens[pos];
	}
	
	void advance()
	{
		verify(pos < tokens.size(), "Unexpected end of file.");
		pos = tokens[pos].next;
	}
	
	const CppToken& peek() const
	{
		verify(pos < tokens.size() && tokens[pos].next < tokens.size(), "Unexpected end of file.");
		return tokens[tokens[pos].next];
	}
	
	bool eof() const
	{
		return pos < tokens.size();
	}
};

static void parse_enum(CppType& dest, CppParserState& parser);
static void parse_struct_or_union(CppType& dest, CppParserState& parser);
static CppType parse_field(CppParserState& parser);
static CppType parse_type_name(CppParserState& parser);
static std::vector<CppPreprocessorDirective> parse_preprocessor_directives(CppParserState& parser, size_t token);

bool parse_cpp_types(std::map<std::string, CppType>& types, const std::vector<CppToken>& tokens)
{
	CppParserState parser{tokens};
	bool enabled = false;
	bool ever_enabled_for_this_file = false;
	
	while (parser.pos < tokens.size()) {
		if (tokens[parser.pos].type == CPP_PREPROCESSOR_DIRECTIVE) {
			std::string_view str(tokens[parser.pos].str_begin, tokens[parser.pos].str_end);
			if (str.starts_with("pragma wrench parser on")) {
				enabled = true;
				ever_enabled_for_this_file = true;
			} else if (str.starts_with("pragma wrench parser off")) {
				enabled = false;
			}
		}
		
		if (!enabled || tokens[parser.pos].type != CPP_KEYWORD) {
			parser.pos++;
			continue;
		}
		
		if (tokens[parser.pos].keyword == CPP_KEYWORD_struct || tokens[parser.pos].keyword == CPP_KEYWORD_union) {
			size_t second_pos = tokens[parser.pos].next;
			
			// Parse alignof expression.
			size_t next_pos = tokens.size();
			s32 alignment = -1;
			if (second_pos < tokens.size() && tokens[second_pos].type == CPP_KEYWORD && tokens[second_pos].keyword == CPP_KEYWORD_alignas) {
				size_t third_pos = tokens[second_pos].next;
				if (third_pos < tokens.size() && tokens[third_pos].type == CPP_OPERATOR && tokens[third_pos].op == CPP_OP_OPENING_ROUND) {
					size_t fourth_pos = tokens[third_pos].next;
					if (fourth_pos < tokens.size() && tokens[fourth_pos].type == CPP_INTEGER_LITERAL) {
						size_t fifth_pos = tokens[fourth_pos].next;
						if (fifth_pos < tokens.size() && tokens[fifth_pos].type == CPP_OPERATOR && tokens[fifth_pos].op == CPP_OP_CLOSING_ROUND) {
							alignment = (s32) tokens[fourth_pos].i;
							next_pos = tokens[fifth_pos].next;
						}
					}
				}
			} else {
				next_pos = second_pos;
			}
			
			// Parse struct body.
			if (next_pos < tokens.size() && tokens[next_pos].type == CPP_IDENTIFIER) {
				size_t third_pos = tokens[next_pos].next;
				if (third_pos < tokens.size() && tokens[third_pos].type == CPP_OPERATOR && tokens[third_pos].op == CPP_OP_OPENING_CURLY) {
					std::string_view name(tokens[next_pos].str_begin, tokens[next_pos].str_end);
					CppType type(CPP_STRUCT_OR_UNION);
					type.struct_or_union.is_union = tokens[parser.pos].keyword == CPP_KEYWORD_union;
					type.name = name;
					parser.advance(); // struct or union
					if (alignment > -1) {
						parser.advance(); // alignof
						parser.advance(); // (
						parser.advance(); // integer literal
						parser.advance(); // )
					}
					parser.advance(); // identifier
					parser.advance(); // {
					parse_struct_or_union(type, parser);
					type.alignment = alignment;
					types.emplace(name, std::move(type));
					continue;
				}
			}
		}
		
		if (tokens[parser.pos].keyword == CPP_KEYWORD_enum) {
			size_t second_pos = tokens[parser.pos].next;
			if (second_pos < tokens.size() && tokens[second_pos].type == CPP_IDENTIFIER) {
				size_t third_pos = tokens[second_pos].next;
				if (third_pos < tokens.size() && tokens[third_pos].type == CPP_OPERATOR && tokens[third_pos].op == CPP_OP_OPENING_CURLY) {
					std::string_view name(tokens[second_pos].str_begin, tokens[second_pos].str_end);
					CppType type(CPP_ENUM);
					type.name = name;
					parser.advance();
					parser.advance();
					parser.advance();
					parse_enum(type, parser);
					types.emplace(name, std::move(type));
					continue;
				}
			}
		}
		
		if (tokens[parser.pos].keyword == CPP_KEYWORD_typedef) {
			std::vector<CppPreprocessorDirective> directives = parse_preprocessor_directives(parser, parser.pos);
			parser.advance();
			CppType type = parse_field(parser);
			type.preprocessor_directives = std::move(directives);
			types.emplace(type.name, std::move(type));
		}
		
		parser.pos++;
	}
	
	return ever_enabled_for_this_file;
}

static void parse_enum(CppType& dest, CppParserState& parser)
{
	while (true) {
		const CppToken& first = parser.cur();
		if (first.type == CPP_OPERATOR && first.op == CPP_OP_CLOSING_CURLY) {
			parser.advance();
			break;
		}
		
		verify(first.type == CPP_IDENTIFIER, "Expected identifier on line %d, got %s.", first.line, cpp_token_type(first.type));
		parser.advance();
		
		const CppToken& second = parser.cur();
		verify(second.type == CPP_OPERATOR && second.op == CPP_OP_EQUALS, "Expected '=' on line %d, got %s.", second.line, cpp_token_type(second.type));
		parser.advance();
		
		const CppToken& third = parser.cur();
		verify(third.type == CPP_INTEGER_LITERAL, "Expected integer literal on line %d in enum, got %s.", third.line, cpp_token_type(third.type));
		parser.advance();
		
		dest.enumeration.constants.emplace_back(third.i, std::string(first.str_begin, first.str_end));
		
		const CppToken& comma = parser.cur();
		if (comma.type == CPP_OPERATOR && comma.op == CPP_OP_COMMA) {
			parser.advance();
		}
	}
}

static void parse_struct_or_union(CppType& dest, CppParserState& parser)
{
	while (true) {
		const CppToken& terminator = parser.cur();
		if (terminator.type == CPP_OPERATOR && terminator.op == CPP_OP_CLOSING_CURLY) {
			parser.advance();
			break;
		}
		
		CppType field_type = parse_field(parser);
		dest.struct_or_union.fields.emplace_back(std::move(field_type));
		
		const CppToken& semicolon = parser.cur();
		verify(semicolon.type == CPP_OPERATOR && semicolon.type == CPP_OPERATOR && semicolon.op == CPP_OP_SEMICOLON,
			"Expected ';' on line %d, got %s.", semicolon.line, cpp_token_type(semicolon.type));
		parser.advance();
	}
	parser.advance();
}

static CppType parse_field(CppParserState& parser)
{
	std::vector<CppPreprocessorDirective> directives = parse_preprocessor_directives(parser, parser.pos);
	CppType field_type = parse_type_name(parser);
	
	// Parse pointers.
	while (true) {
		const CppToken& token = parser.cur();
		if (token.type != CPP_OPERATOR || (token.op != CPP_OP_STAR && token.op != CPP_OP_AMPERSAND)) {
			break;
		}
		
		CppType type(CPP_POINTER_OR_REFERENCE);
		type.pointer_or_reference.is_reference = token.op == CPP_OP_AMPERSAND;
		type.pointer_or_reference.value_type = std::make_unique<CppType>(std::move(field_type));
		field_type = std::move(type);
		
		parser.advance();
	}
	
	const CppToken& name_token = parser.cur();
	std::string_view name(name_token.str_begin, name_token.str_end);
	parser.advance();
	
	// Parse bitfields.
	const CppToken& bitfield_operator = parser.cur();
	if (bitfield_operator.type == CPP_OPERATOR && bitfield_operator.op == CPP_OP_COLON) {
		verify(field_type.descriptor == CPP_BUILT_IN,
			"A bitfield storage unit can only be a built-in type (line %d).\n", bitfield_operator.line);
		parser.advance();
		
		const CppToken& bitfield_literal = parser.cur();
		verify(bitfield_literal.type == CPP_INTEGER_LITERAL,
			"Expected integer literal on line %d, got %s.",
			bitfield_literal.line, cpp_token_type(bitfield_literal.type));
		parser.advance();
		
		CppType bitfield_type(CPP_BITFIELD);
		bitfield_type.name = name;
		bitfield_type.preprocessor_directives = std::move(directives);
		bitfield_type.bitfield.bit_size = bitfield_literal.i;
		bitfield_type.bitfield.storage_unit_type = std::make_unique<CppType>(std::move(field_type));
		return bitfield_type;
	}
	
	// Parse array subscripts.
	std::vector<s32> array_indices;
	while (true) {
		const CppToken& opening_bracket_token = parser.cur();
		if (opening_bracket_token.type != CPP_OPERATOR || opening_bracket_token.op != CPP_OP_OPENING_SQUARE) {
			break;
		}
		parser.advance();
		
		const CppToken& literal = parser.cur();
		verify(literal.type == CPP_INTEGER_LITERAL, "Expected integer literal on line %d, got %s.", literal.line, cpp_token_type(literal.type));
		array_indices.emplace_back(literal.i);
		parser.advance();
		
		const CppToken closing_bracket_token = parser.cur();
		verify(closing_bracket_token.type == CPP_OPERATOR && closing_bracket_token.op == CPP_OP_CLOSING_SQUARE,
			"Expected ']' on line %d, got %s.", closing_bracket_token.line, cpp_token_type(closing_bracket_token.type));
		parser.advance();
	}
	for (size_t i = array_indices.size(); i > 0; i--) {
		CppType array_type(CPP_ARRAY);
		array_type.array.element_count = array_indices[i - 1];
		array_type.array.element_type = std::make_unique<CppType>(std::move(field_type));
		field_type = std::move(array_type);
	}
	
	field_type.name = name;
	field_type.preprocessor_directives = std::move(directives);
	return field_type;
}

static CppType parse_type_name(CppParserState& parser)
{
	const CppToken& first = parser.cur();
	
	if (first.type == CPP_KEYWORD) {
		u8 has_keyword[128];
		bool has_double_long = false;
		memset(has_keyword, 0, CPP_KEYWORD_COUNT);
		
		// Parse tokens until a token of a type that can't be part of a built-in
		// type name is encountered.
		while (true) {
			const CppToken& token = parser.cur();
			if (token.type != CPP_KEYWORD) break;
			
			bool good = false;
			if (token.keyword == CPP_KEYWORD_bool) good = true;
			if (token.keyword == CPP_KEYWORD_char) good = true;
			if (token.keyword == CPP_KEYWORD_short) good = true;
			if (token.keyword == CPP_KEYWORD_int) good = true;
			if (token.keyword == CPP_KEYWORD_long) good = true;
			if (token.keyword == CPP_KEYWORD_float) good = true;
			if (token.keyword == CPP_KEYWORD_double) good = true;
			if (token.keyword == CPP_KEYWORD_void) good = true;
			if (token.keyword == CPP_KEYWORD_signed) good = true;
			if (token.keyword == CPP_KEYWORD_unsigned) good = true;
			if (token.keyword == CPP_KEYWORD_const) good = true;
			if (token.keyword == CPP_KEYWORD_mutable) good = true;
			
			if (good) {
				if (token.keyword == CPP_KEYWORD_long && has_keyword[CPP_KEYWORD_long]) {
					has_double_long = true;
				}
				has_keyword[token.keyword] = 1;
				parser.advance();
			} else {
				break;
			}
		}
		
		CppType type(CPP_BUILT_IN);
		if (has_keyword[CPP_KEYWORD_float]) {
			verify(!has_keyword[CPP_KEYWORD_short], "'short' specified with 'float' on line %d.", first.line);
			verify(!has_keyword[CPP_KEYWORD_long], "'long' specified with 'float' on line %d.", first.line);
			verify(!has_keyword[CPP_KEYWORD_signed], "'signed' specified with 'float' on line %d.", first.line);
			verify(!has_keyword[CPP_KEYWORD_unsigned], "'unsigned' specified with 'float' on line %d.", first.line);
			type.built_in = CPP_FLOAT;
		} else if (has_keyword[CPP_KEYWORD_double]) {
			verify(!has_keyword[CPP_KEYWORD_short], "'short' specified with 'double' on line %d.", first.line);
			verify(!has_keyword[CPP_KEYWORD_long], "'long' specified with 'double' on line %d.", first.line);
			verify(!has_keyword[CPP_KEYWORD_signed], "'signed' specified with 'double' on line %d.", first.line);
			verify(!has_keyword[CPP_KEYWORD_unsigned], "'unsigned' specified with 'double'. on line %d", first.line);
			type.built_in = CPP_DOUBLE;
		} else if (has_keyword[CPP_KEYWORD_bool]) {
			verify(!has_keyword[CPP_KEYWORD_short], "'short' specified with 'bool' on line %d.", first.line);
			verify(!has_keyword[CPP_KEYWORD_long], "'long' specified with 'bool' on line %d.", first.line);
			verify(!has_keyword[CPP_KEYWORD_signed], "'signed' specified with 'bool' on line %d.", first.line);
			verify(!has_keyword[CPP_KEYWORD_unsigned], "'unsigned' specified with 'bool' on line %d.", first.line);
			type.built_in = CPP_BOOL;
		} else if (has_keyword[CPP_KEYWORD_char]) {
			verify(!has_keyword[CPP_KEYWORD_short], "'short' specified with 'char' on line %d.", first.line);
			verify(!has_keyword[CPP_KEYWORD_long], "'long' specified with 'char' on line %d.", first.line);
			type.built_in = has_keyword[CPP_KEYWORD_unsigned] ? CPP_UCHAR : CPP_CHAR;
		} else if (has_keyword[CPP_KEYWORD_short]) {
			verify(!has_keyword[CPP_KEYWORD_long], "'long' specified with 'short' on line %d.", first.line);
			type.built_in = has_keyword[CPP_KEYWORD_unsigned] ? CPP_USHORT : CPP_SHORT;
		} else if (has_keyword[CPP_KEYWORD_long]) {
			if (has_double_long) {
				type.built_in = has_keyword[CPP_KEYWORD_unsigned] ? CPP_ULONGLONG : CPP_LONGLONG;
			} else {
				type.built_in = has_keyword[CPP_KEYWORD_unsigned] ? CPP_ULONG : CPP_LONG;
			}
		} else if (has_keyword[CPP_KEYWORD_int]) {
			type.built_in = has_keyword[CPP_KEYWORD_unsigned] ? CPP_UINT : CPP_INT;
		} else if (has_keyword[CPP_KEYWORD_void]) {
			type.built_in = CPP_VOID;
		}
		return type;
	} else if (first.type == CPP_IDENTIFIER) {
		std::string str(first.str_begin, first.str_end);
		parser.advance();
		
		static const struct { CppBuiltIn built_in; const char* name; } fixed_width_integers[] = {
			{CPP_S8, "s8"},
			{CPP_U8, "u8"},
			{CPP_S16, "s16"},
			{CPP_U16, "u16"},
			{CPP_S32, "s32"},
			{CPP_U32, "u32"},
			{CPP_S64, "s64"},
			{CPP_U64, "u64"},
			{CPP_S128, "s128"},
			{CPP_U128, "u128"},
		};
		
		for (const auto& entry : fixed_width_integers) {
			if (str == entry.name) {
				CppType type(CPP_BUILT_IN);
				type.built_in = entry.built_in;
				return type;
			}
		}
		
		CppType type(CPP_TYPE_NAME);
		type.type_name.string = std::move(str);
		return type;
	}
	verify_not_reached("Expected type name on line %d, got %s.", first.line, cpp_token_type(first.type));
}

static const CppPreprocessorDirective CPP_DIRECTIVES[] = {
	{CPP_DIRECTIVE_BCD, "bcd"},
	{CPP_DIRECTIVE_BITFLAGS, "bitflags"},
	{CPP_DIRECTIVE_ELEMENTNAMES, "elementnames"},
	{CPP_DIRECTIVE_ENUM, "enum"}
};

static std::vector<CppPreprocessorDirective> parse_preprocessor_directives(
	CppParserState& parser, size_t token)
{
	std::vector<CppPreprocessorDirective> directives;
	
	while (token > 0 && parser.tokens[token - 1].type == CPP_PREPROCESSOR_DIRECTIVE) {
		std::string line(parser.tokens[token - 1].str_begin, parser.tokens[token - 1].str_end);
		if (line.starts_with("pragma wrench ")) {
			line = line.substr(14);
			
			bool found = false;
			for (const CppPreprocessorDirective& info : CPP_DIRECTIVES) {
				if (line.starts_with(info.string)) {
					CppPreprocessorDirective& directive = directives.emplace_back();
					directive.type = info.type;
					if (line.size() >= info.string.size() + 1 && line[info.string.size()] == ' ') {
						directive.string = line.substr(info.string.size() + 1);
					}
					
					found = true;
					break;
				}
			}
			
			verify(found || line.starts_with("parser"), "Unkown wrench pragma directive '%s'.", line.c_str());
		}
		token--;
	}
	
	return directives;
}
