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

AssetLink::AssetLink() {}

AssetLinkPointers AssetLink::get() const {
	AssetLinkPointers pointers;
	const char* ptr = &data[0];
	if(prefix) {
		pointers.prefix = ptr;
		ptr += strlen(ptr) + 1;
	}
	pointers.tags.reserve(tags);
	for(s16 i = 0; i < tags; i++) {
		pointers.tags.push_back(ptr);
		ptr += strlen(ptr) + 1;
	}
	return pointers;
}

void AssetLink::set(const char* src) {
	prefix = false;
	tags = 0;
	size_t size = strlen(src);
	data.resize(size + 1);
	for(size_t i = 0; i < strlen(src); i++) {
		if(src[i] == '.' || src[i] == ':') {
			if(src[i] == ':') {
				verify(!prefix && tags == 0, "Syntax error while parsing asset link.");
				prefix = true;
			} else {
				tags++;
			}
			data[i] = '\0';
		} else {
			data[i] = src[i];
		}
	}
	tags++;
	data[size] = '\0';
}

void AssetLink::add_prefix(const char* str) {
	assert(!prefix && tags == 0);
	size_t size = strlen(str);
	data.resize(size + 1);
	memcpy(data.data(), str, size);
	data[size] = '\0';
	prefix = true;
}

void AssetLink::add_tag(const char* tag) {
	size_t old_size = data.size();
	size_t tag_size = strlen(tag);
	data.resize(old_size + tag_size + 1);
	for(size_t i = 0; i < tag_size; i++) {
		data[old_size + i] = tag[i];
	}
	data[old_size + tag_size] = '\0';
	tags++;
}

std::string AssetLink::to_string() const {
	std::string str;
	const char* ptr = &data[0];
	if(prefix) {
		str += ptr;
		str += ':';
		ptr += strlen(ptr) + 1;
	}
	for(s16 i = 0; i < tags; i++) {
		str += ptr;
		if(i != tags - 1) {
			str += '.';
		}
		ptr += strlen(ptr) + 1;
	}
	return str;
}

std::vector<ColladaScene*> read_collada_files(std::vector<std::unique_ptr<ColladaScene>>& owners, std::vector<FileReference> refs) {
	std::vector<ColladaScene*> scenes;
	for(size_t i = 0; i < refs.size(); i++) {
		bool unique = true;
		size_t j;
		for(j = 0; j < refs.size(); j++) {
			if(i > j && refs[i].owner == refs[j].owner && refs[i].path == refs[j].path) {
				unique = false;
				break;
			}
		}
		if(unique) {
			std::string xml = refs[i].owner->read_text_file(refs[i].path);
			std::unique_ptr<ColladaScene>& owner = owners.emplace_back(std::make_unique<ColladaScene>(read_collada((char*) xml.data())));
			scenes.emplace_back(owner.get());
		} else {
			scenes.emplace_back(scenes[j]);
		}
	}
	return scenes;
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
