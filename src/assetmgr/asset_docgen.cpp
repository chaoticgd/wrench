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

#include <math.h>
#include <string>
#include <vector>
#undef NDEBUG
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <filesystem>

#include <platform/fileio.h>

#include <wtf/wtf.h>
#include <wtf/wtf_writer.h>

static void write_index(const WtfNode* root);
static void write_contents(const WtfNode* root);
static void write_attribute_table(const WtfNode* asset_type);
static void write_child_table(const WtfNode* asset_type);
static void write_type_list(std::string& dest, const WtfNode* child, int depth);
static void write_examples(const WtfNode* examples);
static void write_hints(const WtfNode* asset_type);
static std::string to_link(const char* str);
static void reify_node(WtfWriter* ctx, const WtfNode* node);
static void reify_value(WtfWriter* ctx, const WtfAttribute* value);
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
	
	out("# Asset Reference\n");
	out("\n");
	out("This file was generated from %s and is for version %d of the asset format.\n",
		std::filesystem::path(argv[1]).filename().string().c_str(), format_version->number.i);
	
	write_index(root);
	write_contents(root);
	
	wtf_free(root);

	if (out_handle != NULL) {
		file_close(out_handle);
	}
}

static void write_index(const WtfNode* root)
{
	out("\n");
	out("## Index\n");
	out("\n");
	out("- [Index](#index)\n");
	for (const WtfNode* node = root->first_child; node != NULL; node = node->next_sibling) {
		if (strcmp(node->type_name, "Category") == 0) {
			const WtfAttribute* name = wtf_attribute(node, "name");
			assert(name && name->type == WTF_STRING);
			out("- [%s](#%s)\n", name->string.begin, to_link(name->string.begin).c_str());
		}
		
		if (strcmp(node->type_name, "AssetType") == 0) {
			const WtfAttribute* hidden = wtf_attribute(node, "hidden");
			if (hidden && hidden->type == WTF_BOOLEAN && hidden->boolean) {
				continue;
			}
			
			out("\t- [%s](#%s)\n", node->tag, to_link(node->tag).c_str());
		}
	}
}

static void write_contents(const WtfNode* root)
{
	for (const WtfNode* node = root->first_child; node != NULL; node = node->next_sibling) {
		if (strcmp(node->type_name, "Category") == 0) {
			const WtfAttribute* name = wtf_attribute(node, "name");
			assert(name && name->type == WTF_STRING);
			out("\n");
			out("## %s\n", name->string.begin);
		}
		
		if (strcmp(node->type_name, "AssetType") == 0) {
			const WtfAttribute* hidden = wtf_attribute(node, "hidden");
			if (hidden && hidden->type == WTF_BOOLEAN && hidden->boolean) {
				continue;
			}
			
			out("\n");
			out("### %s\n", node->tag);
			const WtfAttribute* desc = wtf_attribute(node, "desc");
			if (desc && desc->type == WTF_STRING) {
				out("\n");
				out("%s\n", desc->string.begin);
			}
			
			write_attribute_table(node);
			write_child_table(node);
			
			const WtfNode* examples = wtf_child(node, "Examples", "examples");
			if (examples) {
				write_examples(examples);
			}
			
			write_hints(node);
		}
	}
}

