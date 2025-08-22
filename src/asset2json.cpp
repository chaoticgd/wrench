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

#include <wtf/wtf.h>
#include <core/filesystem.h>

#include <nlohmann/json.hpp>
using Json = nlohmann::ordered_json;

static Json node_to_json(WtfNode* node);
static Json attribute_to_json(WtfAttribute* attribute);

int main(int argc, char** argv)
{
	if (argc != 3) {
		fprintf(stderr, "usage: %s <input .asset file> <output .json file>\n", argv[0]);
		return 1;
	}
	
	std::vector<u8> input = read_file(argv[1], true);
	input.push_back(0); // Null terminator.
	
	char* error = nullptr;
	WtfNode* root = wtf_parse((char*) input.data(), &error);
	if (error) {
		fprintf(stderr, "error: %s\n", error);
		return 1;
	}
	
	Json json = node_to_json(root);
	
	std::string output = json.dump(1, '\t');
	write_file(argv[2], Buffer(output), true);
	
	wtf_free(root);
}

static Json node_to_json(WtfNode* node)
{
	Json json = Json::object();
	json["type_name"] = node->type_name ? node->type_name : Json();
	json["tag"] = node->tag ? node->tag : Json();
	for (WtfAttribute* attribute = node->first_attribute; attribute != nullptr; attribute = attribute->next) {
		json[attribute->key] = attribute_to_json(attribute);
	}
	if (node->first_child != NULL) {
		Json children = Json::array();
		for (WtfNode* child = node->first_child; child != NULL; child = child->next_sibling) {
			children.emplace_back(node_to_json(child));
		}
		json["children"] = children;
	}
	return json;
}

static Json attribute_to_json(WtfAttribute* attribute)
{
	switch (attribute->type) {
		case WTF_NUMBER: {
			return attribute->number.f;
		}
		case WTF_BOOLEAN: {
			return (bool) attribute->boolean;
		}
		case WTF_STRING: {
			return attribute->string.begin;
		}
		case WTF_ARRAY: {
			Json array = Json::array();
			for (WtfAttribute* element = attribute->first_array_element; element != NULL; element = attribute->next) {
				array.emplace_back(attribute_to_json(element));
			}
			return array;
		}
	}
	return {};
}
