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

#include <catch2/catch_amalgamated.hpp>

#include <core/filesystem.h>
#include <instancemgr/cpp_lexer.h>

#ifdef __linux__
TEST_CASE("c++ lexer" "[instancemgr]") {
	srand(0);
	for(auto& entry : fs::recursive_directory_iterator("/usr/include")) {
		if(entry.is_regular_file() && (rand() % 100) == 0) {
			fs::path path = entry.path();
			if(path.extension() == ".h") {
				printf("************** TESTING %s\n", path.string().c_str());
				fflush(stdout);
				std::vector<u8> file = read_file(path, true);
				file.emplace_back(0);
				std::vector<CppToken> tokens;
				const char* result = eat_cpp_file(tokens, (char*) file.data());
				if(result == CPP_NO_ERROR) {
					for(CppToken& token : tokens) {
						switch(token.type) {
							case CppTokenType::COMMENT: {
								std::string str(token.str_begin, token.str_end);
								printf("COMMENT %s\n", str.c_str());
								break;
							}
							case CppTokenType::IDENTIFIER: {
								std::string str(token.str_begin, token.str_end);
								printf("IDENTIFIER %s\n", str.c_str());
								break;
							}
							case CppTokenType::KEYWORD: {
								for(s32 i = 0; i < CPP_KEYWORD_COUNT; i++) {
									if(CPP_KEYWORDS[i].keyword == token.keyword) {
										printf("KEYWORD %s\n", CPP_KEYWORDS[i].string);
										break;
									}
								}
								break;
							}
							case CppTokenType::LITERAL: {
								std::string str(token.str_begin, token.str_end);
								printf("LITERAL %s\n", str.c_str());
								break;
							}
							case CppTokenType::OPERATOR: {
								for(s32 i = 0; i < CPP_OPERATOR_COUNT; i++) {
									if(CPP_OPERATORS[i].op == token.op) {
										printf("OPERATOR %s\n", CPP_OPERATORS[i].string);
										break;
									}
								}
								break;
							}
						}
					}
				} else {
					printf("error: %s\n(end of error)\n", result);
					exit(1);
				}
			}
		}
	}
}
#endif