static void write_attribute_table(const WtfNode* asset_type)
{
	out("\n");
	out("*Attributes*\n\n");
	
	bool has_attributes = false;
	for (const WtfNode* child = asset_type->first_child; child != nullptr; child = child->next_sibling) {
		const WtfAttribute* attrib_hidden = wtf_attribute(child, "hidden");
		if (attrib_hidden && attrib_hidden->type == WTF_BOOLEAN && attrib_hidden->boolean) {
			continue;
		}
		char* end_of_name = strrchr(child->type_name, 'A');
		if (!end_of_name || strcmp(end_of_name, "Attribute") != 0) {
			continue;
		}
		has_attributes = true;
		break;
	}
	
	if (!has_attributes) {
		out("No attributes.\n");
		return;
	}
	
	out("| Name | Description | Type | Required | Games |\n");
	out("| - | - | - | - | - |\n");
	for (const WtfNode* child = asset_type->first_child; child != nullptr; child = child->next_sibling) {
		const WtfAttribute* attrib_hidden = wtf_attribute(child, "hidden");
		if (attrib_hidden && attrib_hidden->type == WTF_BOOLEAN && attrib_hidden->boolean) {
			continue;
		}
		
		const WtfAttribute* desc = wtf_attribute(child, "desc");
		const char* desc_str = (desc && desc->type == WTF_STRING)
			? desc->string.begin : "*Not yet documented.*";
		const char* type;
		if (strcmp(child->type_name, "IntegerAttribute") == 0) {
			type = "Integer";
		} else if (strcmp(child->type_name, "FloatAttribute") == 0) {
			type = "Float";
		} else if (strcmp(child->type_name, "BooleanAttribute") == 0) {
			type = "Boolean";
		} else if (strcmp(child->type_name, "StringAttribute") == 0) {
			type = "String";
		} else if (strcmp(child->type_name, "ArrayAttribute") == 0) {
			type = "Array";
		} else if (strcmp(child->type_name, "Vector3Attribute") == 0) {
			type = "Vector3";
		} else if (strcmp(child->type_name, "ColourAttribute") == 0) {
			type = "Colour";
		} else if (strcmp(child->type_name, "AssetLinkAttribute") == 0) {
			type = "AssetLink";
		} else if (strcmp(child->type_name, "FileReferenceAttribute") == 0) {
			type = "FilePath";
		} else {
			continue;
		}
		const WtfAttribute* required = wtf_attribute(child, "required");
		const char* required_str = (required && required->type == WTF_BOOLEAN)
			? (required->boolean ? "Yes" : "No") : "*Not yet documented.*";
		std::string games_str;
		const WtfAttribute* games = wtf_attribute(child, "games");
		if (games && games->type == WTF_ARRAY) {
			for (const WtfAttribute* elem = games->first_array_element; elem != nullptr; elem = elem->next) {
				if (elem->type == WTF_NUMBER) {
					switch (elem->number.i) {
						case 1: games_str += "RAC"; break;
						case 2: games_str += "GC"; break;
						case 3: games_str += "UYA"; break;
						case 4: games_str += "DL"; break;
					}
				}
				if (elem->next != nullptr) {
					games_str += "/";
				}
			}
		} else {
			games_str += "*Not yet documented.* ";
		}
		out("| %s | %s | %s | %s | %s |\n", child->tag, desc_str, type, required_str, games_str.c_str());
	}
}

static void write_child_table(const WtfNode* asset_type)
{
	out("\n");
	out("*Children*\n");
	out("\n");
	
	bool has_children = false;
	for (const WtfNode* child = asset_type->first_child; child != nullptr; child = child->next_sibling) {
		if (strcmp(child->type_name, "Child") == 0) {
			has_children = true;
			break;
		}
	}
	
	if (!has_children) {
		out("No children.\n");
		return;
	}
	
	out("| Name | Description | Allowed Types | Required | Games |\n");
	out("| - | - | - | - | - |\n");
	for (const WtfNode* child = asset_type->first_child; child != nullptr; child = child->next_sibling) {
		if (strcmp(child->type_name, "Child") == 0) {
			const char* name_str = child->tag;
			const WtfAttribute* desc = wtf_attribute(child, "desc");
			const char* desc_str = (desc && desc->type == WTF_STRING)
				? desc->string.begin : "*Not yet documented.*";
			std::string types_str;
			write_type_list(types_str, child, 0);
			if (types_str.size() > 0) {
				types_str = types_str.substr(0, types_str.size() - 2);
			} else {
				types_str = "*Not yet documented.*";
			}
			const WtfAttribute* required = wtf_attribute(child, "required");
			const char* required_str = (required && required->type == WTF_BOOLEAN)
				? (required->boolean ? "Yes" : "No") : "*Not yet documented.*";
			const WtfAttribute* games = wtf_attribute(child, "games");
			std::string games_str;
			if (games && games->type == WTF_ARRAY) {
				for (const WtfAttribute* elem = games->first_array_element; elem != nullptr; elem = elem->next) {
					if (elem->type == WTF_NUMBER) {
						switch (elem->number.i) {
							case 1: games_str += "RAC"; break;
							case 2: games_str += "GC"; break;
							case 3: games_str += "UYA"; break;
							case 4: games_str += "DL"; break;
						}
					}
					if (elem->next != nullptr) {
						games_str += "/";
					}
				}
			} else {
				games_str += "*Not yet documented.*";
			}
			out("| %s | %s | %s | %s | %s |\n", name_str, desc_str, types_str.c_str(), required_str, games_str.c_str());
		}
	}
	out("\n");
}

