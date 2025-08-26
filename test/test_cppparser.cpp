/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2025 chaoticgd

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
#include <cppparser/cpp_parser.h>

#define CPP_TEST_PASSED -1
static s32 test_lexer(const char* src, std::vector<CppTokenType>&& expected);
static void print_token(const CppToken& token);
static bool test_parser(const char* src, CppType&& expected);
static bool test_layout(const char* src, CppType&& expected);
static bool compare_cpp_types(const CppType& lhs, const CppType& rhs);

TEST_CASE("c++ lexer", "[cpp]")
{
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
	
	CHECK(CPP_TEST_PASSED == test_lexer(
		"enum SomeEnum {A=1}",
		{CPP_KEYWORD, CPP_IDENTIFIER, CPP_OPERATOR, CPP_IDENTIFIER, CPP_OPERATOR, CPP_INTEGER_LITERAL, CPP_OPERATOR}));
}

static s32 test_lexer(const char* src, std::vector<CppTokenType>&& expected)
{
	std::string str(src);
	std::vector<CppToken> tokens = eat_cpp_file(&str[0]);
	for (size_t i = 0; i < std::max(tokens.size(), expected.size()); i++) {
		print_token(tokens[i]);
		if (i >= tokens.size()) return -3;
		if (i >= expected.size()) return -4;
		if (tokens[i].type != expected[i]) {
			return (s32) i;
		}
	}
	return CPP_TEST_PASSED;
}

static void print_token(const CppToken& token)
{
	switch (token.type) {
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
		case CPP_PREPROCESSOR_DIRECTIVE: {
			UNSCOPED_INFO("preprocessor directive");
			break;
		}
	}
}

TEST_CASE("c++ parser", "[cpp]")
{
	CHECK(test_parser(
		"struct SomeVars { int array_of_ints[5]; };",
		[]() {
			CppType type(CPP_STRUCT_OR_UNION);
			type.name = "SomeVars";
			CppType& field = type.struct_or_union.fields.emplace_back(CPP_ARRAY);
			field.name = "array_of_ints";
			field.array.element_count = 5;
			field.array.element_type = std::make_unique<CppType>(CPP_BUILT_IN);
			field.array.element_type->built_in = CPP_INT;
			return type;
		}()
	));
	CHECK(test_parser(
		"struct /* comment */ SomeVars /* comment */ { s8 byte; s16 halfword; s32 word; s64 doubleword; s128 quadword; };",
		[]() {
			CppType type(CPP_STRUCT_OR_UNION);
			
			type.name = "SomeVars";
			CppType& byte = type.struct_or_union.fields.emplace_back(CPP_BUILT_IN);
			byte.name = "byte";
			byte.built_in = CPP_S8;
			
			CppType& halfword = type.struct_or_union.fields.emplace_back(CPP_BUILT_IN);
			halfword.name = "halfword";
			halfword.built_in = CPP_S16;
			
			CppType& word = type.struct_or_union.fields.emplace_back(CPP_BUILT_IN);
			word.name = "word";
			word.built_in = CPP_S32;
			
			CppType& doubleword = type.struct_or_union.fields.emplace_back(CPP_BUILT_IN);
			doubleword.name = "doubleword";
			doubleword.built_in = CPP_S64;
			
			CppType& quadword = type.struct_or_union.fields.emplace_back(CPP_BUILT_IN);
			quadword.name = "quadword";
			quadword.built_in = CPP_S128;
			
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
			inner.pointer_or_reference.value_type->built_in = CPP_FLOAT;
			return type;
		}()
	));
	CHECK(test_parser(
		"struct alignas(64) CharInABox { char c; };",
		[]() {
			CppType type(CPP_STRUCT_OR_UNION);
			type.name = "CharInABox";
			type.alignment = 64;
			CppType& field = type.struct_or_union.fields.emplace_back(CPP_BUILT_IN);
			field.name = "c";
			field.built_in = CPP_CHAR;
			return type;
		}()
	));
	CHECK(test_parser(
		"enum Enum { A = 1, B = 2, C = 3 };",
		[]() {
			CppType type(CPP_ENUM);
			type.name = "Enum";
			type.enumeration.constants.emplace_back(1, "A");
			type.enumeration.constants.emplace_back(2, "B");
			type.enumeration.constants.emplace_back(3, "C");
			return type;
		}()
	));
	CHECK(test_parser(
		"#pragma wrench bitflags ThingFlags\ntypedef int Thing;",
		[]() {
			CppType type(CPP_BUILT_IN);
			type.name = "Thing";
			type.preprocessor_directives.emplace_back(CPP_DIRECTIVE_BITFLAGS, "ThingFlags");
			type.built_in = CPP_INT;
			return type;
		}()
	));
	CHECK(test_parser(
		"struct S {\n#pragma wrench enum Enum\nint var;};",
		[]() {
			CppType type(CPP_STRUCT_OR_UNION);
			type.name = "S";
			CppType& field = type.struct_or_union.fields.emplace_back(CPP_BUILT_IN);
			field.name = "var";
			field.preprocessor_directives.emplace_back(CPP_DIRECTIVE_ENUM, "Enum");
			field.built_in = CPP_INT;
			return type;
		}()
	));
	CHECK(test_parser(
		"struct S { int x : 12; };",
		[]() {
			CppType type(CPP_STRUCT_OR_UNION);
			type.name = "S";
			CppType& field = type.struct_or_union.fields.emplace_back(CPP_BITFIELD);
			field.name = "x";
			field.bitfield.bit_size = 12;
			field.bitfield.storage_unit_type = std::make_unique<CppType>(CPP_BUILT_IN);
			field.bitfield.storage_unit_type->built_in = CPP_INT;
			return type;
		}()
	));
}

