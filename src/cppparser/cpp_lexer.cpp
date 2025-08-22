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

#include "cpp_lexer.h"

#include <map>

const CppKeywordTableEntry CPP_KEYWORDS[] = {
	#define DEF_CPP_KEYWORD(keyword) {CPP_KEYWORD_##keyword, #keyword},
	#include "cpp_keywords.h"
	#undef DEF_CPP_KEYWORD
};
const s32 CPP_KEYWORD_COUNT = ARRAY_SIZE(CPP_KEYWORDS);

extern const CppOperatorTableEntry CPP_OPERATORS[] = {
	#define DEF_CPP_OPERATOR(string, name) {CPP_OP_##name, string},
	#include "cpp_operators.h"
	#undef DEF_CPP_OPERATOR
};
extern const s32 CPP_OPERATOR_COUNT = ARRAY_SIZE(CPP_OPERATORS);

static void splice_physical_lines(char* ptr, std::map<s32, s32>& line_lookup_dest);

struct CppLexer
{
	const char* begin;
	const char* ptr;
	std::vector<CppToken> tokens;
	std::map<s32, s32> line_lookup;
	
	bool eat_raw_string();
	bool eat_comment();
	bool eat_keyword_or_operator();
	bool eat_literal();
	bool eat_number_literal();
	bool eat_character_literal();
	bool eat_string_literal();
	void eat_literal_char();
	bool eat_boolean_literal();
	bool eat_pointer_literal();
	bool eat_identifier();
	
	s32 get_line();
};

static bool is_literal_char(char c);

std::vector<CppToken> eat_cpp_file(char* input)
{
	CppLexer lexer;
	lexer.begin = input;
	lexer.ptr = input;
	splice_physical_lines(input, lexer.line_lookup); // [lex.phases] 1
	while (*lexer.ptr != '\0') {
		// Skip whitespace.
		if (*lexer.ptr == ' ' || *lexer.ptr == '\n' || *lexer.ptr == '\t') {
			lexer.ptr++;
			continue;
		}
		
		if (*lexer.ptr == '#') {
			// Preprocessor directive.
			lexer.ptr++;
			while (*lexer.ptr == ' ' || *lexer.ptr == '\t') lexer.ptr++; // Skip whitespace.
			const char* str_begin = lexer.ptr;
			while (*lexer.ptr != '\n' && *lexer.ptr != '\0') {
				lexer.ptr++;
			}
			CppToken& token = lexer.tokens.emplace_back();
			token.type = CPP_PREPROCESSOR_DIRECTIVE;
			token.str_begin = str_begin;
			token.str_end = lexer.ptr;
			token.line = lexer.get_line();
			continue;
		}
		
		if (lexer.eat_raw_string()) {
			continue;
		}
		
		// [lex.pptoken] 3.2
		if (lexer.ptr[0] == '<' && lexer.ptr[1] == ':' && lexer.ptr[2] == ':' && (lexer.ptr[3] != ':' && lexer.ptr[3] != '>')) {
			lexer.ptr += 3;
			CppToken& token_1 = lexer.tokens.emplace_back();
			token_1.type = CPP_OPERATOR;
			token_1.op = CPP_OP_LESS_THAN;
			token_1.line = lexer.get_line();
			CppToken& token_2 = lexer.tokens.emplace_back();
			token_2.type = CPP_OPERATOR;
			token_2.op = CPP_OP_SCOPE_SEPARATOR;
			token_2.line = lexer.get_line();
			continue;
		}
		
		if (lexer.eat_comment()) {
			continue;
		}
		
		if (lexer.eat_keyword_or_operator()) {
			continue;
		}
		
		if (lexer.eat_literal()) {
			continue;
		}
		
		if (lexer.eat_identifier()) {
			continue;
		}
		
		std::string str(lexer.ptr);
		verify_not_reached("Unrecognised token: %s", str.substr(0, 32).c_str());
	}
	
	// Fill in prev and next indices for skipping preprocessor directives.
	size_t prev = lexer.tokens.size();
	for (size_t i = 0; i < lexer.tokens.size(); i++) {
		lexer.tokens[i].prev = prev;
		if (lexer.tokens[i].type != CPP_PREPROCESSOR_DIRECTIVE) {
			prev = i;
		}
	}
	size_t next = lexer.tokens.size();
	for (size_t i = lexer.tokens.size(); i > 0; i--) {
		lexer.tokens[i - 1].next = next;
		if (lexer.tokens[i - 1].type != CPP_PREPROCESSOR_DIRECTIVE) {
			next = i - 1;
		}
	}
	
	return lexer.tokens;
}

