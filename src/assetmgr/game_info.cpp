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

#include "game_info.h"

#include <wtf/wtf.h>
#include <wtf/wtf_writer.h>

static std::string read_string_attribute(WtfNode* node, const char* name);

GameInfo read_game_info(char* input) {
	char* error_dest = nullptr;
	WtfNode* root = wtf_parse(input, &error_dest);
	if(error_dest) {
		fprintf(stderr, "warning: Failed to read gameinfo.txt!\n");
		return {};
	}
	
	GameInfo info;
	
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
	
	info.name = read_string_attribute(root, "name");
	info.author = read_string_attribute(root, "author");
	info.description = read_string_attribute(root, "description");
	info.version = read_string_attribute(root, "version");
	
	const WtfAttribute* images = wtf_attribute(root, "images");
	if(images && images->type == WTF_ARRAY) {
		for(const WtfAttribute* element = images->first_array_element; element != nullptr; element = element->next) {
			if(element->type == WTF_STRING) {
				info.images.emplace_back(element->string.begin);
			}
		}
	}
	
	const WtfAttribute* builds = wtf_attribute(root, "builds");
	if(builds && builds->type == WTF_ARRAY) {
		for(const WtfAttribute* element = builds->first_array_element; element != nullptr; element = element->next) {
			if(element->type == WTF_STRING) {
				info.builds.emplace_back(element->string.begin);
			}
		}
	}
	
	return info;
}

void write_game_info(std::string& dest, const GameInfo& info) {
	WtfWriter* ctx = wtf_begin_file(dest);
	
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
	
	wtf_write_string_attribute(ctx, "name", info.name.c_str());
	wtf_write_string_attribute(ctx, "author", info.author.c_str());
	wtf_write_string_attribute(ctx, "description", info.description.c_str());
	wtf_write_string_attribute(ctx, "version", info.version.c_str());
	
	wtf_begin_attribute(ctx, "images");
	wtf_begin_array(ctx);
	for(const std::string& image : info.images) {
		wtf_write_string(ctx, image.c_str());
	}
	wtf_end_array(ctx);
	wtf_end_attribute(ctx);
	
	wtf_begin_attribute(ctx, "builds");
	wtf_begin_array(ctx);
	for(const std::string& build : info.builds) {
		wtf_write_string(ctx, build.c_str());
	}
	wtf_end_array(ctx);
	wtf_end_attribute(ctx);
	
	wtf_end_file(ctx);
}

static std::string read_string_attribute(WtfNode* node, const char* name) {
	const WtfAttribute* attribute = wtf_attribute(node, name);
	if(attribute && attribute->type == WTF_STRING) {
		return attribute->string.begin;
	}
	return "";
}
