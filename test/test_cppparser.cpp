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
#include <cppparser/cpp_type.h>

#define CPP_TEST_PASSED -1
static s32 test_lexer(const char* src, std::vector<CppTokenType>&& expected);
static void print_token(const CppToken& token);

TEST_CASE("c++ lexer" "[instancemgr]") {
	CHECK(CPP_TEST_PASSED == test_lexer(
		"int dec_lit = 123;",
		{CPP_KEYWORD, CPP_IDENTIFIER, CPP_OPERATOR, CPP_INTEGER_LITERAL, CPP_OPERATOR}));
	
	CHECK(CPP_TEST_PASSED == test_lexer(
		"int hex_lit = 0x123;",
		{CPP_KEYWORD, CPP_IDENTIFIER, CPP_OPERATOR, CPP_INTEGER_LITERAL, CPP_OPERATOR}));
	
	CHECK(CPP_TEST_PASSED == test_lexer(
		"int octal_lit = 0123;",
		{CPP_KEYWORD, CPP_IDENTIFIER, CPP_OPERATOR, CPP_INTEGER_LITERAL, CPP_OPERATOR}));
	
	CHECK(CPP_TEST_PASSED == test_lexer(
		"float float_lit = 1.23f;",
		{CPP_KEYWORD, CPP_IDENTIFIER, CPP_OPERATOR, CPP_FLOATING_POINT_LITERAL, CPP_OPERATOR}));
	
	CHECK(CPP_TEST_PASSED == test_lexer(
		"char c = '\x42';",
		{CPP_KEYWORD, CPP_IDENTIFIER, CPP_OPERATOR, CPP_CHARACTER_LITERAL, CPP_OPERATOR}));
	
	CHECK(CPP_TEST_PASSED == test_lexer(
		"const char* simple_str = \"simple string\";",
		{CPP_KEYWORD, CPP_KEYWORD, CPP_OPERATOR, CPP_IDENTIFIER, CPP_OPERATOR, CPP_STRING_LITERAL, CPP_OPERATOR}));
	
	CHECK(CPP_TEST_PASSED == test_lexer(
		"const char* raw_str = R\"abc(\\\"Hello World\\\"\n(Hello\\x20World))abc\";",
		{CPP_KEYWORD, CPP_KEYWORD, CPP_OPERATOR, CPP_IDENTIFIER, CPP_OPERATOR, CPP_STRING_LITERAL, CPP_OPERATOR}));
	
	CHECK(CPP_TEST_PASSED == test_lexer(
		"struct SomeStruct {int a;}",
		{CPP_KEYWORD, CPP_IDENTIFIER, CPP_OPERATOR, CPP_KEYWORD, CPP_IDENTIFIER, CPP_OPERATOR, CPP_OPERATOR}));
}

static s32 test_lexer(const char* src, std::vector<CppTokenType>&& expected) {
	std::string str(src);
	std::vector<CppToken> tokens = eat_cpp_file(&str[0]);
	for(size_t i = 0; i < std::max(tokens.size(), expected.size()); i++) {
		print_token(tokens[i]);
		if(i >= tokens.size()) return -3;
		if(i >= expected.size()) return -4;
		if(tokens[i].type != expected[i]) {
			return (s32) i;
		}
	}
	return CPP_TEST_PASSED;
}

static void print_token(const CppToken& token) {
	switch(token.type) {
		case CPP_COMMENT: {
			std::string str(token.str_begin, token.str_end);
			UNSCOPED_INFO(stringf("comment %s\n", str.c_str()));
			break;
		}
		case CPP_IDENTIFIER: {
			std::string str(token.str_begin, token.str_end);
			UNSCOPED_INFO(stringf("identifier %s\n", str.c_str()));
			break;
		}
		case CPP_KEYWORD: {
			for(s32 i = 0; i < CPP_KEYWORD_COUNT; i++) {
				if(CPP_KEYWORDS[i].keyword == token.keyword) {
					UNSCOPED_INFO(stringf("keyword %s\n", CPP_KEYWORDS[i].string));
					break;
				}
			}
			break;
		}
		case CPP_BOOLEAN_LITERAL: {
			UNSCOPED_INFO(stringf("boolean literal %s", (token.i != 0) ? "true" : "false"));
			break;
		}
		case CPP_CHARACTER_LITERAL: {
			UNSCOPED_INFO("character literal");
			break;
		}
		case CPP_FLOATING_POINT_LITERAL: {
			UNSCOPED_INFO(stringf("floating point literal %f", token.f));
			break;
		}
		case CPP_INTEGER_LITERAL: {
			UNSCOPED_INFO(stringf("integer literal %d", token.i));
			break;
		}
		case CPP_POINTER_LITERAL: {
			UNSCOPED_INFO("pointer literal");
			break;
		}
		case CPP_STRING_LITERAL: {
			UNSCOPED_INFO("string literal");
			break;
		}
		case CPP_OPERATOR: {
			for(s32 i = 0; i < CPP_OPERATOR_COUNT; i++) {
				if(CPP_OPERATORS[i].op == token.op) {
					UNSCOPED_INFO(stringf("operator %s\n", CPP_OPERATORS[i].string));
					break;
				}
			}
			break;
		}
	}
}