static void splice_physical_lines(char* string, std::map<s32, s32>& line_lookup_dest)
{
	size_t size = strlen(string);
	size_t out = 0;
	s32 current_line = 1;
	for (size_t in = 0; in < size; in++) {
		if (string[in] == '\\' && string[in + 1] == '\n') {
			in += 2;
			current_line++;
		} else if (string[in] == '\n') {
			current_line++;
			line_lookup_dest[-((s32) out + 1)] = current_line;
		}
		string[out++] = string[in];
	}
	string[out] = '\0';
}

bool CppLexer::eat_raw_string()
{
	if (ptr[0] != 'R' || ptr[1] != '"') {
		return false;
	}
	
	ptr += 2;
	char dchar[17];
	dchar[0] = ')';
	s32 dchar_size;
	// Read opening delimiter.
	for (dchar_size = 0; dchar_size < 16; dchar_size++) {
		if (*ptr == '(' || *ptr == '\0') break;
		dchar[dchar_size + 1] = *(ptr++);
	}
	if (*ptr != '\0') ptr++; // Skip past '('.
	const char* str_begin = ptr;
	// Scan for closing delimiter.
	while (*ptr != '\0') {
		bool matches = true;
		for (s32 i = 0; i < dchar_size + 1; i++) {
			if (ptr[i] != dchar[i]) {
				matches = false;
				break;
			}
		}
		if (matches) {
			break;
		}
		ptr++;
	}
	const char* str_end = ptr;
	ptr += dchar_size + 2;
	
	CppToken& token = tokens.emplace_back();
	token.type = CPP_STRING_LITERAL;
	token.str_begin = str_begin;
	token.str_end = str_end;
	token.line = get_line();
	
	return true;
}

bool CppLexer::eat_comment()
{
	// [lex.comment]
	if (ptr[0] == '/' && ptr[1] == '*') {
		ptr += 2;
		while (*ptr != '\0') {
			if (ptr[0] == '*' && ptr[1] == '/') {
				ptr += 2;
				break;
			}
			ptr++;
		}
		return true;
	} else if (ptr[0] == '/' && ptr[1] == '/') {
		ptr += 2;
		while (*ptr != '\0') {
			if (ptr[0] == '\n') {
				ptr++;
				break;
			}
			ptr++;
		}
		return true;
	}
	return false;
}

bool CppLexer::eat_keyword_or_operator()
{
	// [lex.key]
	for (s32 i = 0; i < CPP_KEYWORD_COUNT; i++) {
		size_t chars = strlen(CPP_KEYWORDS[i].string);
		if (strncmp(ptr, CPP_KEYWORDS[i].string, chars) == 0 && !is_literal_char(ptr[chars])) {
			ptr += strlen(CPP_KEYWORDS[i].string);
			CppToken& token = tokens.emplace_back();
			token.type = CPP_KEYWORD;
			token.keyword = CPP_KEYWORDS[i].keyword;
			token.line = get_line();
			return true;
		}
	}
	
	// [lex.operators]
	for (s32 i = 0; i < CPP_OPERATOR_COUNT; i++) {
		size_t chars = strstr(CPP_OPERATORS[i].string, " ") - CPP_OPERATORS[i].string;
		if (strncmp(ptr, CPP_OPERATORS[i].string, chars) == 0) {
			ptr += chars;
			CppToken& token = tokens.emplace_back();
			token.type = CPP_OPERATOR;
			token.op = CPP_OPERATORS[i].op;
			token.line = get_line();
			return true;
		}
	}
	
	return false;
}

bool CppLexer::eat_literal()
{
	bool hungry = true;
	
	if (hungry && eat_number_literal()) hungry = false;
	if (hungry && eat_character_literal()) hungry = false;
	if (hungry && eat_string_literal()) hungry = false;
	if (hungry && eat_boolean_literal()) hungry = false;
	if (hungry && eat_pointer_literal()) hungry = false;
	
	return !hungry;
}