static bool test_parser(const char* src, CppType&& expected)
{
	std::string str;
	str += "#pragma wrench parser on\n";
	str += src;
	std::vector<CppToken> tokens = eat_cpp_file(&str[0]);
	std::map<std::string, CppType> types;
	parse_cpp_types(types, tokens);
	if (types.size() != 1) {
		UNSCOPED_INFO("types.size() != 1");
		return false;
	}
	return compare_cpp_types(types.begin()->second, expected);
}

TEST_CASE("c++ layout", "[cpp]")
{
	CHECK(test_layout(
		"struct S { int a; int b; int c; };",
		[]() {
			CppType type(CPP_STRUCT_OR_UNION);
			type.name = "S";
			type.size = 12;
			type.alignment = 4;
			CppType& a = type.struct_or_union.fields.emplace_back(CPP_BUILT_IN);
			a.name = "a";
			a.offset = 0;
			a.size = 4;
			a.alignment = 4;
			a.built_in = CPP_INT;
			CppType& b = type.struct_or_union.fields.emplace_back(CPP_BUILT_IN);
			b.name = "b";
			b.offset = 4;
			b.size = 4;
			b.alignment = 4;
			b.built_in = CPP_INT;
			CppType& c = type.struct_or_union.fields.emplace_back(CPP_BUILT_IN);
			c.name = "c";
			c.offset = 8;
			c.size = 4;
			c.alignment = 4;
			c.built_in = CPP_INT;
			return type;
		}()
	));
	CHECK(test_layout(
		"struct S { char a; int b; };",
		[]() {
			CppType type(CPP_STRUCT_OR_UNION);
			type.name = "S";
			type.size = 8;
			type.alignment = 4;
			CppType& a = type.struct_or_union.fields.emplace_back(CPP_BUILT_IN);
			a.name = "a";
			a.offset = 0;
			a.size = 1;
			a.alignment = 1;
			a.built_in = CPP_CHAR;
			CppType& b = type.struct_or_union.fields.emplace_back(CPP_BUILT_IN);
			b.name = "b";
			b.offset = 4;
			b.size = 4;
			b.alignment = 4;
			b.built_in = CPP_INT;
			return type;
		}()
	));
	CHECK(test_layout(
		"struct S { int a; int b : 12; int c : 20; };",
		[]() {
			CppType type(CPP_STRUCT_OR_UNION);
			type.name = "S";
			type.size = 8;
			type.alignment = 4;
			CppType& a = type.struct_or_union.fields.emplace_back(CPP_BUILT_IN);
			a.name = "a";
			a.offset = 0;
			a.size = 4;
			a.alignment = 4;
			a.built_in = CPP_INT;
			CppType& b = type.struct_or_union.fields.emplace_back(CPP_BITFIELD);
			b.name = "b";
			b.offset = 4;
			b.size = 4;
			b.alignment = 4;
			b.bitfield.bit_offset = 0;
			b.bitfield.bit_size = 12;
			b.bitfield.storage_unit_type = std::make_unique<CppType>(CPP_BUILT_IN);
			b.bitfield.storage_unit_type->size = 4;
			b.bitfield.storage_unit_type->alignment = 4;
			b.bitfield.storage_unit_type->built_in = CPP_INT;
			CppType& c = type.struct_or_union.fields.emplace_back(CPP_BITFIELD);
			c.name = "c";
			c.offset = 4;
			c.size = 4;
			c.alignment = 4;
			c.bitfield.bit_offset = 12;
			c.bitfield.bit_size = 20;
			c.bitfield.storage_unit_type = std::make_unique<CppType>(CPP_BUILT_IN);
			c.bitfield.storage_unit_type->size = 4;
			c.bitfield.storage_unit_type->alignment = 4;
			c.bitfield.storage_unit_type->built_in = CPP_INT;
			return type;
		}()
	));
	CHECK(test_layout(
		"struct S { int a : 12; int b : 20; int c : 32; };",
		[]() {
			CppType type(CPP_STRUCT_OR_UNION);
			type.name = "S";
			type.size = 8;
			type.alignment = 4;
			CppType& a = type.struct_or_union.fields.emplace_back(CPP_BITFIELD);
			a.name = "a";
			a.offset = 0;
			a.size = 4;
			a.alignment = 4;
			a.bitfield.bit_offset = 0;
			a.bitfield.bit_size = 12;
			a.bitfield.storage_unit_type = std::make_unique<CppType>(CPP_BUILT_IN);
			a.bitfield.storage_unit_type->size = 4;
			a.bitfield.storage_unit_type->alignment = 4;
			a.bitfield.storage_unit_type->built_in = CPP_INT;
			CppType& b = type.struct_or_union.fields.emplace_back(CPP_BITFIELD);
			b.name = "b";
			b.offset = 0;
			b.size = 4;
			b.alignment = 4;
			b.bitfield.bit_offset = 12;
			b.bitfield.bit_size = 20;
			b.bitfield.storage_unit_type = std::make_unique<CppType>(CPP_BUILT_IN);
			b.bitfield.storage_unit_type->size = 4;
			b.bitfield.storage_unit_type->alignment = 4;
			b.bitfield.storage_unit_type->built_in = CPP_INT;
			CppType& c = type.struct_or_union.fields.emplace_back(CPP_BITFIELD);
			c.name = "c";
			c.offset = 4;
			c.size = 4;
			c.alignment = 4;
			c.bitfield.bit_offset = 0;
			c.bitfield.bit_size = 32;
			c.bitfield.storage_unit_type = std::make_unique<CppType>(CPP_BUILT_IN);
			c.bitfield.storage_unit_type->size = 4;
			c.bitfield.storage_unit_type->alignment = 4;
			c.bitfield.storage_unit_type->built_in = CPP_INT;
			return type;
		}()
	));
}

