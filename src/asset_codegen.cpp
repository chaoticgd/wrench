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

static void generate_asset_type(const WtfNode* asset_type, s32 id);
static void generate_asset_type_function(const WtfNode* root);
static void generate_create_asset_function(const WtfNode* root);
static void generate_asset_string_to_type_function(const WtfNode* root);
static void generate_asset_type_to_string_function(const WtfNode* root);
static void generate_read_function(const WtfNode* asset_type);
static void generate_read_attribute_code(const WtfNode* node, const char* result, const char* attrib, s32 depth);
static void generate_attribute_type_check_code(const std::string& attribute, const char* expected, s32 ind);
static void generate_write_function(const WtfNode* asset_type);
static void generate_asset_write_code(const WtfNode* node, const char* expr, s32 depth);
static void generate_attribute_getter_and_setter_functions(const WtfNode* asset_type);
static void generate_attribute_getter_code(const WtfNode* attribute, s32 depth);
static void generate_attribute_setter_code(const WtfNode* attribute, s32 depth);
static void generate_child_functions(const WtfNode* asset_type);
static void generate_setter_functions(const WtfNode* asset_type);
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
		out("class %sAsset;\n", node->tag);
	}
	out("std::unique_ptr<Asset> create_asset(AssetType type, AssetForest& forest, AssetBank& pack, AssetFile& file, Asset* parent, std::string tag);\n");
	out("AssetType asset_string_to_type(const char* type_name);\n");
	out("const char* asset_type_to_string(AssetType type);\n");
	s32 id = 0;
	for(const WtfNode* node = wtf_first_child(root, "AssetType"); node != NULL; node = wtf_next_sibling(node, "AssetType")) {
		generate_asset_type(node, id++);
	}
	out("#endif\n\n");
	
	out("// *****************************************************************************\n");
	out("// Implementation\n");
	out("// *****************************************************************************\n\n");
	
	out("#ifdef GENERATED_ASSET_IMPLEMENTATION\n");
	generate_create_asset_function(root);
	generate_asset_string_to_type_function(root);
	generate_asset_type_to_string_function(root);
	const WtfNode* first_asset_type = wtf_first_child(root, "AssetType");
	for(const WtfNode* node = first_asset_type; node != NULL; node = wtf_next_sibling(node, "AssetType")) {
		if(node != first_asset_type) {
			out("\n");
			out("// *****************************************************************************\n");
		}
		
		out("\n");
		out("%sAsset::%sAsset(AssetForest& forest, AssetBank& pack, AssetFile& file, Asset* parent, std::string tag)\n", node->tag, node->tag);
		out("\t: Asset(forest, pack, file, parent, ASSET_TYPE, std::move(tag), funcs) {\n");
		
		const WtfAttribute* wad = wtf_attribute(node, "wad");
		if(wad && wad->type == WTF_BOOLEAN && wad->boolean) {
			out("\tis_wad = true;\n");
		}
		
		const WtfAttribute* level_wad = wtf_attribute(node, "level_wad");
		if(level_wad && level_wad->type == WTF_BOOLEAN && level_wad->boolean) {
			out("\tis_level_wad = true;\n");
		}
		
		const WtfAttribute* bin_leaf = wtf_attribute(node, "bin_leaf");
		if(bin_leaf && bin_leaf->type == WTF_BOOLEAN && bin_leaf->boolean) {
			out("\tis_bin_leaf = true;\n");
		}
		
		out("}\n\n");
		
		generate_read_function(node);
		generate_write_function(node);
		out("%sAsset& %sAsset::switch_files(const fs::path& name) {\n", node->tag, node->tag);
		out("\tif(name.empty()) {\n");
		out("\t\treturn static_cast<%sAsset&>(asset_file(fs::path(tag())/tag()));\n", node->tag);
		out("\t} else {\n");
		out("\t\treturn static_cast<%sAsset&>(asset_file(name));\n", node->tag);
		out("\t}\n");
		out("}\n");
		out("\n");
		out("AssetDispatchTable %sAsset::funcs;\n", node->tag);
		generate_attribute_getter_and_setter_functions(node);
		generate_child_functions(node);
	}
	out("#endif\n");
	
	wtf_free(root);
}

