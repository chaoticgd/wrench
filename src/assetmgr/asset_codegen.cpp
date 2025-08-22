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

#include <string>
#include <vector>
#undef NDEBUG
#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include <platform/fileio.h>

#include <wtf/wtf.h>

#define INTEGER_ATTRIB        "IntegerAttribute"
#define FLOAT_ATTRIB          "FloatAttribute"
#define BOOLEAN_ATTRIB        "BooleanAttribute"
#define STRING_ATTRIB         "StringAttribute"
#define ARRAY_ATTRIB          "ArrayAttribute"
#define VECTOR3_ATTRIB        "Vector3Attribute"
#define COLOUR_ATTRIB         "ColourAttribute"
#define ASSET_LINK_ATTRIB     "AssetLinkAttribute"
#define FILE_REFERENCE_ATTRIB "FileReferenceAttribute"

static void generate_asset_type(const WtfNode* asset_type, int id);
static void generate_create_asset_function(const WtfNode* root);
static void generate_asset_string_to_type_function(const WtfNode* root);
static void generate_asset_type_to_string_function(const WtfNode* root);
static void generate_dispatch_table_from_asset_type_function(const WtfNode* root);
static void generate_asset_implementation(const WtfNode* asset_type);
static void generate_read_function(const WtfNode* asset_type);
static void generate_read_attribute_code(const WtfNode* node, const char* result, const char* attrib, int depth);
static void generate_attribute_type_check_code(const std::string& attribute, const char* expected, const char* name, int ind);
static void generate_write_function(const WtfNode* asset_type);
static void generate_asset_write_code(const WtfNode* node, const char* expr, int depth);
static void generate_attribute_getter_and_setter_functions(const WtfNode* asset_type);
static void generate_attribute_getter_code(const WtfNode* attribute, int depth);
static void generate_attribute_setter_code(const WtfNode* attribute, int depth);
static void generate_child_functions(const WtfNode* asset_type);
static std::string node_to_cpp_type(const WtfNode* node);
static void indent(int ind);
static void out(const char* format, ...);

static FILE* out_file = NULL;
static WrenchFileHandle* out_handle = NULL;

