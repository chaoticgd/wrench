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

static void splice_physical_lines(char* ptr);
static bool eat_raw_string(std::vector<CppToken>& tokens, char*& ptr);
static bool eat_comment(std::vector<CppToken>& tokens, char*& ptr);
static bool eat_keyword_or_operator(std::vector<CppToken>& tokens, char*& ptr);
static bool eat_literal(std::vector<CppToken>& tokens, char*& ptr);
static bool eat_number_literal(std::vector<CppToken>& tokens, char*& ptr);
static bool eat_character_literal(std::vector<CppToken>& tokens, char*& ptr);
static bool eat_string_literal(std::vector<CppToken>& tokens, char*& ptr);
static void eat_literal_char(char*& ptr);
static bool eat_boolean_literal(std::vector<CppToken>& tokens, char*& ptr);
static bool eat_pointer_literal(std::vector<CppToken>& tokens, char*& ptr);
static bool eat_identifier(std::vector<CppToken>& tokens, char*& ptr);
static bool is_literal_char(char c);

std::vector<CppToken> eat_cpp_file(char* ptr) {
	std::vector<CppToken> tokens;
	splice_physical_lines(ptr); // [lex.phases] 1
	while(*ptr != '\0') {
		
		if(*ptr == ' ' || *ptr == '\n' || *ptr == '\t') {
			ptr++;
			continue;
		}
		
		if(*ptr == '#') {
			// Skip preprocessor stuff.
			while(*ptr != '\n' && *ptr != '\0') {
				ptr++;
			}
			continue;
		}
		
		if(eat_raw_string(tokens, ptr)) {
			continue;
		}
		
		// [lex.pptoken] 3.2
		if(ptr[0] == '<' && ptr[1] == ':' && ptr[2] == ':' && (ptr[3] != ':' && ptr[3] != '>')) {
			ptr += 3;
			CppToken& token_1 = tokens.emplace_back();
			token_1.type = CPP_OPERATOR;
			token_1.op = CPP_OP_LESS_THAN;
			CppToken& token_2 = tokens.emplace_back();
			token_2.type = CPP_OPERATOR;
			token_2.op = CPP_OP_SCOPE_SEPARATOR;
			continue;
		}
		
		if(eat_comment(tokens, ptr)) {
			continue;
		}
		
		if(eat_keyword_or_operator(tokens, ptr)) {
			continue;
		}
		
		if(eat_literal(tokens, ptr)) {
			continue;
		}
		
		if(eat_identifier(tokens, ptr)) {
			continue;
		}
		
		std::string str(ptr);
		verify_not_reached("Unrecognised token: %s", str.substr(0, 32).c_str());
	}
	
	return tokens;
}

static void splice_physical_lines(char* string) {
	size_t size = strlen(string);
	size_t out = 0;
	for(size_t in = 0; in < size; in++) {
		if(string[in] == '\\' && string[in + 1] == '\n') {
			in += 2;
		}
		string[out++] = string[in];
	}
	string[out] = '\0';
}