static bool test_parser(const char* src, CppType&& expected);
static bool compare_pvar_types(const CppType& lhs, const CppType& rhs);

TEST_CASE("c++ parser" "[instancemgr]") {
	CHECK(test_parser(
		"struct SomeVars { int array_of_ints[5]; };",
		[]() {
			CppType type(CPP_STRUCT_OR_UNION);
			type.name = "SomeVars";
			CppType& field = type.struct_or_union.fields.emplace_back(CPP_ARRAY);
			field.name = "array_of_ints";
			field.array.element_count = 5;
			field.array.element_type = std::make_unique<CppType>(CPP_BUILT_IN);
			field.array.element_type->built_in = CppBuiltIn::INT;
			return type;
		}()
	));
	CHECK(test_parser(
		"union Union { float **double_pointer; };",
		[]() {
			CppType type(CPP_STRUCT_OR_UNION);
			type.name = "Union";
			type.struct_or_union.is_union = true;
			CppType& field = type.struct_or_union.fields.emplace_back(CPP_POINTER_OR_REFERENCE);
			field.name = "double_pointer";
			field.pointer_or_reference.value_type = std::make_unique<CppType>(CPP_POINTER_OR_REFERENCE);
			CppType& inner = *field.pointer_or_reference.value_type.get();
			inner.pointer_or_reference.value_type = std::make_unique<CppType>(CPP_BUILT_IN);
			inner.pointer_or_reference.value_type->built_in = CppBuiltIn::FLOAT;
			return type;
		}()
	));
}

static bool test_parser(const char* src, CppType&& expected) {
	std::string str(src);
	std::vector<CppToken> tokens = eat_cpp_file(&str[0]);
	std::vector<CppType> types = parse_cpp_types(tokens);
	if(types.size() != 1) {
		return false;
	}
	return compare_pvar_types(types[0], expected);
}

static bool compare_pvar_types(const CppType& lhs, const CppType& rhs) {
	if(lhs.name != rhs.name) { UNSCOPED_INFO("name"); return false; }
	if(lhs.offset != rhs.offset) { UNSCOPED_INFO("offset"); return false; }
	if(lhs.size != rhs.size) { UNSCOPED_INFO("size"); return false; }
	if(lhs.alignment != rhs.alignment) { UNSCOPED_INFO("alignment"); return false; }
	if(lhs.descriptor != rhs.descriptor) { UNSCOPED_INFO("descriptor"); return false; }
	switch(lhs.descriptor) {
		case CPP_ARRAY: {
			if(lhs.array.element_count != rhs.array.element_count) { UNSCOPED_INFO("array.element_count"); return false; }
			REQUIRE((lhs.array.element_type.get() && rhs.array.element_type.get()));
			bool comp_result = compare_pvar_types(*lhs.array.element_type.get(), *rhs.array.element_type.get());
			if(!comp_result) { UNSCOPED_INFO("array.element_type"); return false; }
			break;
		}
		case CPP_BUILT_IN: {
			if(lhs.built_in != rhs.built_in) { UNSCOPED_INFO("built_in"); return false; }
			break;
		}
		case CPP_STRUCT_OR_UNION: {
			if(lhs.struct_or_union.is_union != rhs.struct_or_union.is_union) { UNSCOPED_INFO("struct_or_union.is_union"); return false; }
			if(lhs.struct_or_union.fields.size() != rhs.struct_or_union.fields.size()) { UNSCOPED_INFO("struct_or_union.fields.size()"); return false; }
			for(s32 i = 0; i < (s32) lhs.struct_or_union.fields.size(); i++) {
				bool comp_result = compare_pvar_types(lhs.struct_or_union.fields[i], rhs.struct_or_union.fields[i]);
				if(!comp_result) { UNSCOPED_INFO(stringf("struct_or_union.fields[%d]", i)); return false; }
			}
			break;
		}
		case CPP_TYPE_NAME: {
			UNSCOPED_INFO("unhandled type name");
			return false;
		}
		case CPP_POINTER_OR_REFERENCE: {
			if(lhs.pointer_or_reference.is_reference != rhs.pointer_or_reference.is_reference) { UNSCOPED_INFO("pointer_or_reference.is_reference"); return false; }
			REQUIRE((lhs.pointer_or_reference.value_type.get() && rhs.pointer_or_reference.value_type.get()));
			bool comp_result = compare_pvar_types(*lhs.pointer_or_reference.value_type.get(), *rhs.pointer_or_reference.value_type.get());
			if(!comp_result) { UNSCOPED_INFO("pointer_or_reference.value_type"); return false; }
			break;
		}
	}
	return true;
}
