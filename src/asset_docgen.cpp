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

static void write_index(const WtfNode* root);
static void write_contents(const WtfNode* root);
static void write_attribute_table(const WtfNode* asset_type);
static void write_child_table(const WtfNode* asset_type);
static std::string to_link(const char* str);
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
	
	out("# Asset Reference\n");
	out("\n");
	out("This file was generated from %s.\n", argv[1]);
	
	write_index(root);
	write_contents(root);
	
	wtf_free(root);
}

static void write_index(const WtfNode* root) {
	out("\n");
	out("## Index\n");
	out("\n");
	out("- [Index](#index)\n");
	for(const WtfNode* node = root->first_child; node != NULL; node = node->next_sibling) {
		if(strcmp(node->type_name, "Category") == 0) {
			const WtfAttribute* name = wtf_attribute(node, "name");
			assert(name && name->type == WTF_STRING);
			out("- [%s](#%s)\n", name->string, to_link(name->string).c_str());
		}
		
		if(strcmp(node->type_name, "AssetType") == 0) {
			const WtfAttribute* hidden = wtf_attribute(node, "hidden");
			if(hidden && hidden->type == WTF_BOOLEAN && hidden->boolean) {
				continue;
			}
			
			out("\t- [%s](#%s)\n", node->tag, to_link(node->tag).c_str());
		}
	}
}

static void write_contents(const WtfNode* root) {
	for(const WtfNode* node = root->first_child; node != NULL; node = node->next_sibling) {
		if(strcmp(node->type_name, "Category") == 0) {
			const WtfAttribute* name = wtf_attribute(node, "name");
			assert(name && name->type == WTF_STRING);
			out("\n");
			out("## %s\n", name->string);
		}
		
		if(strcmp(node->type_name, "AssetType") == 0) {
			const WtfAttribute* hidden = wtf_attribute(node, "hidden");
			if(hidden && hidden->type == WTF_BOOLEAN && hidden->boolean) {
				continue;
			}
			
			out("\n");
			out("### %s\n", node->tag);
			const WtfAttribute* desc = wtf_attribute(node, "desc");
			if(desc && desc->type == WTF_STRING) {
				out("\n");
				out("%s\n", desc->string);
			}
			out("\n");
			out("*Attributes*\n");
			
			write_attribute_table(node);
			
			out("\n");
			out("*Children*\n");
			out("\n");
			
			write_child_table(node);
		}
	}
}

static void write_attribute_table(const WtfNode* asset_type) {
	out("| Name | Description | Type | Required | Games |\n");
	out("| - | - | - | - | - |\n");
	for(const WtfNode* child = asset_type->first_child; child != nullptr; child = child->next_sibling) {
		const WtfAttribute* attrib_hidden = wtf_attribute(child, "hidden");
		if(attrib_hidden && attrib_hidden->type == WTF_BOOLEAN && attrib_hidden->boolean) {
			continue;
		}
		
		const WtfAttribute* desc = wtf_attribute(child, "desc");
		const char* desc_str = (desc && desc->type == WTF_STRING)
			? desc->string : "*Not yet documented.*";
		const char* type;
		if(strcmp(child->type_name, "IntegerAttribute") == 0) {
			type = "Integer";
		} else if(strcmp(child->type_name, "BooleanAttribute") == 0) {
			type = "Boolean";
		} else if(strcmp(child->type_name, "StringAttribute") == 0) {
			type = "String";
		} else if(strcmp(child->type_name, "ArrayAttribute") == 0) {
			type = "Array";
		} else if(strcmp(child->type_name, "AssetReferenceAttribute") == 0) {
			type = "Asset Reference";
		} else if(strcmp(child->type_name, "FileReferenceAttribute") == 0) {
			type = "File Path";
		} else {
			continue;
		}
		const WtfAttribute* required = wtf_attribute(child, "required");
		const char* required_str = (required && required->type == WTF_BOOLEAN)
			? (required->boolean ? "Yes" : "No") : "*Not yet documented.*";
		std::string games_str;
		const WtfAttribute* games = wtf_attribute(child, "games");
		if(games && games->type == WTF_ARRAY) {
			for(const WtfAttribute* elem = games->first_array_element; elem != nullptr; elem = elem->next) {
				if(elem->type == WTF_NUMBER) {
					switch(elem->number.i) {
						case 1: games_str += "RC"; break;
						case 2: games_str += "GC"; break;
						case 3: games_str += "UYA"; break;
						case 4: games_str += "DL"; break;
					}
				}
				if(elem->next != nullptr) {
					games_str += "/";
				}
			}
		} else {
			games_str += "*Not yet documented.* ";
		}
		out("| %s | %s | %s | %s | %s |\n", child->tag, desc_str, type, required_str, games_str.c_str());
	}
}

static void write_child_table(const WtfNode* asset_type) {
	out("| Name | Description | Allowed Types | Required | Games |\n");
	out("| - | - | - | - | - |\n");
	for(const WtfNode* child = asset_type->first_child; child != nullptr; child = child->next_sibling) {
		if(strcmp(child->type_name, "Child") == 0) {
			out("| %s ", child->tag);
			const WtfAttribute* desc = wtf_attribute(child, "desc");
			if(desc && desc->type == WTF_STRING) {
				out("| %s", desc->string);
			} else {
				out("| *Not yet documented.*");
			}
			out(" | ");
			const WtfAttribute* types = wtf_attribute(child, "allowed_types");
			if(types && types->type == WTF_ARRAY) {
				for(const WtfAttribute* elem = types->first_array_element; elem != nullptr; elem = elem->next) {
					if(elem->type == WTF_STRING) {
						out("%s", elem->string);
						if(elem->next != nullptr) {
							out(", ");
						}
					} else {
						abort();
					}
				}
				out("");
			} else {
				out("*Not yet documented.*");
			}
			out(" | ");
			const WtfAttribute* required = wtf_attribute(child, "required");
			if(required && required->type == WTF_BOOLEAN) {
				out("%s", required->boolean ? "Yes" : "No");
			} else {
				out("*Not yet documented.*");
			}
			out(" | ");
			const WtfAttribute* games = wtf_attribute(child, "games");
			if(games && games->type == WTF_ARRAY) {
				for(const WtfAttribute* elem = games->first_array_element; elem != nullptr; elem = elem->next) {
					if(elem->type == WTF_NUMBER) {
						switch(elem->number.i) {
							case 1: out("RC"); break;
							case 2: out("GC"); break;
							case 3: out("UYA"); break;
							case 4: out("DL"); break;
						}
					}
					if(elem->next != nullptr) {
						out("/");
					}
				}
			} else {
				out("*Not yet documented.* ");
			}
			out("|\n");
		}
	}
	out("\n");
}

static std::string to_link(const char* str) {
	std::string out;
	for(s64 i = 0; i < strlen(str); i++) {
		if(str[i] == ' ') {
			out += '-';
		} else {
			out += tolower(str[i]);
		}
	}
	return out;
}

static void out(const char* format, ...) {
	va_list list;
	va_start(list, format);
	vfprintf(out_file, format, list);
	va_end(list);
}