bool CppLexer::eat_number_literal()
{
	const char* str_begin = ptr;
	
	if (ptr[0] == '0' && (ptr[1] == 'b' || ptr[1] == 'B')) {
		// binary-literal
		ptr += 2;
		while (*ptr == '0'  || *ptr == '1' || *ptr == '\'') ptr++;
		
		CppToken& token = tokens.emplace_back();
		token.type = CPP_INTEGER_LITERAL;
		token.str_begin = str_begin;
		token.str_end = ptr;
		token.i = strtoll(std::string(str_begin + 2, (const char*) ptr).c_str(), nullptr, 2);
		token.line = get_line();
		
		return true;
	} else if (ptr[0] == '0' && (ptr[1] == 'x' || ptr[1] == 'X')) {
		// hexadecimal-literal
		ptr += 2;
		while ((*ptr >= '0' && *ptr <= '9')
			|| (*ptr >= 'a' && *ptr <= 'f')
			|| (*ptr >= 'A' && *ptr <= 'F')
			|| *ptr == '\'') ptr++;
		
		CppToken& token = tokens.emplace_back();
		token.type = CPP_INTEGER_LITERAL;
		token.str_begin = str_begin;
		token.str_end = ptr;
		token.i = strtoll(std::string(str_begin + 2, (const char*) ptr).c_str(), nullptr, 16);
		token.line = get_line();
		
		return true;
	} else if (ptr[0] == '0') {
		// octal-literal
		ptr++;
		while ((*ptr >= '0' && *ptr <= '7') || *ptr == '\'') ptr++;
		
		CppToken& token = tokens.emplace_back();
		token.type = CPP_INTEGER_LITERAL;
		token.str_begin = str_begin;
		token.str_end = ptr;
		token.i = strtoll(std::string(str_begin + 1, (const char*) ptr).c_str(), nullptr, 8);
		token.line = get_line();
		
		return true;
	} else if (ptr[0] >= '0' && ptr[0] <= '9') {
		// decimal-literal / decimal-floating-point-literal
		while ((*ptr >= '0' && *ptr <= '9') || *ptr == '\'') ptr++;
		if (*ptr == '.') {
			ptr++;
			while ((*ptr >= '0' && *ptr <= '9') || *ptr == '\'') ptr++;
			
			CppToken& token = tokens.emplace_back();
			token.type = CPP_FLOATING_POINT_LITERAL;
			token.str_begin = str_begin;
			token.str_end = ptr;
			token.f = strtof(std::string(str_begin, (const char*) ptr).c_str(), nullptr);
			token.line = get_line();
			
			// floating-point-suffix
			if (*ptr == 'f' || *ptr == 'F') ptr++;
			if (*ptr == 'f' || *ptr == 'F') ptr++;
		} else {
			while ((*ptr >= '0' && *ptr <= '9')
				|| *ptr == '\'') ptr++;
			
			CppToken& token = tokens.emplace_back();
			token.type = CPP_INTEGER_LITERAL;
			token.str_begin = str_begin;
			token.str_end = ptr;
			token.i = strtoll(std::string(str_begin, (const char*) ptr).c_str(), nullptr, 10);
			token.line = get_line();
			
			// integer-suffix
			if (*ptr == 'u' || *ptr == 'U' || *ptr == 'l' || *ptr == 'L') ptr++;
			if (*ptr == 'u' || *ptr == 'U' || *ptr == 'l' || *ptr == 'L') ptr++;
		}
		
		return true;
	}
	
	return false;
}

bool CppLexer::eat_character_literal()
{
	const char* str_begin = ptr;
	
	bool hungry = true;
	if (hungry && strncmp(ptr, "u8'", 3) == 0) { ptr += 3; hungry = false; }
	if (hungry && strncmp(ptr, "u'", 2) == 0) { ptr += 2; hungry = false; }
	if (hungry && strncmp(ptr, "U'", 2) == 0) { ptr += 2; hungry = false; }
	if (hungry && strncmp(ptr, "L'", 2) == 0) { ptr += 2; hungry = false; }
	if (hungry && strncmp(ptr, "'", 1) == 0) { ptr += 1; hungry = false; }
	
	if (!hungry) {
		while (*ptr != '\'' && *ptr != '\0') {
			eat_literal_char();
		}
		if (*ptr != '\0') ptr++; // '\''
		
		CppToken& token = tokens.emplace_back();
		token.type = CPP_CHARACTER_LITERAL;
		token.str_begin = str_begin;
		token.str_end = ptr;
		token.line = get_line();
		
		return true;
	}
	
	return false;
}

