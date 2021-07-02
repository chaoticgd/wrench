/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2021 chaoticgd

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

#ifndef WAD_JSON_H
#define WAD_JSON_H

// Example usage:
//  Vec3f vec;
//  ...
//  Json json = to_json(vec);
//  vec = from_json(json);
//  
// Given the type definition:
//  packed_struct(Vec3f,
//  	f32 x;
//  	f32 y;
//  	f32 z;
//  	
//  	template <typename T>
//  	void enumerate_fields(T& t) {
//  		DEF_PACKED_FIELD(x);
//  		DEF_PACKED_FIELD(y);
//  		DEF_PACKED_FIELD(z);
//  	}
//  )

#include <nlohmann/json.hpp>
using Json = nlohmann::ordered_json;

#include "util.h"

static const char* HEX_DIGITS = "0123456789abcdef";

[[maybe_unused]] static Json buffer_to_json_hexdump(const std::vector<u8>& buffer) {
	Json json;
	for(size_t i = 0; i < buffer.size(); i += 0x10) {
		std::string line;
		for(size_t j = 0; j < 0x10 && i + j < buffer.size(); j++) {
			u8 byte = buffer[i + j];
			line += HEX_DIGITS[byte >> 4];
			line += HEX_DIGITS[byte & 0xf];
		}
		json.emplace_back(line);
	}
	return json;
}

[[maybe_unused]] static std::vector<u8> buffer_from_json_hexdump(const Json& json) {
	return {};
}

static std::string encode_json_string(const std::string& input) {
	std::string output;
	for(char c : input) {
		if(c == '\\') {
			output += '\\';
			output += '\\';
		} else if(c >= 0x20 && c < 0x7f) {
			output += c;
		} else {
			output += '\\';
			output += 'x';
			output += HEX_DIGITS[(c & 0xff) >> 4];
			output += HEX_DIGITS[(c & 0xff) & 0xf];
		}
	}
	return output;
}

struct ToJsonVisitor {
	Json json;
	template <typename Field>
	void field(const char* name, Field& field) {
		if constexpr(std::is_compound_v<Field>) {
			json[name] = to_json(field);
		} else {
			json[name] = field;
		}
	}
	template <typename Element>
	void field(const char* name, std::vector<Element>& list) {
		Json json_list;
		for(auto& elem : list) {
			if constexpr(std::is_compound_v<Element>) {
				json_list.emplace_back(to_json(elem));
			} else {
				json_list.emplace_back(elem);
			}
		}
		json[name] = json_list;
	}
	template <typename Element>
	void field(const char* name, std::vector<std::vector<Element>>& list) {
		Json outer_json;
		for(auto& inner : list) {
			Json inner_json;
			for(auto& elem : inner) {
				if constexpr(std::is_compound_v<Element>) {
					inner_json.emplace_back(to_json(elem));
				} else {
					inner_json.emplace_back(elem);
				}
			}
			outer_json.emplace_back(inner_json);
		}
		json[name] = outer_json;
	}
	template <typename OptionalField>
	void optional(const char* name, OptionalField& opt) {
		if(opt.has_value()) {
			field(name, *opt);
		}
	}
	void hexdump(const char* name, std::vector<u8>& buffer) {
		json[name] = buffer_to_json_hexdump(buffer);
	}
	void hexdump(const char* name, std::vector<std::vector<u8>>& list) {
		for(auto& buffer : list) {
			json[name].emplace_back(buffer_to_json_hexdump(buffer));
		}
	}
	void string(const char* name, std::optional<std::string>& string) {
		if(string.has_value()) {
			json[name] = encode_json_string(*string);
		}
	}
};

template <typename Object>
Json to_json(Object object) {
	ToJsonVisitor visitor;
	object.enumerate_fields(visitor);
	return visitor.json;
};

template <typename Object>
Object from_json(Json json);

struct FromJsonVisitor {
	Json& json;
	template <typename Field>
	void field(const char* name, Field& field) {
		field = json[name];
	}
	void hexdump(const char* name, std::vector<u8>& buffer) {
		buffer = buffer_from_json_hexdump(json[name]);
	}
};

template <typename Object>
Object from_json(Json json) {
	Object object;
	object.enumerate_fields(FromJsonVisitor{json});
	return object;
};

#endif
