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

#ifndef INSTANCEMGR_CPP_LEXER_H
#define INSTANCEMGR_CPP_LEXER_H

#include <core/util.h>

// Special-purpose C++ lexer written based on the C++20 spec. Lots of features
// are missing compared to a proper C++ compiler, for example there is no logic
// for executing preprocessor macros since that's not relevant for our use case.

enum CppKeyword {
	CPP_KEYWORD_NONE = 0,
#define DEF_CPP_KEYWORD(keyword) CPP_KEYWORD_##keyword,
#include "cpp_keywords.h"
#undef DEF_CPP_KEYWORD
};

struct CppKeywordTableEntry {
	CppKeyword keyword;
	const char* string;
};
extern const CppKeywordTableEntry CPP_KEYWORDS[];
extern const s32 CPP_KEYWORD_COUNT;

#define CPP_MULTICHAR(str) (((str)[0] << 0) | ((str)[1] << 8) | ((str)[2] << 16) | ((str)[3] << 24))
enum CppOperator : u32 {
	CPP_OP_NONE = 0,
	#define DEF_CPP_OPERATOR(string, identifier) CPP_OP_##identifier = CPP_MULTICHAR(string),
	#include "cpp_operators.h"
	#undef DEF_CPP_OPERATOR
};

struct CppOperatorTableEntry {
	CppOperator op;
	const char* string;
};
extern const CppOperatorTableEntry CPP_OPERATORS[];
extern const s32 CPP_OPERATOR_COUNT;

enum class CppTokenType {
	COMMENT,
	IDENTIFIER,
	KEYWORD,
	LITERAL,
	OPERATOR
};

struct CppToken {
	CppTokenType type;
	const char* str_begin = nullptr;
	const char* str_end = nullptr;
	CppKeyword keyword;
	CppOperator op = CPP_OP_NONE;
};

#define CPP_NO_ERROR nullptr
const char* eat_cpp_file(std::vector<CppToken>& tokens, char* ptr);

#endif