bool CppLexer::eat_string_literal()
{
	bool hungry = true;
	if (hungry && strncmp(ptr, "u8\"", 3) == 0) { ptr += 3; hungry = false; }
	if (hungry && strncmp(ptr, "u\"", 2) == 0) { ptr += 2; hungry = false; }
	if (hungry && strncmp(ptr, "U\"", 2) == 0) { ptr += 2; hungry = false; }
	if (hungry && strncmp(ptr, "L\"", 2) == 0) { ptr += 2; hungry = false; }
	if (hungry && strncmp(ptr, "\"", 1) == 0) { ptr += 1; hungry = false; }
	
	const char* str_begin = ptr;
	
	if (!hungry) {
		while (*ptr != '\"' && *ptr != '\0') {
			eat_literal_char();
		}
		
		CppToken& token = tokens.emplace_back();
		token.type = CPP_STRING_LITERAL;
		token.str_begin = str_begin;
		token.str_end = ptr;
		token.line = get_line();
		
		if (*ptr != '\0') ptr++;
		
		return true;
	}
	
	return false;
}

void CppLexer::eat_literal_char()
{
	if (*ptr == '\\') {
		ptr++;
		if (*ptr == '\''
			|| *ptr == '"'
			|| *ptr == '?'
			|| *ptr == '\\'
			|| *ptr == 'a'
			|| *ptr == 'b'
			|| *ptr == 'f'
			|| *ptr == 'n'
			|| *ptr == 'r'
			|| *ptr == 't'
			|| *ptr == 'v') {
			// simple-escape-sequence
			ptr++;
		} else if (*ptr == 'x') {
			// hexadecimal-escape-sequence
			ptr++;
			while ((*ptr >= '0' && *ptr <= '9') || (*ptr >= 'A' && *ptr <= 'F') || (*ptr >= 'a' && *ptr <= 'f')) ptr++;
		} else {
			// octal-escape-sequence
			while (*ptr >= '0' && *ptr <= '7') ptr++;
		}
	} else {
		ptr++;
	}
}

bool CppLexer::eat_boolean_literal()
{
	const char* str_begin = ptr;
	
	if (strncmp(ptr, "false", 5) == 0 && !is_literal_char(ptr[5])) {
		ptr += 5;
		
		CppToken& token = tokens.emplace_back();
		token.type = CPP_BOOLEAN_LITERAL;
		token.str_begin = str_begin;
		token.str_end = ptr;
		token.i = false;
		token.line = get_line();
		
		return true;
	}
	if (strncmp(ptr, "true", 4) == 0 && !is_literal_char(ptr[4])) {
		ptr += 4;
		
		CppToken& token = tokens.emplace_back();
		token.type = CPP_BOOLEAN_LITERAL;
		token.str_begin = str_begin;
		token.str_end = ptr;
		token.i = true;
		token.line = get_line();
		
		return true;
	}
	return false;
}

bool CppLexer::eat_pointer_literal()
{
	const char* str_begin = ptr;
	
	if (strncmp(ptr, "nullptr", 7) == 0 && !is_literal_char(ptr[7])) {
		ptr += 7;
		
		CppToken& token = tokens.emplace_back();
		token.type = CPP_POINTER_LITERAL;
		token.str_begin = str_begin;
		token.str_end = ptr;
		token.i = true;
		token.line = get_line();
		
		return true;
	}
	return false;
}

bool CppLexer::eat_identifier()
{
	if ((*ptr >= 'A' && *ptr <= 'Z') || (*ptr >= 'a' && *ptr <= 'z') || *ptr == '_') {
		const char* str_begin = ptr;
		ptr++;
		while (is_literal_char(*ptr)) {
			ptr++;
		}
		CppToken& token = tokens.emplace_back();
		token.type = CPP_IDENTIFIER;
		token.str_begin = str_begin;
		token.str_end = ptr;
		token.line = get_line();
		return true;
	}
	return false;
}

s32 CppLexer::get_line()
{
	auto iter = line_lookup.lower_bound(-((s32) (ptr - begin)));
	if (iter != line_lookup.end()) {
		return iter->second;
	} else {
		return 0;
	}
}

static bool is_literal_char(char c)
{
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
}

const char* cpp_token_type(CppTokenType type)
{
	switch (type) {
		case CPP_IDENTIFIER: return "identifier";
		case CPP_KEYWORD: return "keyword";
		case CPP_BOOLEAN_LITERAL: return "boolean literal";
		case CPP_CHARACTER_LITERAL: return "character literal";
		case CPP_FLOATING_POINT_LITERAL: return "floating point literal";
		case CPP_INTEGER_LITERAL: return "integer literal";
		case CPP_POINTER_LITERAL: return "pointer literal";
		case CPP_STRING_LITERAL: return "string literal";
		case CPP_OPERATOR: return "operator";
		case CPP_PREPROCESSOR_DIRECTIVE: return "preprocessor directive";
	}
	return "(invalid token)";
}
