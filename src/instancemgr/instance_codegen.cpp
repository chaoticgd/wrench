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

#include <string>
#include <vector>
#undef NDEBUG
#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include <platform/fileio.h>
#include <wtf/wtf.h>

static void generate_instance_macro_calls(WtfNode* root);
static void generate_instance_type_enum(WtfNode* root);
static void generate_instance_types(WtfNode* root);
static void generate_instance_read_write_funcs(WtfNode* root);
static void generate_instance_read_write_table(WtfNode* root);
static void generate_instance_type_to_string_func(WtfNode* root);
static void out(const char* format, ...);

static WrenchFileHandle* out_handle = NULL;

int main(int argc, char** argv)
{
	assert(argc == 3);
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

	out_handle = file_open(argv[2], WRENCH_FILE_MODE_WRITE);
	assert(out_handle);
	
	char* error = NULL;
	WtfNode* root = wtf_parse((char*) bytes.data(), &error);
	if (error) {
		fprintf(stderr, "Failed to parse instance schema. %s\n", error);
		return 1;
	}
	
	const WtfAttribute* format_version = wtf_attribute(root, "format_version");
	assert(format_version && format_version->type == WTF_NUMBER);
	
	out("// Generated from %s. Do not edit.", argv[1]);
	out("");
	out("#ifdef GENERATED_INSTANCE_MACRO_CALLS");
	out("");
	generate_instance_macro_calls(root);
	out("");
	out("#endif");
	out("#ifdef GENERATED_INSTANCE_TYPE_ENUM");
	out("");
	generate_instance_type_enum(root);
	out("");
	out("#endif");
	out("#ifdef GENERATED_INSTANCE_TYPES");
	out("");
	out("#define INSTANCE_FORMAT_VERSION %d", format_version->number.i);
	out("");
	generate_instance_types(root);
	out("#endif");
	out("#ifdef GENERATED_INSTANCE_READ_WRITE_FUNCS");
	out("");
	generate_instance_read_write_funcs(root);
	out("#endif");
	out("#ifdef GENERATED_INSTANCE_READ_WRITE_TABLE");
	out("");
	generate_instance_read_write_table(root);
	out("");
	out("#endif");
	out("#ifdef GENERATED_INSTANCE_TYPE_TO_STRING_FUNC");
	out("");
	generate_instance_type_to_string_func(root);
	out("");
	out("#endif");
}

static void generate_instance_macro_calls(WtfNode* root)
{
	for (const WtfNode* type = wtf_first_child(root, "InstanceType"); type != nullptr; type = wtf_next_sibling(type, "InstanceType")) {
		const WtfAttribute* variable = wtf_attribute_of_type(type, "variable", WTF_STRING);
		assert(variable);
		
		const WtfAttribute* link_type = wtf_attribute_of_type(type, "link_type", WTF_STRING);
		assert(link_type);
		
		std::string uppercase = type->tag;
		for (char& c : uppercase) c = toupper(c);
		
		out("DEF_INSTANCE(%s, %s, %s, %s)", type->tag, uppercase.c_str(), variable->string.begin, link_type->string.begin);
	}
}

static void generate_instance_type_enum(WtfNode* root)
{
	out("enum InstanceType : u32 {");
	out("\tINST_NONE = 0,");
	int number = 1;
	for (const WtfNode* type = wtf_first_child(root, "InstanceType"); type != nullptr; type = wtf_next_sibling(type, "InstanceType")) {
		std::string enum_name = type->tag;
		for (char& c : enum_name) c = toupper(c);
		out("\tINST_%s = %d,", enum_name.c_str(), number++);
	}
	out("};\n");
}

static const struct { const char* wtf_type; const char* cpp_type; const char* set = nullptr; } FIELD_TYPES[] = {
	{"vec3", "glm::vec3", "glm::vec3(0.f, 0.f, 0.f)"},
	{"vec4", "glm::vec4", "glm::vec4(0.f, 0.f, 0.f, 0.f)"},
	{"mat4", "glm::mat4", "glm::mat4(1.f)"},
	{"bytes", "std::vector<u8>"},
	{"Rgb32", "Rgb32", "{}"},
	{"Rgb96", "Rgb96", "{}"}
};