TEST_CASE("c++ bitfield operations", "[cpp]")
{
	CHECK(cpp_unpack_unsigned_bitfield(0xff00, 8, 4) == 0xf);
	CHECK(cpp_pack_unsigned_bitfield(0xf, 8, 8) == 0xf00);
	CHECK(cpp_zero_bitfield(0xffff, 4, 4) == 0xff0f);
}

static bool test_layout(const char* src, CppType&& expected)
{
	std::string str;
	str += "#pragma wrench parser on\n";
	str += src;
	std::vector<CppToken> tokens = eat_cpp_file(&str[0]);
	std::map<std::string, CppType> types;
	parse_cpp_types(types, tokens);
	if(types.size() != 1) {
		UNSCOPED_INFO("types.size() != 1");
		return false;
	}
	layout_cpp_type(types.begin()->second, types, CPP_PS2_ABI);
	return compare_cpp_types(types.begin()->second, expected);
}

static bool compare_cpp_types(const CppType& lhs, const CppType& rhs)
{
	if (lhs.name != rhs.name) { UNSCOPED_INFO("name"); return false; }
	if (lhs.offset != rhs.offset) { UNSCOPED_INFO("offset"); return false; }
	if (lhs.size != rhs.size) { UNSCOPED_INFO("size"); return false; }
	if (lhs.alignment != rhs.alignment) { UNSCOPED_INFO("alignment"); return false; }
	if (lhs.preprocessor_directives != rhs.preprocessor_directives) { UNSCOPED_INFO("preprocessor_directives"); return false; }
	if (lhs.descriptor != rhs.descriptor) { UNSCOPED_INFO("descriptor"); return false; }
	switch (lhs.descriptor) {
		case CPP_ARRAY: {
			if (lhs.array.element_count != rhs.array.element_count) { UNSCOPED_INFO("array.element_count"); return false; }
			REQUIRE((lhs.array.element_type.get() && rhs.array.element_type.get()));
			bool comp_result = compare_cpp_types(*lhs.array.element_type.get(), *rhs.array.element_type.get());
			if (!comp_result) { UNSCOPED_INFO("array.element_type"); return false; }
			break;
		}
		case CPP_BITFIELD: {
			if (lhs.bitfield.bit_offset != rhs.bitfield.bit_offset) { UNSCOPED_INFO("bitfield.bit_offset"); return false; }
			if (lhs.bitfield.bit_size != rhs.bitfield.bit_size) { UNSCOPED_INFO("bitfield.bit_offset"); return false; }
			bool comp_result = compare_cpp_types(*lhs.bitfield.storage_unit_type.get(), *rhs.bitfield.storage_unit_type.get());
			if (!comp_result) { UNSCOPED_INFO("bitfield.storage_unit_type"); return false; }
			break;
		}
		case CPP_BUILT_IN: {
			if (lhs.built_in != rhs.built_in) { UNSCOPED_INFO("built_in"); return false; }
			break;
		}
		case CPP_ENUM: {
			if (lhs.enumeration.constants != rhs.enumeration.constants) { UNSCOPED_INFO("enum"); return false; }
			break;
		}
		case CPP_STRUCT_OR_UNION: {
			if (lhs.struct_or_union.is_union != rhs.struct_or_union.is_union) { UNSCOPED_INFO("struct_or_union.is_union"); return false; }
			if (lhs.struct_or_union.fields.size() != rhs.struct_or_union.fields.size()) { UNSCOPED_INFO("struct_or_union.fields.size()"); return false; }
			for (s32 i = 0; i < (s32) lhs.struct_or_union.fields.size(); i++) {
				bool comp_result = compare_cpp_types(lhs.struct_or_union.fields[i], rhs.struct_or_union.fields[i]);
				if(!comp_result) { UNSCOPED_INFO(stringf("struct_or_union.fields[%d]", i)); return false; }
			}
			break;
		}
		case CPP_TYPE_NAME: {
			UNSCOPED_INFO("unhandled type name");
			return false;
		}
		case CPP_POINTER_OR_REFERENCE: {
			if (lhs.pointer_or_reference.is_reference != rhs.pointer_or_reference.is_reference) { UNSCOPED_INFO("pointer_or_reference.is_reference"); return false; }
			REQUIRE((lhs.pointer_or_reference.value_type.get() && rhs.pointer_or_reference.value_type.get()));
			bool comp_result = compare_cpp_types(*lhs.pointer_or_reference.value_type.get(), *rhs.pointer_or_reference.value_type.get());
			if (!comp_result) { UNSCOPED_INFO("pointer_or_reference.value_type"); return false; }
			break;
		}
	}
	return true;
}
