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
	pointers.fragments.reserve(fragments);
	for(s16 i = 0; i < fragments; i++) {
		pointers.fragments.push_back(ptr);
		ptr += strlen(ptr) + 1;
	}
	return pointers;
}

void AssetLink::set(const char* src) {
	prefix = false;
	fragments = 0;
	size_t size = strlen(src);
	data.resize(size + 1);
	for(size_t i = 0; i < strlen(src); i++) {
		if(src[i] == '.' || src[i] == ':') {
			if(src[i] == ':') {
				verify(!prefix && fragments == 0, "Syntax error while parsing asset link.");
				prefix = true;
			} else {
				fragments++;
			}
			data[i] = '\0';
		} else {
			data[i] = src[i];
		}
	}
	fragments++;
	data[size] = '\0';
}

void AssetLink::add_tag(const char* tag) {
	size_t old_size = data.size();
	size_t tag_size = strlen(tag);
	data.resize(old_size + tag_size + 1);
	for(size_t i = 0; i < tag_size; i++) {
		data[old_size + i] = tag[i];
	}
	data[old_size + tag_size] = '\0';
	fragments++;
}

std::string AssetLink::to_string() const {
	std::string str;
	const char* ptr = &data[0];
	if(prefix) {
		str += ptr;
		str += ':';
		ptr += strlen(ptr) + 1;
	}
	for(s16 i = 0; i < fragments; i++) {
		str += ptr;
		if(i != fragments - 1) {
			str += '.';
		}
		ptr += strlen(ptr) + 1;
	}
	return str;
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