static void generate_asset_type(const WtfNode* asset_type, s32 id) {
	out("class %sAsset : public Asset {\n", asset_type->tag);
	for(WtfNode* node = asset_type->first_child; node != NULL; node = node->next_sibling) {
		std::string cpp_type = node_to_cpp_type(node);
		if(!cpp_type.empty()) {
			out("\tOpt<%s> _attribute_%s;\n", cpp_type.c_str(), node->tag);
		}
	}
	out("public:\n");
	out("\t%sAsset(AssetForest& forest, AssetBank& pack, AssetFile& file, Asset* parent, std::string tag);\n", asset_type->tag);
	out("\t\n");
	out("\tvoid for_each_attribute(AssetVisitorCallback callback) override {}\n");
	out("\tvoid for_each_attribute(ConstAssetVisitorCallback callback) const override {}\n");
	out("\tvoid read_attributes(const WtfNode* node) override;\n");
	out("\tvoid write_attributes(WtfWriter* ctx) const override;\n");
	out("\tvoid validate_attributes() const override {}\n");
	out("\t%sAsset& switch_files(const fs::path& name = \"\");\n", asset_type->tag);
	out("\t\n");
	out("\tstatic AssetDispatchTable funcs;\n");
	bool first = true;
	for(WtfNode* node = asset_type->first_child; node != NULL; node = node->next_sibling) {
		std::string getter_name = node->tag;
		assert(getter_name.size() >= 1);
		if(getter_name[0] >= '0' && getter_name[0] <= '9') {
			getter_name = '_' + getter_name;
		}
		
		std::string cpp_type = node_to_cpp_type(node);
		if(!cpp_type.empty()) {
			if(first) {
				out("\t\n");
				first = false;
			}
			out("\t\n");
			out("\tbool has_%s();\n", getter_name.c_str());
			out("\t%s %s();\n", cpp_type.c_str(), getter_name.c_str());
			out("\t%s %s(%s& def);\n", cpp_type.c_str(), getter_name.c_str(), cpp_type.c_str());
			out("\tvoid set_%s(%s src_0);\n", node->tag, cpp_type.c_str());
		}
		
		if(strcmp(node->type_name, "Child") == 0) {
			if(first) {
				out("\t\n");
				first = false;
			}
			
			const WtfAttribute* allowed_types = wtf_attribute(node, "allowed_types");
			assert(allowed_types);
			assert(allowed_types->type == WTF_ARRAY);
			assert(allowed_types->first_array_element);
			assert(allowed_types->first_array_element->type == WTF_STRING);
			if(allowed_types->first_array_element->next) {
				out("\ttemplate <typename ChildType>\n");
				out("\tChildType& %s() { return child<ChildType>(\"%s\"); }\n", getter_name.c_str(), node->tag);
				out("\tbool has_%s();\n", node->tag);
				out("\tAsset& get_%s();\n", node->tag);
			} else {
				const char* child_type = allowed_types->first_array_element->string;
				out("%sAsset& %s();\n", child_type, getter_name.c_str());
				out("\tbool has_%s();\n", node->tag);
				out("\t%sAsset& get_%s();\n", child_type, node->tag);
			}
		}
	}
	out("\t\n");
	out("\tstatic const constexpr AssetType ASSET_TYPE = AssetType{%d};\n", id);
	if(wtf_attribute)
	out("};\n\n");
}

static void generate_create_asset_function(const WtfNode* root) {
	out("std::unique_ptr<Asset> create_asset(AssetType type, AssetForest& forest, AssetBank& pack, AssetFile& file, Asset* parent, std::string tag) {\n");
	s32 id = 0;
	for(const WtfNode* node = wtf_first_child(root, "AssetType"); node != NULL; node = wtf_next_sibling(node, "AssetType")) {
		out("\tif(type.id == %d) return std::make_unique<%sAsset>(forest, pack, file, parent, std::move(tag));\n", id++, node->tag);
	}
	out("\treturn nullptr;\n");
	out("}\n\n");
}

