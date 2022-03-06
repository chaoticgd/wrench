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

#include "asset_util.h"

#include "asset_types.h"

AssetReference parse_asset_reference(const char* ptr) {
	std::string string(ptr);
	std::vector<AssetReferenceFragment> fragments;
	
	size_t begin = string[0] == '/' ? 1 : 0;
	size_t type_name_begin = begin;
	size_t tag_begin = begin;
	
	for(size_t i = begin; i < string.size(); i++) {
		if(string[i] == ':') {
			tag_begin = i + 1;
		}
		if(string[i] == '/') {
			std::string type_name = string.substr(type_name_begin, tag_begin - type_name_begin - 1);
			AssetType type = asset_string_to_type(type_name.c_str());
			std::string tag = string.substr(tag_begin, i - tag_begin);
			fragments.push_back(AssetReferenceFragment{type, std::move(tag)});
			type_name_begin = i + 1;
		}
	}
	
	if(type_name_begin != tag_begin) {
		std::string type_name = string.substr(type_name_begin, tag_begin - type_name_begin - 1);
		AssetType type = asset_string_to_type(type_name.c_str());
		std::string tag = string.substr(tag_begin, string.size() - tag_begin);
		fragments.push_back(AssetReferenceFragment{type, std::move(tag)});
	}
	
	return {string[0] != '/', fragments};
}

std::string asset_reference_to_string(const AssetReference& reference) {
	std::string string;
	if(!reference.is_relative) {
		string += '/';
	}
	for(auto fragment = reference.fragments.begin(); fragment != reference.fragments.end(); fragment++) {
		string += asset_type_to_string(fragment->type);
		string += ':';
		string += fragment->tag;
		if(fragment + 1 != reference.fragments.end()) {
			string += '/';
		}
	}
	return string;
}
