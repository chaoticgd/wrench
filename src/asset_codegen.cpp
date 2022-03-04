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

#include <stdarg.h>

#include <core/util.h>
#include <core/wtf.h>
#include <core/filesystem.h>

static void generate_asset_type(const WtfNode* asset_type);
static void generate_asset_type_function(const WtfNode* root);
static void generate_asset_type_name_to_type_function(const WtfNode* root);
static void generate_asset_read_function(const WtfNode* asset_type);
static void generate_read_attribute_code(const WtfNode* node, const char* result, const char* attrib, s32 depth);
static void generate_attribute_type_check_code(const std::string& attribute, const char* expected, s32 ind);
static void generate_asset_write_function(const WtfNode* asset_type);
static void generate_asset_write_code(const WtfNode* node, const char* expr, s32 depth);
static std::string node_to_cpp_type(const WtfNode* node);
static void indent(s32 ind);
static void out(const char* format, ...);

static FILE* out_file;

int main(int argc, char** argv) {
	assert(argc == 2 || argc == 3);
	auto bytes = read_file(argv[1], "r");
	bytes.push_back(0);
	
	if(argc == 3) {
		out_file = fopen(argv[2], "w");
	} else {
		out_file = stdout;
	}
	
	char* error = NULL;
	WtfNode* root = wtf_parse((char*) bytes.data(), &error);
	verify(!error, "Failed to parse asset schema. %s", error);
	
	out("// Generated from %s. Do not edit.\n\n", argv[1]);
	
	out("// *****************************************************************************\n");
	out("// Header\n");
	out("// *****************************************************************************\n\n");
	
	out("#ifdef GENERATED_ASSET_HEADER\n\n");
	for(const WtfNode* node = wtf_first_child(root, "AssetType"); node != NULL; node = wtf_next_sibling(node, "AssetType")) {
		generate_asset_type(node);
	}
	generate_asset_type_function(root);
	out("AssetType asset_type_name_to_type(const char* type_name);\n");
	out("#endif\n\n");
	
	out("// *****************************************************************************\n");
	out("// Implementation\n");
	out("// *****************************************************************************\n\n");
	
	out("#ifdef GENERATED_ASSET_IMPLEMENTATION\n");
	const WtfNode* first_asset_type = wtf_first_child(root, "AssetType");
	for(const WtfNode* node = first_asset_type; node != NULL; node = wtf_next_sibling(node, "AssetType")) {
		if(node != first_asset_type) {
			out("\n");
			out("// *****************************************************************************\n");
		}
		
		out("\n");
		out("%sAsset::%sAsset(AssetManager& manager, AssetPack& pack, Asset* parent)\n", node->tag, node->tag);
		out("\t: Asset(manager, pack, parent, asset_type<%sAsset>()) {}\n\n", node->tag);
		
		generate_asset_read_function(node);
		generate_asset_write_function(node);
	}
	generate_asset_type_name_to_type_function(root);
	out("#endif\n");
	
	wtf_free(root);
}

static void generate_asset_type(const WtfNode* asset_type) {
	out("class %sAsset : public Asset {\n", asset_type->tag);
	for(WtfNode* node = asset_type->first_child; node != NULL; node = node->next_sibling) {
		std::string cpp_type = node_to_cpp_type(node);
		if(!cpp_type.empty()) {
			out("\t%s _attribute_%s;\n", cpp_type.c_str(), node->tag);
		}
	}
	out("public:\n");
	out("\t%sAsset(AssetManager& manager, AssetPack& pack, Asset* parent);\n", asset_type->tag);
	out("\t\n");
	out("\tvoid read_attributes(const WtfNode* node) override;\n");
	out("\tvoid write_attributes(WtfWriter* ctx) const override;\n");
	bool first = true;
	for(WtfNode* node = asset_type->first_child; node != NULL; node = node->next_sibling) {
		std::string cpp_type = node_to_cpp_type(node);
		if(!cpp_type.empty()) {
			if(first) {
				out("\t\n");
				first = false;
			}
			out("\t\n");
			out("\tconst %s& %s() const;\n", cpp_type.c_str(), node->tag);
			out("\tvoid set_%s(%s new_%s);\n", node->tag, cpp_type.c_str(), node->tag);
		}
	}
	out("};\n\n");
}

static void generate_asset_type_function(const WtfNode* root) {
	out("template <typename T>\n");
	out("AssetType asset_type() {\n");
	for(const WtfNode* node = wtf_first_child(root, "AssetType"); node != NULL; node = wtf_next_sibling(node, "AssetType")) {
		out("\tif constexpr(std::is_same_v<T, %sAsset>) return AssetType(\"%s\");\n", node->tag, node->tag);
	}
	out("\treturn NULL_ASSET_TYPE;\n");
	out("}\n\n");
}

static void generate_asset_type_name_to_type_function(const WtfNode* root) {
	out("AssetType asset_type_name_to_type(const char* type_name) {\n");
	for(const WtfNode* node = wtf_first_child(root, "AssetType"); node != NULL; node = wtf_next_sibling(node, "AssetType")) {
		out("\tif(strcmp(type_name, \"%s\") == 0) return asset_type<%sAsset>();\n", node->tag, node->tag);
	}
	out("\treturn NULL_ASSET_TYPE;\n");
	out("}\n\n");
}