int main(int argc, char** argv)
{
	assert(argc == 2 || argc == 3);
	WrenchFileHandle* file = file_open(argv[1], WRENCH_FILE_MODE_READ);
	if (!file) {
		fprintf(stderr, "Failed to open input file.\n");
		return 1;
	}
	std::vector<char> bytes;
	char c;
	while (file_read(&c, 1, file) == 1) {
		bytes.emplace_back(c);
	}
	bytes.push_back(0);

	file_close(file);

	if (argc == 3) {
		out_handle = file_open(argv[2], WRENCH_FILE_MODE_WRITE);
	} else {
		out_file = stdout;
	}
	
	char* error = NULL;
	WtfNode* root = wtf_parse((char*) bytes.data(), &error);
	if (error) {
		fprintf(stderr, "Failed to parse asset schema. %s\n", error);
		return 1;
	}
	
	const WtfAttribute* format_version = wtf_attribute(root, "format_version");
	assert(format_version && format_version->type == WTF_NUMBER);
	
	out("// Generated from %s. Do not edit.\n\n", argv[1]);
	
	out("// *****************************************************************************\n");
	out("// Header\n");
	out("// *****************************************************************************\n\n");
	
	out("#ifdef GENERATED_ASSET_HEADER\n\n");
	out("#define ASSET_FORMAT_VERSION %d\n\n", format_version->number.i);
	for (const WtfNode* node = wtf_first_child(root, "AssetType"); node != NULL; node = wtf_next_sibling(node, "AssetType")) {
		out("class %sAsset;\n", node->tag);
	}
	out("std::unique_ptr<Asset> create_asset(AssetType type, AssetFile& file, Asset* parent, std::string tag);\n");
	out("AssetType asset_string_to_type(const char* type_name);\n");
	out("const char* asset_type_to_string(AssetType type);\n");
	out("AssetDispatchTable* dispatch_table_from_asset_type(AssetType type);\n");
	int id = 0;
	for (const WtfNode* node = wtf_first_child(root, "AssetType"); node != NULL; node = wtf_next_sibling(node, "AssetType")) {
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
	generate_dispatch_table_from_asset_type_function(root);
	const WtfNode* first_asset_type = wtf_first_child(root, "AssetType");
	for (const WtfNode* node = first_asset_type; node != NULL; node = wtf_next_sibling(node, "AssetType")) {
		if (node != first_asset_type) {
			out("\n");
			out("// *****************************************************************************\n");
		}
		
		generate_asset_implementation(node);
	}
	out("#endif\n");
	
	wtf_free(root);

	if (out_handle != NULL) {
		file_close(out_handle);
	}
}

static void generate_asset_type(const WtfNode* asset_type, int id)
{
	out("class %sAsset : public Asset {\n", asset_type->tag);
	
	int attribute_count = 0;
	for (WtfNode* node = asset_type->first_child; node != NULL; node = node->next_sibling) {
		std::string cpp_type = node_to_cpp_type(node);
		if (!cpp_type.empty()) {
			if (attribute_count == 0) {
				out("\tenum {\n");
			}
			out("\t\tATTRIB_%s = (1 << %d),\n", node->tag, attribute_count);
			attribute_count++;
		}
	}
	if (attribute_count > 0) {
		out("\t};\n");
	}
	
	// We use a bitfield to store whether each attribute exists instead of
	// something like std::optional to save memory. If you need this to be
	// higher turn the m_attrib_exists in the Asset class into a u64.
	assert(attribute_count <= 32);
	
	for (WtfNode* node = asset_type->first_child; node != NULL; node = node->next_sibling) {
		std::string cpp_type = node_to_cpp_type(node);
		if (!cpp_type.empty()) {
			out("\t%s m_attribute_%s;\n", cpp_type.c_str(), node->tag);
			attribute_count++;
		}
	}
	out("\n");
	out("public:\n");
	out("\t%sAsset(AssetFile& file, Asset* parent, std::string tag);\n", asset_type->tag);
	out("\t\n");
	out("\tvoid for_each_attribute(AssetVisitorCallback callback) override {}\n");
	out("\tvoid for_each_attribute(ConstAssetVisitorCallback callback) const override {}\n");
	out("\tvoid read_attributes(const WtfNode* node) override;\n");
	out("\tvoid write_attributes(WtfWriter* ctx) const override;\n");
	out("\tvoid validate_attributes() const override {}\n");
	out("\t\n");
	out("\tstatic AssetDispatchTable funcs;\n");
	bool first = true;
	for (WtfNode* node = asset_type->first_child; node != NULL; node = node->next_sibling) {
		std::string getter_name = node->tag;
		assert(getter_name.size() >= 1);
		if (getter_name[0] >= '0' && getter_name[0] <= '9') {
			getter_name = '_' + getter_name;
		}
		
		std::string cpp_type = node_to_cpp_type(node);
		if (!cpp_type.empty()) {
			if (first) {
				out("\t\n");
				first = false;
			}
			out("\t\n");
			out("\tbool has_%s() const;\n", getter_name.c_str());
			out("\t%s %s() const;\n", cpp_type.c_str(), getter_name.c_str());
			out("\t%s %s(%s def) const;\n", cpp_type.c_str(), getter_name.c_str(), cpp_type.c_str());
			out("\tvoid set_%s(%s src_0);\n", node->tag, cpp_type.c_str());
		}
		
		if (strcmp(node->type_name, "Child") == 0) {
			if (first) {
				out("\t\n");
				first = false;
			}
			
			const WtfAttribute* allowed_types = wtf_attribute(node, "allowed_types");
			assert(allowed_types);
			assert(allowed_types->type == WTF_ARRAY);
			assert(allowed_types->first_array_element);
			assert(allowed_types->first_array_element->type == WTF_STRING);
			
			const char* child_type  = allowed_types->first_array_element->string.begin;
			
			if (allowed_types->first_array_element->next) {
				out("\ttemplate <typename ChildType>\n");
			} else {
				out("\ttemplate <typename ChildType = %sAsset>\n", child_type);
			}
			out("\tChildType& %s(AssetAccessorMode mode = DO_NOT_SWITCH_FILES)\n", getter_name.c_str());
			out("\t{\n");
			out("\t\tif (mode == SWITCH_FILES) {\n");
			out("\t\t\treturn foreign_child<ChildType>(\"%s/%s\", false, \"%s\");\n", node->tag, node->tag, node->tag);
			out("\t\t} else {\n");
			out("\t\t\treturn child<ChildType>(\"%s\");\n", node->tag);
			out("\t\t}\n");
			out("\t}\n");
			if (allowed_types->first_array_element->next) {
				out("\ttemplate <typename ChildType>\n");
			} else {
				out("\ttemplate <typename ChildType = %sAsset>\n", child_type);
			}
			out("\tChildType& %s(std::string path) { return foreign_child<ChildType>(path, false, \"%s\"); }\n", getter_name.c_str(), node->tag);
			out("\tbool has_%s() const;\n", node->tag);
			if (allowed_types->first_array_element->next) {
				out("\tAsset& get_%s();\n", node->tag);
				out("\tconst Asset& get_%s() const;\n", node->tag);
			} else {
				out("\t%sAsset& get_%s();\n", child_type, node->tag);
				out("\tconst %sAsset& get_%s() const;\n", child_type, node->tag);
			}
		}
	}
	out("\t\n");
	out("\tstatic const constexpr AssetType ASSET_TYPE = AssetType{%d};\n", id);
	
	out("};\n\n");
}

static void generate_create_asset_function(const WtfNode* root)
{
	out("std::unique_ptr<Asset> create_asset(AssetType type, AssetFile& file, Asset* parent, std::string tag) {\n");
	out("\tswitch (type.id) {\n");
	for (const WtfNode* node = wtf_first_child(root, "AssetType"); node != NULL; node = wtf_next_sibling(node, "AssetType")) {
		out("\t\tcase %sAsset::ASSET_TYPE.id: return std::make_unique<%sAsset>(file, parent, std::move(tag));\n", node->tag, node->tag);
	}
	out("\t}\n");
	out("\treturn nullptr;\n");
	out("}\n\n");
}

static void generate_asset_string_to_type_function(const WtfNode* root)
{
	out("AssetType asset_string_to_type(const char* type_name)\n");
	out("{\n");
	int id = 0;
	for (const WtfNode* node = wtf_first_child(root, "AssetType"); node != NULL; node = wtf_next_sibling(node, "AssetType")) {
		out("\tif (strcmp(type_name, \"%s\") == 0) return AssetType{%d};\n", node->tag, id++);
	}
	out("\treturn NULL_ASSET_TYPE;\n");
	out("}\n\n");
}

static void generate_asset_type_to_string_function(const WtfNode* root)
{
	out("const char* asset_type_to_string(AssetType type)\n");
	out("{\n");
	out("\tswitch (type.id) {\n");
	for (const WtfNode* node = wtf_first_child(root, "AssetType"); node != NULL; node = wtf_next_sibling(node, "AssetType")) {
		out("\t\tcase %sAsset::ASSET_TYPE.id: return \"%s\";\n", node->tag, node->tag);
	}
	out("\t}\n");
	out("\treturn nullptr;\n");
	out("}\n\n");
}

static void generate_dispatch_table_from_asset_type_function(const WtfNode* root)
{
	out("AssetDispatchTable* dispatch_table_from_asset_type(AssetType type)\n");
	out("{\n");
	out("\tswitch (type.id) {\n");
	for (const WtfNode* node = wtf_first_child(root, "AssetType"); node != NULL; node = wtf_next_sibling(node, "AssetType")) {
		out("\t\tcase %sAsset::ASSET_TYPE.id: return &%sAsset::funcs;\n", node->tag, node->tag);
	}
	out("\t}\n");
	out("\treturn nullptr;\n");
	out("}\n\n");
}

static void generate_asset_implementation(const WtfNode* asset_type)
{
	out("\n");
	out("%sAsset::%sAsset(AssetFile& file, Asset* parent, std::string tag)\n", asset_type->tag, asset_type->tag);
	out("\t: Asset(file, parent, ASSET_TYPE, std::move(tag), funcs)\n");
	out("{\n");
	
	const WtfAttribute* wad = wtf_attribute(asset_type, "wad");
	if (wad && wad->type == WTF_BOOLEAN && wad->boolean) {
		out("\tflags |= ASSET_IS_WAD;\n");
	}
	
	const WtfAttribute* level_wad = wtf_attribute(asset_type, "level_wad");
	if (level_wad && level_wad->type == WTF_BOOLEAN && level_wad->boolean) {
		out("\tflags |= ASSET_IS_LEVEL_WAD;\n");
	}
	
	const WtfAttribute* bin_leaf = wtf_attribute(asset_type, "bin_leaf");
	if (bin_leaf && bin_leaf->type == WTF_BOOLEAN && bin_leaf->boolean) {
		out("\tflags |= ASSET_IS_BIN_LEAF;\n");
	}
	
	const WtfAttribute* flattenable = wtf_attribute(asset_type, "flattenable");
	if (flattenable && flattenable->type == WTF_BOOLEAN && flattenable->boolean) {
		out("\tflags |= ASSET_IS_FLATTENABLE;\n");
	}
	
	out("}\n\n");
	
	generate_read_function(asset_type);
	generate_write_function(asset_type);
	out("AssetDispatchTable %sAsset::funcs;\n", asset_type->tag);
	generate_attribute_getter_and_setter_functions(asset_type);
	generate_child_functions(asset_type);
}

static void generate_read_function(const WtfNode* asset_type)
{
	bool first = true;
	out("void %sAsset::read_attributes(const WtfNode* node)\n", asset_type->tag);
	out("{\n");
	for (WtfNode* node = asset_type->first_child; node != NULL; node = node->next_sibling) {
		std::string type = node_to_cpp_type(node);
		if (!type.empty()) {
			if (!first) {
				out("\t\n");
			} else {
				first = false;
			}
			std::string result = std::string("m_attribute_") + node->tag;
			std::string attrib = std::string("attribute_") + node->tag;
			out("\tconst WtfAttribute* %s = wtf_attribute(node, \"%s\");\n", attrib.c_str(), node->tag);
			out("\tif (%s) {\n", attrib.c_str());
			generate_read_attribute_code(node, result.c_str(), attrib.c_str(), 0);
			out("\t\tm_attrib_exists |= ATTRIB_%s;\n", node->tag);
			out("\t}\n");
		}
	}
	out("}\n\n");
}

static void generate_read_attribute_code(const WtfNode* node, const char* result, const char* attrib, int depth)
{
	int ind = depth + 2;
	
	if (strcmp(node->type_name, INTEGER_ATTRIB) == 0) {
		generate_attribute_type_check_code(attrib, "WTF_NUMBER", node->tag, ind);
		indent(ind); out("%s = %s->number.i;\n", result, attrib);
	}
	
	if (strcmp(node->type_name, FLOAT_ATTRIB) == 0) {
		generate_attribute_type_check_code(attrib, "WTF_NUMBER", node->tag, ind);
		indent(ind); out("%s = %s->number.f;\n", result, attrib);
	}
	
	if (strcmp(node->type_name, BOOLEAN_ATTRIB) == 0) {
		generate_attribute_type_check_code(attrib, "WTF_BOOLEAN", node->tag, ind);
		indent(ind); out("%s = %s->boolean;\n", result, attrib);
	}
	
	if (strcmp(node->type_name, STRING_ATTRIB) == 0) {
		generate_attribute_type_check_code(attrib, "WTF_STRING", node->tag, ind);
		indent(ind); out("%s = std::string(%s->string.begin, (size_t) (%s->string.end - %s->string.begin));\n", result, attrib, attrib, attrib);
	}
	
	if (strcmp(node->type_name, ARRAY_ATTRIB) == 0) {
		generate_attribute_type_check_code(attrib, "WTF_ARRAY", node->tag, ind);
		const WtfNode* element = wtf_child(node, NULL, "element");
		assert(element);
		std::string element_type = node_to_cpp_type(element);
		std::string element_result = std::string("temp_") + std::to_string(depth);
		std::string element_attrib = "element_" + std::to_string(depth);
		indent(ind); out("for (const WtfAttribute* %s = %s->first_array_element; %s != NULL; %s = %s->next) {\n",
			element_attrib.c_str(), attrib, element_attrib.c_str(), element_attrib.c_str(), element_attrib.c_str());
		indent(ind); out("\t%s %s;\n", element_type.c_str(), element_result.c_str());
		generate_read_attribute_code(element, element_result.c_str(), element_attrib.c_str(), depth + 1);
		indent(ind); out("\t%s.emplace_back(std::move(%s));\n", result, element_result.c_str());
		indent(ind); out("}\n");
	}
	
	if (strcmp(node->type_name, VECTOR3_ATTRIB) == 0) {
		generate_attribute_type_check_code(attrib, "WTF_ARRAY", node->tag, ind);
		indent(ind); out("WtfAttribute* x_elem = %s->first_array_element;\n", attrib);
		indent(ind); out("WtfAttribute* y_elem = x_elem ? x_elem->next : nullptr;\n");
		indent(ind); out("WtfAttribute* z_elem = y_elem ? y_elem->next : nullptr;\n");
		indent(ind); out("verify(z_elem, \"A Vector3 attribute does not have 3 elements.\");\n");
		indent(ind); out("verify(x_elem->type == WTF_NUMBER, \"A Vector3 attribute has elements that aren't numbers.\");\n");
		indent(ind); out("verify(y_elem->type == WTF_NUMBER, \"A Vector3 attribute has elements that aren't numbers.\");\n");
		indent(ind); out("verify(z_elem->type == WTF_NUMBER, \"A Vector3 attribute has elements that aren't numbers.\");\n");
		indent(ind); out("%s = glm::vec3(x_elem->number.f, y_elem->number.f, z_elem->number.f);\n", result);
	}
	
	if (strcmp(node->type_name, COLOUR_ATTRIB) == 0) {
		generate_attribute_type_check_code(attrib, "WTF_ARRAY", node->tag, ind);
		indent(ind); out("WtfAttribute* r_elem = %s->first_array_element;\n", attrib);
		indent(ind); out("WtfAttribute* g_elem = r_elem ? r_elem->next : nullptr;\n");
		indent(ind); out("WtfAttribute* b_elem = g_elem ? g_elem->next : nullptr;\n");
		indent(ind); out("WtfAttribute* a_elem = b_elem ? b_elem->next : nullptr;\n");
		indent(ind); out("verify(a_elem, \"A Colour attribute does not have 4 elements.\");\n");
		indent(ind); out("verify(r_elem->type == WTF_NUMBER, \"A Colour attribute has elements that aren't numbers.\");\n");
		indent(ind); out("verify(g_elem->type == WTF_NUMBER, \"A Colour attribute has elements that aren't numbers.\");\n");
		indent(ind); out("verify(b_elem->type == WTF_NUMBER, \"A Colour attribute has elements that aren't numbers.\");\n");
		indent(ind); out("verify(a_elem->type == WTF_NUMBER, \"A Colour attribute has elements that aren't numbers.\");\n");
		indent(ind); out("%s = glm::vec4(r_elem->number.f, g_elem->number.f, b_elem->number.f, a_elem->number.f);\n", result);
	}
	
	if (strcmp(node->type_name, ASSET_LINK_ATTRIB) == 0) {
		generate_attribute_type_check_code(attrib, "WTF_STRING", node->tag, ind);
		indent(ind); out("%s = AssetLink();\n", result);
		indent(ind); out("%s.set(%s->string.begin);\n", result, attrib);
	}
	
	if (strcmp(node->type_name, FILE_REFERENCE_ATTRIB) == 0) {
		generate_attribute_type_check_code(attrib, "WTF_STRING", node->tag, ind);
		indent(ind); out("%s = FileReference(file(), %s->string.begin);\n", result, attrib);
	}
}

static void generate_attribute_type_check_code(const std::string& attribute, const char* expected, const char* name, int ind)
{
	indent(ind); out("verify(%s->type == %s, \"Asset attribute '%s' has invalid type.\");\n",
		attribute.c_str(), expected, name);
}

static void generate_write_function(const WtfNode* asset_type)
{
	out("void %sAsset::write_attributes(WtfWriter* ctx) const\n", asset_type->tag);
	out("{\n");
	for (WtfNode* node = asset_type->first_child; node != NULL; node = node->next_sibling) {
		std::string cpp_type = node_to_cpp_type(node);
		if (!cpp_type.empty()) {
			out("\tif (m_attrib_exists & ATTRIB_%s) {\n", node->tag);
			out("\t\twtf_begin_attribute(ctx, \"%s\");\n", node->tag);
			std::string expr = std::string("m_attribute_") + node->tag;
			generate_asset_write_code(node, expr.c_str(), 0);
			out("\t\twtf_end_attribute(ctx);\n");
			out("\t}\n");
		}
	}
	out("}\n\n");
}

static void generate_asset_write_code(const WtfNode* node, const char* expr, int depth)
{
	int ind = depth + 2;
	
	if (strcmp(node->type_name, INTEGER_ATTRIB) == 0) {
		indent(ind); out("wtf_write_integer(ctx, %s);\n", expr);
	}
	
	if (strcmp(node->type_name, FLOAT_ATTRIB) == 0) {
		indent(ind); out("wtf_write_float(ctx, %s);\n", expr);
	}
	
	if (strcmp(node->type_name, BOOLEAN_ATTRIB) == 0) {
		indent(ind); out("wtf_write_boolean(ctx, %s);\n", expr);
	}
	
	if (strcmp(node->type_name, STRING_ATTRIB) == 0) {
		indent(ind); out("wtf_write_string(ctx, %s.c_str(), %s.c_str() + %s.size());\n", expr, expr, expr);
	}
	
	if (strcmp(node->type_name, ARRAY_ATTRIB) == 0) {
		const WtfNode* element = wtf_child(node, NULL, "element");
		assert(element);
		std::string element_expr = "element_" + std::to_string(depth);
		indent(ind); out("wtf_begin_array(ctx);\n");
		indent(ind); out("for (const auto& %s : %s) {\n", element_expr.c_str(), expr);
		generate_asset_write_code(element, element_expr.c_str(), depth + 1);
		indent(ind); out("}\n");
		indent(ind); out("wtf_end_array(ctx);\n");
	}
	
	if (strcmp(node->type_name, VECTOR3_ATTRIB) == 0) {
		indent(ind); out("float floats[3] = {%s.x, %s.y, %s.z};\n", expr, expr, expr);
		indent(ind); out("wtf_write_floats(ctx, ARRAY_PAIR(floats));\n");
	}
	
	if (strcmp(node->type_name, COLOUR_ATTRIB) == 0) {
		indent(ind); out("float floats[4] = {%s.x, %s.y, %s.z, %s.w};\n", expr, expr, expr, expr);
		indent(ind); out("wtf_write_floats(ctx, ARRAY_PAIR(floats));\n");
	}
	
	if (strcmp(node->type_name, ASSET_LINK_ATTRIB) == 0) {
		indent(ind); out("wtf_write_string(ctx, %s.to_string().c_str());\n", expr);
	}
	
	if (strcmp(node->type_name, FILE_REFERENCE_ATTRIB) == 0) {
		indent(ind); out("std::string path_%d = %s.path.string();\n", depth, expr);
		indent(ind); out("wtf_write_string(ctx, path_%d.c_str());\n", depth);
	}
}

static void generate_attribute_getter_and_setter_functions(const WtfNode* asset_type)
{
	for (WtfNode* node = asset_type->first_child; node != NULL; node = node->next_sibling) {
		std::string cpp_type = node_to_cpp_type(node);
		if (!cpp_type.empty()) {
			std::string getter_name = node->tag;
			assert(getter_name.size() >= 1);
			if (getter_name[0] >= '0' && getter_name[0] <= '9') {
				getter_name = '_' + getter_name;
			}
			
			out("bool %sAsset::has_%s() const\n", asset_type->tag, getter_name.c_str());
			out("{\n");
			out("\tfor (const Asset* asset = this; asset != nullptr; asset = asset->lower_precedence()) {\n");
			out("\t\tif (asset->physical_type() == physical_type() && (static_cast<const %sAsset*>(asset)->m_attrib_exists & ATTRIB_%s)) {\n", asset_type->tag, node->tag);
			out("\t\t\treturn true;\n");
			out("\t\t}\n");
			out("\t}\n");
			out("\treturn false;\n");
			out("}\n");
			out("\n");
			
			for (int getter_type = 0; getter_type < 2; getter_type++) {
				if (getter_type == 0) {
					out("%s %sAsset::%s() const\n", cpp_type.c_str(), asset_type->tag, getter_name.c_str());
					out("{\n");
				} else {
					out("%s %sAsset::%s(%s def) const\n", cpp_type.c_str(), asset_type->tag, getter_name.c_str(), cpp_type.c_str());
					out("{\n");
				}
				out("\tfor (const Asset* asset = &highest_precedence(); asset != nullptr; asset = asset->lower_precedence()) {\n");
				out("\t\tif (asset->physical_type() == ASSET_TYPE) {\n");
				out("\t\t\t%s dest_0;\n", cpp_type.c_str());
				out("\t\t\tconst auto& sub = static_cast<const %sAsset&>(*asset);\n", asset_type->tag);
				out("\t\t\tif (sub.m_attrib_exists & ATTRIB_%s) {\n", node->tag);
				out("\t\t\t\tconst %s& src_0 = sub.m_attribute_%s;\n", cpp_type.c_str(), node->tag);
				generate_attribute_getter_code(node, 0);
				out("\t\t\t\treturn dest_0;\n");
				out("\t\t\t}\n");
				out("\t\t}\n");
				out("\t}\n");
				if (getter_type == 0) {
					out("\tverify_not_reached(\"Asset '%%s' has missing attribute '%s'.\",\n"
						"\t\tabsolute_link().to_string().c_str());\n", node->tag);
				} else {
					out("\treturn def;\n");
				}
				out("}\n");
				out("\n");
			}
			
			out("void %sAsset::set_%s(%s src_0)\n", asset_type->tag, node->tag, cpp_type.c_str());
			out("{\n");
			out("\tassert(bank().is_writeable());\n");
			out("\t%s dest_0;\n", cpp_type.c_str());
			generate_attribute_setter_code(node, 0);
			out("\tm_attribute_%s = std::move(dest_0);\n", node->tag);
			out("\tm_attrib_exists |= ATTRIB_%s;\n", node->tag);
			out("}\n");
			out("\n");
		}
	}
}

static void generate_attribute_getter_code(const WtfNode* attribute, int depth)
{
	int ind = depth + 4;
	
	if (strcmp(attribute->type_name, INTEGER_ATTRIB) == 0) {
		indent(ind); out("dest_%d = src_%d;\n", depth, depth);
	}
	
	if (strcmp(attribute->type_name, FLOAT_ATTRIB) == 0) {
		indent(ind); out("dest_%d = src_%d;\n", depth, depth);
	}
	
	if (strcmp(attribute->type_name, BOOLEAN_ATTRIB) == 0) {
		indent(ind); out("dest_%d = src_%d;\n", depth, depth);
	}
	
	if (strcmp(attribute->type_name, STRING_ATTRIB) == 0) {
		indent(ind); out("dest_%d = src_%d;\n", depth, depth);
	}
	
	if (strcmp(attribute->type_name, ARRAY_ATTRIB) == 0) {
		indent(ind); out("for (const auto& src_%d : src_%d) {\n", depth + 1, depth);
		indent(ind); out("\tdecltype(dest_%d)::value_type dest_%d;\n", depth, depth + 1);
		generate_attribute_getter_code(wtf_child(attribute, NULL, "element"), depth + 1);
		indent(ind); out("\tdest_%d.emplace_back(std::move(dest_%d));\n", depth, depth + 1);
		indent(ind); out("}\n");
	}
	
	if (strcmp(attribute->type_name, VECTOR3_ATTRIB) == 0) {
		indent(ind); out("dest_%d = src_%d;\n", depth, depth);
	}
	
	if (strcmp(attribute->type_name, COLOUR_ATTRIB) == 0) {
		indent(ind); out("dest_%d = src_%d;\n", depth, depth);
	}
	
	if (strcmp(attribute->type_name, ASSET_LINK_ATTRIB) == 0) {
		indent(ind); out("dest_%d = src_%d;\n", depth, depth);
	}
	
	if (strcmp(attribute->type_name, FILE_REFERENCE_ATTRIB) == 0) {
		indent(ind); out("dest_%d = src_%d;\n", depth, depth);
	}
}

static void generate_attribute_setter_code(const WtfNode* attribute, int depth)
{
	int ind = depth + 1;
	
	if (strcmp(attribute->type_name, INTEGER_ATTRIB) == 0) {
		indent(ind); out("dest_%d = src_%d;\n", depth, depth);
	}
	
	if (strcmp(attribute->type_name, FLOAT_ATTRIB) == 0) {
		indent(ind); out("dest_%d = src_%d;\n", depth, depth);
	}
	
	if (strcmp(attribute->type_name, BOOLEAN_ATTRIB) == 0) {
		indent(ind); out("dest_%d = src_%d;\n", depth, depth);
	}
	
	if (strcmp(attribute->type_name, STRING_ATTRIB) == 0) {
		indent(ind); out("dest_%d = src_%d;\n", depth, depth);
	}
	
	if (strcmp(attribute->type_name, ARRAY_ATTRIB) == 0) {
		indent(ind); out("for (const auto& src_%d : src_%d) {\n", depth + 1, depth);
		indent(ind); out("\tdecltype(dest_%d)::value_type dest_%d;\n", depth, depth + 1);
		generate_attribute_setter_code(wtf_child(attribute, NULL, "element"), depth + 1);
		indent(ind); out("\tdest_%d.emplace_back(std::move(dest_%d));\n", depth, depth + 1);
		indent(ind); out("}\n");
	}
	
	if (strcmp(attribute->type_name, VECTOR3_ATTRIB) == 0) {
		indent(ind); out("dest_%d = src_%d;\n", depth, depth);
	}
	
	if (strcmp(attribute->type_name, COLOUR_ATTRIB) == 0) {
		indent(ind); out("dest_%d = src_%d;\n", depth, depth);
	}
	
	if (strcmp(attribute->type_name, ASSET_LINK_ATTRIB) == 0) {
		indent(ind); out("dest_%d = src_%d;\n", depth, depth);
	}
	
	if (strcmp(attribute->type_name, FILE_REFERENCE_ATTRIB) == 0) {
		indent(ind); out("dest_%d = src_%d;\n", depth, depth);
	}
}

static void generate_child_functions(const WtfNode* asset_type)
{
	for (WtfNode* node = asset_type->first_child; node != NULL; node = node->next_sibling) {
		if (strcmp(node->type_name, "Child") == 0) {
			std::string getter_name = node->tag;
			assert(getter_name.size() >= 1);
			if (getter_name[0] >= '0' && getter_name[0] <= '9') {
				getter_name = '_' + getter_name;
			}
			
			const WtfAttribute* allowed_types = wtf_attribute(node, "allowed_types");
			assert(allowed_types);
			assert(allowed_types->type == WTF_ARRAY);
			assert(allowed_types->first_array_element);
			assert(allowed_types->first_array_element->type == WTF_STRING);
			const char* child_type = allowed_types->first_array_element->string.begin;
			
			out("bool %sAsset::has_%s() const\n", asset_type->tag, node->tag);
			out("{\n");
			out("\treturn has_child(\"%s\");\n", node->tag);
			out("}\n");
			out("\n");
			
			for (int is_const = 0; is_const < 2; is_const++) {
				const char* qualifier_space_after = is_const ? "const " : "";
				const char* qualifier_space_before = is_const ? " const" : "";
				if (allowed_types->first_array_element->next) {
					out("%sAsset& %sAsset::get_%s()%s\n", qualifier_space_after, asset_type->tag, node->tag, qualifier_space_before);
					out("{\n");
					out("\treturn get_child(\"%s\");\n", node->tag);
				} else {
					out("%s%sAsset& %sAsset::get_%s()%s\n", qualifier_space_after, child_type, asset_type->tag, node->tag, qualifier_space_before);
					out("{\n");
					out("\treturn get_child(\"%s\").as<%sAsset>();\n", node->tag, child_type);
				}
				out("}\n");
				out("\n");
			}
		}
	}
}

static std::string node_to_cpp_type(const WtfNode* node)
{
	if (strcmp(node->type_name, INTEGER_ATTRIB) == 0) {
		return "int";
	}
	
	if (strcmp(node->type_name, FLOAT_ATTRIB) == 0) {
		return "float";
	}
	
	if (strcmp(node->type_name, BOOLEAN_ATTRIB) == 0) {
		return "bool";
	}
	
	if (strcmp(node->type_name, STRING_ATTRIB) == 0) {
		return "std::string";
	}
	
	if (strcmp(node->type_name, ARRAY_ATTRIB) == 0) {
		const WtfNode* element = wtf_child(node, NULL, "element");
		assert(element);
		std::string element_type = node_to_cpp_type(element);
		assert(!element_type.empty());
		return "std::vector<" + element_type + ">";
	}
	
	if (strcmp(node->type_name, VECTOR3_ATTRIB) == 0) {
		return "glm::vec3";
	}
	
	if (strcmp(node->type_name, COLOUR_ATTRIB) == 0) {
		return "glm::vec4";
	}
	
	if (strcmp(node->type_name, ASSET_LINK_ATTRIB) == 0) {
		return "AssetLink";
	}
	
	if (strcmp(node->type_name, FILE_REFERENCE_ATTRIB) == 0) {
		return "FileReference";
	}
	
	return "";
}

static void indent(int ind)
{
	for (int i = 0; i < ind; i++) {
		out("\t");
	}
}

static void out(const char* format, ...)
{
	va_list list;
	va_start(list, format);
	if (out_file != NULL) {
		vfprintf(out_file, format, list);
	}
	if (out_handle != NULL) {
		file_vprintf(out_handle, format, list);
	}
	va_end(list);
}
