/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2022 chaoticgd

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

#include <core/util.h>
#include <core/wtf.h>
#include <core/filesystem.h>

static void generate_asset_type(const WtfNode* asset_type);
static void generate_asset_read_function(const WtfNode* asset_type);
static void generate_read_attribute_code(const WtfNode* node, const char* result, const char* attrib, s32 depth);
static void generate_attribute_type_check_code(const std::string& attribute, const char* expected, s32 ind);
static void generate_asset_write_function(const WtfNode* asset_type);
static void generate_asset_write_code(const WtfNode* node, const char* expr, s32 depth);
static std::string node_to_cpp_type(const WtfNode* node);
static std::string PascalCase_to_SCREAMING_SNAKE_CASE(const char* input);
static void indent(s32 ind);

int main(int argc, char** argv) {
	assert(argc == 2);
	auto bytes = read_file(argv[1], "r");
	bytes.push_back(0);
	
	char* error = NULL;
	WtfNode* root = wtf_parse((char*) bytes.data(), &error);
	verify(!error, "Failed to parse asset schema. %s", error);
	
	printf("// Generated from %s. Do not edit.\n\n", argv[1]);
	
	printf("#ifdef GENERATED_ASSET_ENUM\n\n");
	printf("enum class AssetType {\n");
	for(const WtfNode* node = wtf_first_child(root, "AssetType"); node != NULL; node = wtf_next_sibling(node, "AssetType")) {
		std::string name = PascalCase_to_SCREAMING_SNAKE_CASE(node->tag);
		printf("\t%s%s\n", name.c_str(), wtf_next_sibling(node, "AssetType") != NULL ? "," : "");
	}
	printf("};\n\n");
	printf("#endif\n\n");
	
	printf("// *****************************************************************************\n");
	printf("// Classes\n");
	printf("// *****************************************************************************\n\n");
	
	printf("#ifdef GENERATED_ASSET_HEADER\n");
	for(const WtfNode* node = wtf_first_child(root, "AssetType"); node != NULL; node = wtf_next_sibling(node, "AssetType")) {
		generate_asset_type(node);
	}
	printf("\n#endif\n\n");
	
	printf("// *****************************************************************************\n");
	printf("// Implementation\n");
	printf("// *****************************************************************************\n\n");
	
	printf("#ifdef GENERATED_ASSET_IMPLEMENTATION\n");
	const WtfNode* first_asset_type = wtf_first_child(root, "AssetType");
	for(const WtfNode* node = first_asset_type; node != NULL; node = wtf_next_sibling(node, "AssetType")) {
		if(node != first_asset_type) {
			printf("\n// *****************************************************************************\n");
		}
		
		std::string enum_name = PascalCase_to_SCREAMING_SNAKE_CASE(node->tag);
		printf("\n%sAsset::%sAsset() : Asset(AssetType::%s) {}\n\n", node->tag, node->tag, enum_name.c_str());
		
		generate_asset_read_function(node);
		generate_asset_write_function(node);
	}
	printf("#endif\n");
	
	wtf_free(root);
}

static void generate_asset_type(const WtfNode* asset_type) {
	printf("\nclass %sAsset : public Asset {\n", asset_type->tag);
	for(WtfNode* node = asset_type->first_child; node != NULL; node = node->next_sibling) {
		std::string cpp_type = node_to_cpp_type(node);
		if(!cpp_type.empty()) {
			printf("\t%s _attribute_%s;\n", cpp_type.c_str(), node->tag);
		}
	}
	printf("public:\n");
	printf("\t%sAsset();\n", asset_type->tag);
	printf("\t\n");
	printf("\tvoid read_attributes(const WtfNode* node) override;\n");
	printf("\tvoid write_attributes(WtfWriter* ctx) const override;\n");
	printf("\t\n");
	bool first = true;
	for(WtfNode* node = asset_type->first_child; node != NULL; node = node->next_sibling) {
		std::string cpp_type = node_to_cpp_type(node);
		if(!cpp_type.empty()) {
			if(first) {
				printf("\t\n");
				first = false;
			}
			printf("\tconst %s& %s() const;\n", cpp_type.c_str(), node->tag);
			printf("\tvoid set_%s(%s new_%s);\n", node->tag, cpp_type.c_str(), node->tag);
		}
	}
	printf("};\n");
}

static void generate_asset_read_function(const WtfNode* asset_type) {
	printf("void %sAsset::read_attributes(const WtfNode* node) {\n", asset_type->tag);
	for(WtfNode* node = asset_type->first_child; node != NULL; node = node->next_sibling) {
		std::string type = node_to_cpp_type(node);
		if(!type.empty()) {
			printf("\t{\n");
			std::string result = std::string("_attribute_") + node->tag;
			const char* attrib = "attribute_0";
			printf("\t\tWtfAttribute* %s = wtf_attribute(node, \"%s\");\n", attrib, node->tag);
			printf("\t\tif(%s) {\n", attrib);
			generate_read_attribute_code(node, result.c_str(), attrib, 1);
			printf("\t\t} else {\n");
			const WtfAttribute* def = wtf_attribute(node, "default");
			if(def) {
				printf("\t\t\t// TODO: Default attributes.");
			} else {
				printf("\t\t\tthrow MissingAssetAttribute(node, \"%s\");\n", node->tag);
			}
			printf("\t\t}\n");
			printf("\t}\n");
		}
	}
	printf("}\n\n");
}