static void write_type_list(std::string& dest, const WtfNode* child, int depth)
{
	const WtfAttribute* types = wtf_attribute(child, "allowed_types");
	if (types && types->type == WTF_ARRAY) {
		for (const WtfAttribute* elem = types->first_array_element; elem != nullptr; elem = elem->next) {
			if (elem->type == WTF_STRING) {
				const WtfNode* sub_child = wtf_child(child, "Child", "child");
				if (strcmp(elem->string.begin, "Collection") == 0 && sub_child) {
					write_type_list(dest, sub_child, depth + 1);
				} else {
					dest += elem->string.begin;
					for (int i = 0; i < depth; i++) {
						dest += "\\[\\]";
					}
					dest += ", ";
				}
			} else {
				abort();
			}
		}
	}
}

static void write_examples(const WtfNode* examples)
{
	int example_count = 0;
	for (const WtfNode* child = examples->first_child; child != nullptr; child = child->next_sibling) {
		example_count++;
	}
	
	out("\n");
	if (example_count == 1) {
		out("*Example*\n");
	} else {
		out("*Examples*\n");
	}
	out("\n");
	
	for (const WtfNode* example = examples->first_child; example != nullptr; example = example->next_sibling) {
		std::string str;
		WtfWriter* ctx = wtf_begin_file(str);
		reify_node(ctx, example);
		out("```\n");
		out("%s", str.c_str());
		out("```\n");
		wtf_end_file(ctx);
	}
}

static void write_hints(const WtfNode* asset_type)
{
	int hint_count = 0;
	for (const WtfNode* child = asset_type->first_child; child != nullptr; child = child->next_sibling) {
		if (strcmp(child->type_name, "Hint") == 0) {
			hint_count++;
		}
	}
	
	if (hint_count == 0) {
		return;
	}
	
	out("\n");
	out("*Hints*\n");
	out("\n");
	
	out("| Syntax | Example | Description |\n");
	out("| - | - | - |\n");
	for (const WtfNode* child = asset_type->first_child; child != nullptr; child = child->next_sibling) {
		if (strcmp(child->type_name, "Hint") == 0) {
			const WtfAttribute* syntax = wtf_attribute(child, "syntax");
			assert(syntax && syntax->type == WTF_STRING);
			const WtfAttribute* example = wtf_attribute(child, "example");
			assert(example && example->type == WTF_STRING);
			const WtfAttribute* desc = wtf_attribute(child, "desc");
			assert(desc && desc->type == WTF_STRING);
			
			out("| `%s` | `%s` | ", syntax->string.begin, example->string.begin);
			for (size_t i = 0; i < strlen(desc->string.begin); i++) {
				if (desc->string.begin[i] == '<' || desc->string.begin[i] == '>') {
					out("\\");
				}
				if (desc->string.begin[i] == '\'') {
					out("`");
				} else {
					out("%c", desc->string.begin[i]);
				}
			}
			out(" |\n");
		}
	}
}

static std::string to_link(const char* str)
{
	std::string out;
	for (size_t i = 0; i < strlen(str); i++) {
		if (str[i] == ' ') {
			out += '-';
		} else {
			out += tolower(str[i]);
		}
	}
	return out;
}

static void reify_node(WtfWriter* ctx, const WtfNode* node)
{
	wtf_begin_node(ctx, node->type_name, node->tag);
	for (WtfAttribute* attrib = node->first_attribute; attrib != nullptr; attrib = attrib->next) {
		wtf_begin_attribute(ctx, attrib->key);
		reify_value(ctx, attrib);
		wtf_end_attribute(ctx);
	}
	for (WtfNode* child = node->first_child; child != nullptr; child = child->next_sibling) {
		reify_node(ctx, child);
	}
	wtf_end_node(ctx);
}

static void reify_value(WtfWriter* ctx, const WtfAttribute* value)
{
	switch (value->type) {
		case WTF_NUMBER: {
			if (fabsf(floorf(value->number.f) - value->number.f) > 0.0001f) {
				wtf_write_float(ctx, value->number.f);
			} else {
				wtf_write_integer(ctx, value->number.i);
			}
			break;
		}
		case WTF_BOOLEAN: {
			wtf_write_boolean(ctx, value->boolean);
			break;
		}
		case WTF_STRING: {
			wtf_write_string(ctx, value->string.begin);
			break;
		}
		case WTF_ARRAY: {
			wtf_begin_array(ctx);
			for (WtfAttribute* elem = value->first_array_element; elem != nullptr; elem = elem->next) {
				reify_value(ctx, elem);
			}
			wtf_end_array(ctx);
			break;
		}
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
