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
	
	AssetReference reference;
	
	size_t tag_begin = 0;
	for(size_t i = 0; i < string.size(); i++) {
		if(string[i] == '.') {
			std::string tag = string.substr(tag_begin, i - tag_begin);
			reference.fragments.push_back(AssetReferenceFragment{std::move(tag)});
			tag_begin = i + 1;
		}
		if(string[i] == '@') {
			std::string tag = string.substr(tag_begin, i - tag_begin);
			reference.fragments.push_back(AssetReferenceFragment{std::move(tag)});
			reference.pack = string.substr(i + 1);
			return reference;
		}
	}
	
	std::string tag = string.substr(tag_begin, string.size() - tag_begin);
	if(tag.size() > 0) {
		reference.fragments.push_back(AssetReferenceFragment{std::move(tag)});
	}
	
	return reference;
}

std::string asset_reference_to_string(const AssetReference& reference) {
	std::string string;
	for(auto fragment = reference.fragments.begin(); fragment != reference.fragments.end(); fragment++) {
		string += fragment->tag;
		if(fragment + 1 != reference.fragments.end()) {
			string += '.';
		}
	}
	if(!reference.pack.empty()) {
		string += '@' + reference.pack;
	}
	return string;
}

const char* next_hint(const char** hint) {
	static char temp[256];
	if(hint) {
		for(s32 i = 0;; i++) {
			if((*hint)[i] == ',' || (*hint)[i] == '\0' || i >= 255) {
				strncpy(temp, *hint, i);
				temp[i] = '\0';
				if((*hint)[i] == ',') {
					*hint += i + 1;
				} else {
					*hint += i;
				}
				break;
			}
		}
	} else {
		temp[0] = '\0';
	}
	return temp;
}
