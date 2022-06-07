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

GameInfo read_game_info(char* input) {
	char* error_dest = nullptr;
	WtfNode* root = wtf_parse(input, &error_dest);
	if(error_dest) {
		fprintf(stderr, "warning: Failed to read gameinfo.txt!\n");
		return {};
	}
	
	GameInfo info;
	
	const WtfAttribute* name = wtf_attribute(root, "name");
	if(name && name->type == WTF_STRING) {
		info.name = name->string.begin;
	}
	
	const WtfAttribute* format_version = wtf_attribute(root, "format_version");
	if(format_version && format_version->type == WTF_NUMBER) {
		info.format_version = format_version->number.i;
	}
	
	const WtfAttribute* type = wtf_attribute(root, "type");
	if(type && type->type == WTF_STRING) {
		if(strcmp(type->string.begin, "unpacked") == 0) {
			info.type = AssetBankType::UNPACKED;
		} else if(strcmp(type->string.begin, "test") == 0) {
			info.type = AssetBankType::TEST;
		} else {
			info.type = AssetBankType::MOD;
		}
	} else {
		fprintf(stderr, "warning: No type attribute in gameinfo.txt file.\n");
	}
	
	const WtfAttribute* builds = wtf_attribute(root, "builds");
	if(builds && builds->type == WTF_ARRAY) {
		for(const WtfAttribute* element = builds->first_array_element; element != nullptr; element = element->next) {
			if(element->type == WTF_STRING) {
				info.builds.emplace_back(parse_asset_reference(element->string.begin));
			}
		}
	}
	
	return info;
}

void write_game_info(std::string& dest, const GameInfo& info) {
	WtfWriter* ctx = wtf_begin_file(dest);
	
	wtf_begin_attribute(ctx, "name");
	wtf_write_string(ctx, info.name.c_str());
	wtf_end_attribute(ctx);
	
	wtf_begin_attribute(ctx, "format_version");
	wtf_write_integer(ctx, info.format_version);
	wtf_end_attribute(ctx);
	
	wtf_begin_attribute(ctx, "type");
	if(info.type == AssetBankType::UNPACKED) {
		wtf_write_string(ctx, "unpacked");
	} else if(info.type == AssetBankType::TEST) {
		wtf_write_string(ctx, "test");
	} else {
		wtf_write_string(ctx, "mod");
	}
	wtf_end_attribute(ctx);
	
	wtf_begin_attribute(ctx, "builds");
	wtf_begin_array(ctx);
	for(const AssetReference& reference : info.builds) {
		std::string str = asset_reference_to_string(reference);
		wtf_write_string(ctx, str.c_str());
	}
	wtf_end_array(ctx);
	wtf_end_attribute(ctx);
	
	wtf_end_file(ctx);
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