static void generate_asset_string_to_type_function(const WtfNode* root) {
	out("AssetType asset_string_to_type(const char* type_name) {\n");
	s32 id = 0;
	for(const WtfNode* node = wtf_first_child(root, "AssetType"); node != NULL; node = wtf_next_sibling(node, "AssetType")) {
		out("\tif(strcmp(type_name, \"%s\") == 0) return AssetType{%d};\n", node->tag, id++);
	}
	out("\treturn NULL_ASSET_TYPE;\n");
	out("}\n\n");
}

static void generate_asset_type_to_string_function(const WtfNode* root) {
	out("const char* asset_type_to_string(AssetType type) {\n");
	s32 id = 0;
	for(const WtfNode* node = wtf_first_child(root, "AssetType"); node != NULL; node = wtf_next_sibling(node, "AssetType")) {
		out("\tif(type.id == %d) return \"%s\";\n", id++, node->tag);
	}
	out("\treturn nullptr;\n");
	out("}\n\n");
}

static void generate_read_function(const WtfNode* asset_type) {
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
			std::string result = std::string("(*_attribute_") + node->tag + ")";
			std::string attrib = std::string("attribute_") + node->tag;
			out("\tconst WtfAttribute* %s = wtf_attribute(node, \"%s\");\n", attrib.c_str(), node->tag);
			out("\tif(%s) {\n", attrib.c_str());
			out("\t_attribute_%s.emplace();\n", node->tag);
			generate_read_attribute_code(node, result.c_str(), attrib.c_str(), 0);
			out("\t}\n");
		}
	}
	out("}\n\n");
}