static void generate_read_attribute_code(const WtfNode* node, const char* result, const char* attrib, s32 depth) {
	s32 ind = depth + 2;
	
	if(strcmp(node->type_name, "StringAttribute") == 0) {
		generate_attribute_type_check_code(attrib, "WTF_STRING", ind);
		indent(ind); printf("std::string %s = %s->string;\n", result, attrib);
	}
	
	if(strcmp(node->type_name, "ArrayAttribute") == 0) {
		generate_attribute_type_check_code(attrib, "WTF_ARRAY", ind);
		const WtfNode* element = wtf_child(node, NULL, "element");
		assert(element);
		std::string element_type = node_to_cpp_type(element);
		std::string element_result = std::string("temp_") + std::to_string(depth);
		std::string element_attrib = "element_" + std::to_string(depth);
		indent(ind); printf("for(WtfAttribute* %s = %s->first_array_element; %s != NULL; %s = %s->next) {\n",
			element_attrib.c_str(), attrib, element_attrib.c_str(), element_attrib.c_str(), element_attrib.c_str());
		indent(ind); printf("\t%s %s;\n", element_type.c_str(), element_result.c_str());
		generate_read_attribute_code(element, element_result.c_str(), element_attrib.c_str(), depth + 1);
		indent(ind); printf("\t%s.emplace_back(std::move(%s));\n", result, element_result.c_str());
		indent(ind); printf("}\n");
	}
	
	if(strcmp(node->type_name, "AssetReferenceAttribute") == 0) {
		generate_attribute_type_check_code(attrib, "WTF_STRING", ind);
		indent(ind); printf("%s = AssetReference(%s->string);\n", result, attrib);
	}
	
	if(strcmp(node->type_name, "FileReferenceAttribute") == 0) {
		generate_attribute_type_check_code(attrib, "WTF_STRING", ind);
		indent(ind); printf("%s = FileReference(%s->string);\n", result, attrib);
	}
}

static void generate_attribute_type_check_code(const std::string& attribute, const char* expected, s32 ind) {
	indent(ind); printf("if(attribute->type != %s) {\n", expected);
	indent(ind); printf("\tthrow InvalidAssetAttributeType(node, %s);\n", attribute.c_str());
	indent(ind); printf("}\n");
}

static void generate_asset_write_function(const WtfNode* asset_type) {
	printf("void %sAsset::write_attributes(WtfWriter* ctx) const {\n", asset_type->tag);
	for(WtfNode* node = asset_type->first_child; node != NULL; node = node->next_sibling) {
		std::string type = node_to_cpp_type(node);
		if(!type.empty()) {
			printf("\t{\n");
			printf("\t\twtf_begin_attribute(ctx, \"%s\");\n", node->tag);
			std::string expr = std::string("_attribute_") + node->tag;
			generate_asset_write_code(node, expr.c_str(), 1);
			printf("\t\twtf_end_attribute(ctx);\n");
			printf("\t}\n");
		}
	}
	printf("}\n\n");
}

static void generate_asset_write_code(const WtfNode* node, const char* expr, s32 depth) {
	s32 ind = depth + 1;
	
	if(strcmp(node->type_name, "StringAttribute") == 0) {
		indent(ind); printf("wtf_write_string(ctx, %s.c_str());\n", expr);
	}
	
	if(strcmp(node->type_name, "ArrayAttribute") == 0) {
		const WtfNode* element = wtf_child(node, NULL, "element");
		assert(element);
		std::string element_expr = "element_" + std::to_string(depth);
		indent(ind); printf("wtf_begin_array(ctx);\n");
		indent(ind); printf("for(const auto& element : %s) {\n", expr);
		generate_asset_write_code(element, element_expr.c_str(), depth + 1);
		indent(ind); printf("}\n");
		indent(ind); printf("wtf_end_array(ctx);\n");
	}
	
	if(strcmp(node->type_name, "AssetReferenceAttribute") == 0) {
		indent(ind); printf("wtf_write_string(ctx, %s.c_str());\n", expr);
	}
	
	if(strcmp(node->type_name, "FileReferenceAttribute") == 0) {
		indent(ind); printf("wtf_write_string(ctx, %s.c_str());\n", expr);
	}
}

static std::string node_to_cpp_type(const WtfNode* node) {
	if(strcmp(node->type_name, "StringAttribute") == 0) {
		return "std::string";
	}
	
	if(strcmp(node->type_name, "ArrayAttribute") == 0) {
		const WtfNode* element = wtf_child(node, NULL, "element");
		assert(element);
		std::string element_type = node_to_cpp_type(element);
		assert(!element_type.empty());
		return "std::vector<" + element_type + ">";
	}
	
	if(strcmp(node->type_name, "AssetReferenceAttribute") == 0) {
		return "AssetReference";
	}
	
	if(strcmp(node->type_name, "FileReferenceAttribute") == 0) {
		return "FileReference";
	}
	
	return "";
}

static std::string PascalCase_to_SCREAMING_SNAKE_CASE(const char* input) {
	std::string output;
	for(const char* src = input; *src != '\0'; src++) {
		if(src != input && isupper(*src)) {
			output += '_';
		}
		output += toupper(*src);
	}
	return output;
}

static void indent(s32 ind) {
	for(s32 i = 0; i < ind; i++) {
		printf("\t");
	}
}