static bool eat_raw_string(std::vector<CppToken>& tokens, char*& ptr) {
	if(ptr[0] != 'R' || ptr[1] != '"') {
		return false;
	}
	
	ptr += 2;
	char dchar[17];
	dchar[0] = ')';
	s32 dchar_size;
	// Read opening delimiter.
	for(dchar_size = 0; dchar_size < 16; dchar_size++) {
		if(*ptr == '(' || *ptr == '\0') break;
		dchar[dchar_size + 1] = *(ptr++);
	}
	if(*ptr != '\0') ptr++; // Skip past '('.
	const char* str_begin = ptr;
	// Scan for closing delimiter.
	while(*ptr != '\0') {
		bool matches = true;
		for(s32 i = 0; i < dchar_size + 1; i++) {
			if(ptr[i] != dchar[i]) {
				matches = false;
				break;
			}
		}
		if(matches) {
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
	
	return true;
}

static bool eat_comment(std::vector<CppToken>& tokens, char*& ptr) {
	// [lex.comment]
	if(ptr[0] == '/' && ptr[1] == '*') {
		ptr += 2;
		const char* str_begin = ptr;
		while(*ptr != '\0') {
			if(ptr[0] == '*' && ptr[1] == '/') {
				ptr += 2;
				CppToken& token = tokens.emplace_back();
				token.type = CPP_COMMENT;
				token.str_begin = str_begin;
				token.str_end = ptr;
				break;
			}
			ptr++;
		}
		return true;
	} else if(ptr[0] == '/' && ptr[1] == '/') {
		ptr += 2;
		const char* str_begin = ptr;
		while(*ptr != '\0') {
			if(ptr[0] == '\n') {
				ptr++;
				CppToken& token = tokens.emplace_back();
				token.type = CPP_COMMENT;
				token.str_begin = str_begin;
				token.str_end = ptr;
				break;
			}
			ptr++;
		}
		return true;
	}
	return false;
}

static bool eat_keyword_or_operator(std::vector<CppToken>& tokens, char*& ptr) {
	// [lex.key]
	for(s32 i = 0; i < CPP_KEYWORD_COUNT; i++) {
		size_t chars = strlen(CPP_KEYWORDS[i].string);
		if(strncmp(ptr, CPP_KEYWORDS[i].string, chars) == 0 && !is_literal_char(ptr[chars])) {
			ptr += strlen(CPP_KEYWORDS[i].string);
			CppToken& token = tokens.emplace_back();
			token.type = CPP_KEYWORD;
			token.keyword = CPP_KEYWORDS[i].keyword;
			return true;
		}
	}
	
	// [lex.operators]
	for(s32 i = 0; i < CPP_OPERATOR_COUNT; i++) {
		size_t chars = strstr(CPP_OPERATORS[i].string, " ") - CPP_OPERATORS[i].string;
		if(strncmp(ptr, CPP_OPERATORS[i].string, chars) == 0) {
			ptr += chars;
			CppToken& token = tokens.emplace_back();
			token.type = CPP_OPERATOR;
			token.op = CPP_OPERATORS[i].op;
			return true;
		}
	}
	
	return false;
}

static bool eat_literal(std::vector<CppToken>& tokens, char*& ptr) {
	bool hungry = true;
	
	if(hungry && eat_number_literal(tokens, ptr)) hungry = false;
	if(hungry && eat_character_literal(tokens, ptr)) hungry = false;
	if(hungry && eat_string_literal(tokens, ptr)) hungry = false;
	if(hungry && eat_boolean_literal(tokens, ptr)) hungry = false;
	if(hungry && eat_pointer_literal(tokens, ptr)) hungry = false;
	
	return !hungry;
}

static bool eat_number_literal(std::vector<CppToken>& tokens, char*& ptr) {
	const char* str_begin = ptr;
	
	if(ptr[0] == '0' && (ptr[1] == 'b' || ptr[1] == 'B')) {
		// binary-literal
		ptr += 2;
		while(*ptr == '0'  || *ptr == '1' || *ptr == '\'') ptr++;
		
		CppToken& token = tokens.emplace_back();
		token.type = CPP_INTEGER_LITERAL;
		token.str_begin = str_begin;
		token.str_end = ptr;
		token.i = strtoll(std::string(str_begin + 2, (const char*) ptr).c_str(), nullptr, 2);
		
		return true;
	} else if(ptr[0] == '0' && (ptr[1] == 'x' || ptr[1] == 'X')) {
		// hexadecimal-literal
		ptr += 2;
		while((*ptr >= '0' && *ptr <= '9')
			|| (*ptr >= 'a' && *ptr <= 'f')
			|| (*ptr >= 'A' && *ptr <= 'F')
			|| *ptr == '\'') ptr++;
		
		CppToken& token = tokens.emplace_back();
		token.type = CPP_INTEGER_LITERAL;
		token.str_begin = str_begin;
		token.str_end = ptr;
		token.i = strtoll(std::string(str_begin + 2, (const char*) ptr).c_str(), nullptr, 16);
		
		return true;
	} else if(ptr[0] == '0') {
		// octal-literal
		ptr++;
		while((*ptr >= '0' && *ptr <= '7') || *ptr == '\'') ptr++;
		
		CppToken& token = tokens.emplace_back();
		token.type = CPP_INTEGER_LITERAL;
		token.str_begin = str_begin;
		token.str_end = ptr;
		token.i = strtoll(std::string(str_begin + 1, (const char*) ptr).c_str(), nullptr, 8);
		
		return true;
	} else if(ptr[0] >= '0' && ptr[0] <= '9') {
		// decimal-literal / decimal-floating-point-literal
		while((*ptr >= '0' && *ptr <= '9') || *ptr == '\'') ptr++;
		if(*ptr == '.') {
			ptr++;
			while((*ptr >= '0' && *ptr <= '9') || *ptr == '\'') ptr++;
			
			CppToken& token = tokens.emplace_back();
			token.type = CPP_FLOATING_POINT_LITERAL;
			token.str_begin = str_begin;
			token.str_end = ptr;
			token.f = strtof(std::string(str_begin, (const char*) ptr).c_str(), nullptr);
			
			// floating-point-suffix
			if(*ptr == 'f' || *ptr == 'F') ptr++;
			if(*ptr == 'f' || *ptr == 'F') ptr++;
		} else {
			while((*ptr >= '0' && *ptr <= '9')
				|| *ptr == '\'') ptr++;
			
			CppToken& token = tokens.emplace_back();
			token.type = CPP_INTEGER_LITERAL;
			token.str_begin = str_begin;
			token.str_end = ptr;
			token.i = strtoll(std::string(str_begin, (const char*) ptr).c_str(), nullptr, 10);
			
			// integer-suffix
			if(*ptr == 'u' || *ptr == 'U' || *ptr == 'l' || *ptr == 'L') ptr++;
			if(*ptr == 'u' || *ptr == 'U' || *ptr == 'l' || *ptr == 'L') ptr++;
		}
		
		return true;
	}
	
	return false;
}

static bool eat_character_literal(std::vector<CppToken>& tokens, char*& ptr) {
	const char* str_begin = ptr;
	
	bool hungry = true;
	if(hungry && strncmp(ptr, "u8'", 3) == 0) { ptr += 3; hungry = false; }
	if(hungry && strncmp(ptr, "u'", 2) == 0) { ptr += 2; hungry = false; }
	if(hungry && strncmp(ptr, "U'", 2) == 0) { ptr += 2; hungry = false; }
	if(hungry && strncmp(ptr, "L'", 2) == 0) { ptr += 2; hungry = false; }
	if(hungry && strncmp(ptr, "'", 1) == 0) { ptr += 1; hungry = false; }
	
	if(!hungry) {
		while(*ptr != '\'' && *ptr != '\0') {
			eat_literal_char(ptr);
		}
		if(*ptr != '\0') ptr++; // '\''
		
		CppToken& token = tokens.emplace_back();
		token.type = CPP_CHARACTER_LITERAL;
		token.str_begin = str_begin;
		token.str_end = ptr;
		
		return true;
	}
	
	return false;
}

static bool eat_string_literal(std::vector<CppToken>& tokens, char*& ptr) {
	bool hungry = true;
	if(hungry && strncmp(ptr, "u8\"", 3) == 0) { ptr += 3; hungry = false; }
	if(hungry && strncmp(ptr, "u\"", 2) == 0) { ptr += 2; hungry = false; }
	if(hungry && strncmp(ptr, "U\"", 2) == 0) { ptr += 2; hungry = false; }
	if(hungry && strncmp(ptr, "L\"", 2) == 0) { ptr += 2; hungry = false; }
	if(hungry && strncmp(ptr, "\"", 1) == 0) { ptr += 1; hungry = false; }
	
	const char* str_begin = ptr;
	
	if(!hungry) {
		while(*ptr != '\"' && *ptr != '\0') {
			eat_literal_char(ptr);
		}
		
		CppToken& token = tokens.emplace_back();
		token.type = CPP_STRING_LITERAL;
		token.str_begin = str_begin;
		token.str_end = ptr;
		
		if(*ptr != '\0') ptr++;
		
		return true;
	}
	
	return false;
}

static void eat_literal_char(char*& ptr) {
	if(*ptr == '\\') {
		ptr++;
		if(*ptr == '\''
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
		} else if(*ptr == 'x') {
			// hexadecimal-escape-sequence
			ptr++;
			while((*ptr >= '0' && *ptr <= '9') || (*ptr >= 'A' && *ptr <= 'F') || (*ptr >= 'a' && *ptr <= 'f')) ptr++;
		} else {
			// octal-escape-sequence
			while(*ptr >= '0' && *ptr <= '7') ptr++;
		}
	} else {
		ptr++;
	}
}

static bool eat_boolean_literal(std::vector<CppToken>& tokens, char*& ptr) {
	const char* str_begin = ptr;
	
	if(strncmp(ptr, "false", 5) == 0 && !is_literal_char(ptr[5])) {
		ptr += 5;
		
		CppToken& token = tokens.emplace_back();
		token.type = CPP_BOOLEAN_LITERAL;
		token.str_begin = str_begin;
		token.str_end = ptr;
		token.i = false;
		
		return true;
	}
	if(strncmp(ptr, "true", 4) == 0 && !is_literal_char(ptr[4])) {
		ptr += 4;
		
		CppToken& token = tokens.emplace_back();
		token.type = CPP_BOOLEAN_LITERAL;
		token.str_begin = str_begin;
		token.str_end = ptr;
		token.i = true;
		
		return true;
	}
	return false;
}

static bool eat_pointer_literal(std::vector<CppToken>& tokens, char*& ptr) {
	const char* str_begin = ptr;
	
	if(strncmp(ptr, "nullptr", 7) == 0 && !is_literal_char(ptr[7])) {
		ptr += 7;
		
		CppToken& token = tokens.emplace_back();
		token.type = CPP_POINTER_LITERAL;
		token.str_begin = str_begin;
		token.str_end = ptr;
		token.i = true;
		
		return true;
	}
	return false;
}

static bool eat_identifier(std::vector<CppToken>& tokens, char*& ptr) {
	if((*ptr >= 'A' && *ptr <= 'Z') || (*ptr >= 'a' && *ptr <= 'z') || *ptr == '_') {
		const char* str_begin = ptr;
		ptr++;
		while(is_literal_char(*ptr)) {
			ptr++;
		}
		CppToken& token = tokens.emplace_back();
		token.type = CPP_IDENTIFIER;
		token.str_begin = str_begin;
		token.str_end = ptr;
		return true;
	}
	return false;
}

static bool is_literal_char(char c) {
	return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
}

const char* cpp_token_type(CppTokenType type) {
	switch(type) {
		case CPP_COMMENT: return "comment";
		case CPP_IDENTIFIER: return "identifier";
		case CPP_KEYWORD: return "keyword";
		case CPP_BOOLEAN_LITERAL: return "boolean literal";
		case CPP_CHARACTER_LITERAL: return "character literal";
		case CPP_FLOATING_POINT_LITERAL: return "floating point literal";
		case CPP_INTEGER_LITERAL: return "integer literal";
		case CPP_POINTER_LITERAL: return "pointer literal";
		case CPP_STRING_LITERAL: return "string literal";
		case CPP_OPERATOR: return "operator";
	}
	return "(invalid token)";
}