static void generate_read_attribute_code(const WtfNode* node, const char* result, const char* attrib, s32 depth) {
	s32 ind = depth + 2;
	
	if(strcmp(node->type_name, "IntegerAttribute") == 0) {
		generate_attribute_type_check_code(attrib, "WTF_NUMBER", ind);
		indent(ind); out("%s = %s->number.i;\n", result, attrib);
	}
	
	if(strcmp(node->type_name, "BooleanAttribute") == 0) {
		generate_attribute_type_check_code(attrib, "WTF_BOOLEAN", ind);
		indent(ind); out("%s = %s->boolean;\n", result, attrib);
	}
	
	if(strcmp(node->type_name, "StringAttribute") == 0) {
		generate_attribute_type_check_code(attrib, "WTF_STRING", ind);
		indent(ind); out("%s = %s->string;\n", result, attrib);
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
	
	if(strcmp(node->type_name, "FileReferenceAttribute") == 0) {
		generate_attribute_type_check_code(attrib, "WTF_STRING", ind);
		indent(ind); out("%s = FileReference(file(), %s->string);\n", result, attrib);
	}
}

static void generate_attribute_type_check_code(const std::string& attribute, const char* expected, s32 ind) {
	indent(ind); out("if(%s->type != %s) {\n", attribute.c_str(), expected);
	indent(ind); out("\tthrow InvalidAssetAttributeType(node, %s);\n", attribute.c_str());
	indent(ind); out("}\n");
}

static void generate_write_function(const WtfNode* asset_type) {
	out("void %sAsset::write_attributes(WtfWriter* ctx) const {\n", asset_type->tag);
	for(WtfNode* node = asset_type->first_child; node != NULL; node = node->next_sibling) {
		std::string cpp_type = node_to_cpp_type(node);
		if(!cpp_type.empty()) {
			out("\tif(_attribute_%s.has_value()) {\n", node->tag);
			out("\t\twtf_begin_attribute(ctx, \"%s\");\n", node->tag);
			std::string expr = std::string("(*_attribute_") + node->tag + ")";
			generate_asset_write_code(node, expr.c_str(), 0);
			out("\t\twtf_end_attribute(ctx);\n");
			out("\t}\n");
		}
	}
	out("}\n\n");
}

static void generate_asset_write_code(const WtfNode* node, const char* expr, s32 depth) {
	s32 ind = depth + 2;
	
	if(strcmp(node->type_name, "IntegerAttribute") == 0) {
		indent(ind); out("wtf_write_integer(ctx, %s);\n", expr);
	}
	
	if(strcmp(node->type_name, "BooleanAttribute") == 0) {
		indent(ind); out("wtf_write_boolean(ctx, %s);\n", expr);
	}
	
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
	
	if(strcmp(node->type_name, "FileReferenceAttribute") == 0) {
		indent(ind); out("std::string path_%d = %s.path.string();\n", depth, expr);
		indent(ind); out("wtf_write_string(ctx, path_%d.c_str());\n", depth);
	}
}

static void generate_attribute_getter_and_setter_functions(const WtfNode* asset_type) {
	for(WtfNode* node = asset_type->first_child; node != NULL; node = node->next_sibling) {
		std::string cpp_type = node_to_cpp_type(node);
		if(!cpp_type.empty()) {
			std::string getter_name = node->tag;
			assert(getter_name.size() >= 1);
			if(getter_name[0] >= '0' && getter_name[0] <= '9') {
				getter_name = '_' + getter_name;
			}
			
			out("bool %sAsset::has_%s() {\n", asset_type->tag, getter_name.c_str());
			out("\tfor(Asset* asset = this; asset != nullptr; asset = asset->lower_precedence()) {\n");
			out("\t\tif(asset->type() == type() && _attribute_%s.has_value()) {\n", node->tag);
			out("\t\t\treturn true;\n");
			out("\t\t}\n");
			out("\t}\n");
			out("\treturn false;\n");
			out("}\n");
			out("\n");
			
			// TODO: Fix precedence behaviour.
			for(s32 getter_type = 0; getter_type < 2; getter_type++) {
				if(getter_type == 0) {
					out("%s %sAsset::%s() {\n", cpp_type.c_str(), asset_type->tag, getter_name.c_str());
				} else {
					out("%s %sAsset::%s(%s& def) {\n", cpp_type.c_str(), asset_type->tag, getter_name.c_str(), cpp_type.c_str());
				}
				out("\t%s dest_0;\n", cpp_type.c_str());
				out("\tif(!_attribute_%s.has_value()) {\n", node->tag);
				out("\t\tif(lower_precedence()) {\n");
				if(getter_type == 0) {
					out("\t\t\tdest_0 = static_cast<%sAsset*>(lower_precedence())->%s();\n", asset_type->tag, getter_name.c_str());
				} else {
					out("\t\t\tdest_0 = static_cast<%sAsset*>(lower_precedence())->%s(def);\n", asset_type->tag, getter_name.c_str());
				}
				out("\t\t} else {\n");
				if(getter_type == 0) {
					out("\t\tthrow MissingAssetAttribute();\n");
				} else {
					out("\t\treturn def;\n");
				}
				out("\t\t}\n");
				out("\t} else {\n");
				out("\t\tconst %s& src_0 = *_attribute_%s;\n\n", cpp_type.c_str(), node->tag);
				generate_attribute_getter_code(node, 0);
				out("\t}\n");
				out("\treturn dest_0;\n");
				out("}\n");
				out("\n");
			}
			
			out("void %sAsset::set_%s(%s src_0) {\n", asset_type->tag, node->tag, cpp_type.c_str());
			out("\t%s dest_0;\n", cpp_type.c_str());
			generate_attribute_setter_code(node, 0);
			out("\t_attribute_%s = std::move(dest_0);\n", node->tag);
			out("}\n");
			out("\n");
		}
	}
}

static void generate_attribute_getter_code(const WtfNode* attribute, s32 depth) {
	s32 ind = depth + 2;
	
	if(strcmp(attribute->type_name, "IntegerAttribute") == 0) {
		indent(ind); out("dest_%d = src_%d;\n", depth, depth);
	}
	
	if(strcmp(attribute->type_name, "BooleanAttribute") == 0) {
		indent(ind); out("dest_%d = src_%d;\n", depth, depth);
	}
	
	if(strcmp(attribute->type_name, "StringAttribute") == 0) {
		indent(ind); out("dest_%d = src_%d;\n", depth, depth);
	}
	
	if(strcmp(attribute->type_name, "ArrayAttribute") == 0) {
		indent(ind); out("for(const auto& src_%d : src_%d) {\n", depth + 1, depth);
		indent(ind); out("\tdecltype(dest_%d)::value_type dest_%d;\n", depth, depth + 1);
		generate_attribute_getter_code(wtf_child(attribute, NULL, "element"), depth + 1);
		indent(ind); out("\tdest_%d.emplace_back(std::move(dest_%d));\n", depth, depth + 1);
		indent(ind); out("}\n");
		
	}
	
	if(strcmp(attribute->type_name, "FileReferenceAttribute") == 0) {
		indent(ind); out("dest_%d = src_%d;\n", depth, depth);
	}
}

static void generate_attribute_setter_code(const WtfNode* attribute, s32 depth) {
	s32 ind = depth + 1;
	
	if(strcmp(attribute->type_name, "IntegerAttribute") == 0) {
		indent(ind); out("dest_%d = src_%d;\n", depth, depth);
	}
	
	if(strcmp(attribute->type_name, "BooleanAttribute") == 0) {
		indent(ind); out("dest_%d = src_%d;\n", depth, depth);
	}
	
	if(strcmp(attribute->type_name, "StringAttribute") == 0) {
		indent(ind); out("dest_%d = src_%d;\n", depth, depth);
	}
	
	if(strcmp(attribute->type_name, "ArrayAttribute") == 0) {
		indent(ind); out("for(const auto& src_%d : src_%d) {\n", depth + 1, depth);
		indent(ind); out("\tdecltype(dest_%d)::value_type dest_%d;\n", depth, depth + 1);
		generate_attribute_setter_code(wtf_child(attribute, NULL, "element"), depth + 1);
		indent(ind); out("\tdest_%d.emplace_back(std::move(dest_%d));\n", depth, depth + 1);
		indent(ind); out("}\n");
	}
	
	if(strcmp(attribute->type_name, "FileReferenceAttribute") == 0) {
		indent(ind); out("dest_%d = src_%d;\n", depth, depth);
	}
}

static void generate_child_functions(const WtfNode* asset_type) {
	for(WtfNode* node = asset_type->first_child; node != NULL; node = node->next_sibling) {
		if(strcmp(node->type_name, "Child") == 0) {
			std::string getter_name = node->tag;
			assert(getter_name.size() >= 1);
			if(getter_name[0] >= '0' && getter_name[0] <= '9') {
				getter_name = '_' + getter_name;
			}
			
			const WtfAttribute* allowed_types = wtf_attribute(node, "allowed_types");
			assert(allowed_types);
			assert(allowed_types->type == WTF_ARRAY);
			assert(allowed_types->first_array_element);
			assert(allowed_types->first_array_element->type == WTF_STRING);
			const char* child_type = allowed_types->first_array_element->string;
			
			if(!allowed_types->first_array_element->next) {
				out("%sAsset& %sAsset::%s() {\n", child_type, asset_type->tag, getter_name.c_str());
				out("\treturn child<%sAsset>(\"%s\");\n", child_type, node->tag);
				out("}\n");
				out("\n");
			}
			
			out("bool %sAsset::has_%s() {\n", asset_type->tag, node->tag);
			out("\treturn has_child(\"%s\");\n", node->tag);
			out("}\n");
			out("\n");
			
			if(allowed_types->first_array_element->next) {
				out("Asset& %sAsset::get_%s() {\n", asset_type->tag, node->tag);
				out("\treturn get_child(\"%s\");\n", node->tag);
			} else {
				out("%sAsset& %sAsset::get_%s() {\n", child_type, asset_type->tag, node->tag);
				out("\treturn get_child(\"%s\").as<%sAsset>();\n", node->tag, child_type);
			}
			out("}\n");
			out("\n");
		}
	}
}

static std::string node_to_cpp_type(const WtfNode* node) {
	if(strcmp(node->type_name, "IntegerAttribute") == 0) {
		return "s32";
	}
	
	if(strcmp(node->type_name, "BooleanAttribute") == 0) {
		return "bool";
	}
	
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
	
	if(strcmp(node->type_name, "FileReferenceAttribute") == 0) {
		return "FileReference";
	}
	
	return "";
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