static void generate_instance_types(WtfNode* root)
{
	for (const WtfNode* type = wtf_first_child(root, "InstanceType"); type != nullptr; type = wtf_next_sibling(type, "InstanceType")) {
		const WtfAttribute* components = wtf_attribute_of_type(type, "components", WTF_STRING);
		assert(components);
		
		const WtfAttribute* transform_mode = wtf_attribute_of_type(type, "transform_mode", WTF_STRING);
		
		std::string enum_name = type->tag;
		for (char& c : enum_name) c = toupper(c);
		
		out("struct %sInstance : Instance {", type->tag);
		out("\tstatic const InstanceType TYPE = INST_%s;", enum_name.c_str());
		if (transform_mode) {
			out("\t%sInstance() : Instance(TYPE, %s, TransformMode::%s) {}", type->tag, components->string.begin, transform_mode->string.begin);
		} else {
			out("\t%sInstance() : Instance(TYPE, %s) {}", type->tag, components->string.begin);
		}
		out("\tstatic void read(Instances& dest, const WtfNode* src);");
		out("\tstatic void write(WtfWriter* dest, const Instances& src);");
		out("\t");
		for (WtfNode* field = type->first_child; field != nullptr; field = field->next_sibling) {
			const char* cpp_type = nullptr;
			const char* set = "0";
			for (auto& field_type : FIELD_TYPES) {
				if (strcmp(field_type.wtf_type, field->type_name) == 0) {
					cpp_type = field_type.cpp_type;
					set = field_type.set;
				}
			}
			if (strstr(field->type_name, "link")) {
				set = nullptr;
			}
			if (!cpp_type) {
				cpp_type = field->type_name;
			}
			if (set) {
				out("\t%s %s = %s;", cpp_type, field->tag, set);
			} else {
				out("\t%s %s;", cpp_type, field->tag);
			}
		}
		out("};");
		out("");
	}
}

static void generate_instance_read_write_funcs(WtfNode* root)
{
	for (const WtfNode* type = wtf_first_child(root, "InstanceType"); type != nullptr; type = wtf_next_sibling(type, "InstanceType")) {
		const WtfAttribute* variable = wtf_attribute_of_type(type, "variable", WTF_STRING);
		assert(variable);
		
		out("void %sInstance::read(Instances& dest, const WtfNode* src)", type->tag);
		out("{");
		out("\t%sInstance& inst = dest.%s.create(atoi(src->tag));", type->tag, variable->string.begin);
		out("\tinst.read_common(src);");
		for (const WtfNode* field = type->first_child; field != nullptr; field = field->next_sibling) {
			out("\tread_inst_field(inst.%s, src, \"%s\");", field->tag, field->tag);
		}
		out("}");
		out("");
		out("void %sInstance::write(WtfWriter* dest, const Instances& src)", type->tag);
		out("{");
		out("\tfor (const %sInstance& inst : src.%s) {", type->tag, variable->string.begin);
		out("\t\tinst.begin_write(dest);");
		for (const WtfNode* field = type->first_child; field != nullptr; field = field->next_sibling) {
			out("\t\twrite_inst_field(dest, \"%s\", inst.%s);", field->tag, field->tag);
		}
		out("\t\tinst.end_write(dest);");
		out("\t}");
		out("}");
		out("");
	}
}

static void generate_instance_read_write_table(WtfNode* root)
{
	out("static const InstanceReadWriteFuncs read_write_funcs[] = {");
	for (const WtfNode* type = wtf_first_child(root, "InstanceType"); type != nullptr; type = wtf_next_sibling(type, "InstanceType")) {
		std::string enum_name = type->tag;
		for (char& c : enum_name) c = toupper(c);
		out("\t{INST_%s, %sInstance::read, %sInstance::write},", enum_name.c_str(), type->tag, type->tag);
	}
	out("};");
}

static void generate_instance_type_to_string_func(WtfNode* root)
{
	out("const char* instance_type_to_string(InstanceType type)");
	out("{");
	out("\tswitch (type) {");
	out("\t\tcase INST_NONE: return \"None\";");
	for (const WtfNode* type = wtf_first_child(root, "InstanceType"); type != nullptr; type = wtf_next_sibling(type, "InstanceType")) {
		std::string enum_name = type->tag;
		for (char& c : enum_name) c = toupper(c);
		out("\t\tcase INST_%s: return \"%s\";", enum_name.c_str(), type->tag);
	}
	out("\t}");
	out("\tverify_not_reached(\"Tried to lookup name of bad instance type.\");");
	out("};");
}

static void out(const char* format, ...)
{
	va_list list;
	va_start(list, format);
	file_vprintf(out_handle, format, list);
	file_write("\n", 1, out_handle);
	va_end(list);
}
