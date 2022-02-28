/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2022 chaoticgd

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

#include <core/wtf.h>
#include <core/filesystem.h>

#include <nlohmann/json.hpp>
using Json = nlohmann::ordered_json;

static Json node_to_json(WtfFile* file, s32 index);
static Json attribute_to_json(WtfFile* file, WtfAttribute* attrib);

int main(int argc, char** argv) {
	if(argc != 3) {
		fprintf(stderr, "usage: %s <input .asset file> <output .json file>\n", argv[0]);
		return 1;
	}
	
	std::vector<u8> input = read_file(argv[1], "r");
	input.push_back(0); // Null terminator.
	
	char* error = nullptr;
	WtfFile* file = wtf_parse((char*) input.data(), &error);
	if(error) {
		fprintf(stderr, "error: %s\n", error);
		return 1;
	}
	
	Json json = node_to_json(file, 0);
	
	std::string output = json.dump(1, '\t');
	write_file("/", argv[2], Buffer(output), "w");
}

static Json node_to_json(WtfFile* file, s32 index) {
	Json json = Json::object();
	WtfNode* node = &file->nodes[index];
	json["type_name"] = node->type_name ? node->type_name : Json();
	json["tag"] = node->tag ? node->tag : Json();
	for(s32 attrib = node->first_attribute; attrib != -1; attrib = file->attributes[attrib].next) {
		WtfAttribute* attribute = &file->attributes[attrib];
		json[attribute->key] = attribute_to_json(file, attribute);
	}
	if(node->first_child != -1) {
		Json children = Json::array();
		for(s32 child = node->first_child; child != -1; child = file->nodes[child].next_sibling) {
			children.emplace_back(node_to_json(file, child));
		}
		json["children"] = children;
	}
	return json;
}

static Json attribute_to_json(WtfFile* file, WtfAttribute* attrib) {
	if(attrib->type == WTF_FLOAT) {
		return attrib->f;
	} else if(attrib->type == WTF_STRING) {
		return attrib->s;
	} else if(attrib->type == WTF_ARRAY) {
		Json array = Json::array();
		for(s32 element = attrib->first_array_element; element != -1; element = file->attributes[element].next) {
			array.emplace_back(attribute_to_json(file, &file->attributes[element]));
		}
		return array;
	}
	return {};
}