static void generate_asset_read_function(const WtfNode* asset_type) {
	bool first = true;
	out("void %sAsset::read_attributes(const WtfNode* node) {\n", asset_type->tag);
	for(WtfNode* node = asset_type->first_child; node != NULL; node = node->next_sibling) {
		std::string type = node_to_cpp_type(node);
		if(!type.empty()) {
			if(!first) {
				out("\t\n");
			} else {
				first = false;
			}
			std::string result = std::string("_attribute_") + node->tag;
			std::string attrib = std::string("attribute_") + node->tag;
			out("\tconst WtfAttribute* %s = wtf_attribute(node, \"%s\");\n", attrib.c_str(), node->tag);
			out("\tif(%s) {\n", attrib.c_str());
			generate_read_attribute_code(node, result.c_str(), attrib.c_str(), 0);
			out("\t} else {\n");
			const WtfAttribute* def = wtf_attribute(node, "default");
			if(def) {
				out("\t\t// TODO: Default attributes.");
			} else {
				out("\t\tthrow MissingAssetAttribute(node, \"%s\");\n", node->tag);
			}
			out("\t}\n");
		}
	}
	out("}\n\n");
}

static void generate_read_attribute_code(const WtfNode* node, const char* result, const char* attrib, s32 depth) {
	s32 ind = depth + 2;
	
	if(strcmp(node->type_name, "StringAttribute") == 0) {
		generate_attribute_type_check_code(attrib, "WTF_STRING", ind);
		indent(ind); out("std::string %s = %s->string;\n", result, attrib);
	}
	
	if(strcmp(node->type_name, "ArrayAttribute") == 0) {
		generate_attribute_type_check_code(attrib, "WTF_ARRAY", ind);
		const WtfNode* element = wtf_child(node, NULL, "element");
		assert(element);
		std::string element_type = node_to_cpp_type(element);
		std::string element_result = std::string("temp_") + std::to_string(depth);
		std::string element_attrib = "element_" + std::to_string(depth);
		indent(ind); out("for(const WtfAttribute* %s = %s->first_array_element; %s != NULL; %s = %s->next) {\n",
			element_attrib.c_str(), attrib, element_attrib.c_str(), element_attrib.c_str(), element_attrib.c_str());
		indent(ind); out("\t%s %s;\n", element_type.c_str(), element_result.c_str());
		generate_read_attribute_code(element, element_result.c_str(), element_attrib.c_str(), depth + 1);
		indent(ind); out("\t%s.emplace_back(std::move(%s));\n", result, element_result.c_str());
		indent(ind); out("}\n");
	}
	
	if(strcmp(node->type_name, "AssetReferenceAttribute") == 0) {
		generate_attribute_type_check_code(attrib, "WTF_STRING", ind);
		indent(ind); out("%s = parse_asset_reference(%s->string);\n", result, attrib);
	}
	
	if(strcmp(node->type_name, "FileReferenceAttribute") == 0) {
		generate_attribute_type_check_code(attrib, "WTF_STRING", ind);
		indent(ind); out("%s = FileReference{%s->string};\n", result, attrib);
	}
}

static void generate_attribute_type_check_code(const std::string& attribute, const char* expected, s32 ind) {
	indent(ind); out("if(%s->type != %s) {\n", attribute.c_str(), expected);
	indent(ind); out("\tthrow InvalidAssetAttributeType(node, %s);\n", attribute.c_str());
	indent(ind); out("}\n");
}

static void generate_asset_write_function(const WtfNode* asset_type) {
	out("void %sAsset::write_attributes(WtfWriter* ctx) const {\n", asset_type->tag);
	for(WtfNode* node = asset_type->first_child; node != NULL; node = node->next_sibling) {
		std::string type = node_to_cpp_type(node);
		if(!type.empty()) {
			out("\t{\n");
			out("\t\twtf_begin_attribute(ctx, \"%s\");\n", node->tag);
			std::string expr = std::string("_attribute_") + node->tag;
			generate_asset_write_code(node, expr.c_str(), 0);
			out("\t\twtf_end_attribute(ctx);\n");
			out("\t}\n");
		}
	}
	out("}\n\n");
}

static void generate_asset_write_code(const WtfNode* node, const char* expr, s32 depth) {
	s32 ind = depth + 2;
	
	if(strcmp(node->type_name, "StringAttribute") == 0) {
		indent(ind); out("wtf_write_string(ctx, %s.c_str());\n", expr);
	}
	
	if(strcmp(node->type_name, "ArrayAttribute") == 0) {
		const WtfNode* element = wtf_child(node, NULL, "element");
		assert(element);
		std::string element_expr = "element_" + std::to_string(depth);
		indent(ind); out("wtf_begin_array(ctx);\n");
		indent(ind); out("for(const auto& %s : %s) {\n", element_expr.c_str(), expr);
		generate_asset_write_code(element, element_expr.c_str(), depth + 1);
		indent(ind); out("}\n");
		indent(ind); out("wtf_end_array(ctx);\n");
	}
	
	if(strcmp(node->type_name, "AssetReferenceAttribute") == 0) {
		indent(ind); out("std::string reference_%d = asset_reference_to_string(%s);\n", depth, expr);
		indent(ind); out("wtf_write_string(ctx, reference_%d.c_str());\n", depth);
	}
	
	if(strcmp(node->type_name, "FileReferenceAttribute") == 0) {
		indent(ind); out("std::string path_%d = %s.path.string();\n", depth, expr);
		indent(ind); out("wtf_write_string(ctx, path_%d.c_str());\n", depth);
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
		out("\t");
	}
}

static void out(const char* format, ...) {
	va_list list;
	va_start(list, format);
	vfprintf(out_file, format, list);
	va_end(list);
}
