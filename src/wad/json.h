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
//  from_json(vec, json);
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

[[maybe_unused]] static std::string encode_json_string(const std::string& input) {
	std::string output;
	for(char c : input) {
		output += HEX_DIGITS[(c & 0xff) >> 4];
		output += HEX_DIGITS[(c & 0xff) & 0xf];
	}
	return output;
}

[[maybe_unused]] static std::string decode_json_string(const std::string& input) {
	std::string output;
	verify(input.size() % 2 == 0, "Invalid string.");
	for(size_t i = 0; i < input.size(); i += 2) {
		u32 c;
		sscanf(&input[i], "%02x", &c);
		output += (char) (c & 0xff);
	}
	return output;
}

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
	verify(json.is_array(), "Expected JSON array for hexdump.");
	std::vector<u8> result;
	for(const Json& line : json) {
		verify(line.is_string(), "Expected JSON string.");
		std::string line_str = decode_json_string(line.get<std::string>());
		for(char c : line_str) {
			result.push_back(c);
		}
	}
	return result;
}

template <typename Object>
Json to_json(Object object);

struct GC_8c_DL_70;
struct Rgb96;
struct DL_3c;
struct GC_64_DL_48;
struct PropertiesSecondPart;
struct PropertiesThirdPart;

struct ToJsonVisitor {
	Json json;
	template <typename T>
	void field(const char* name, T& value) {
		if constexpr(std::is_same_v<T, std::string>) {
			json[name] = encode_json_string(value);
		} else {
			json[name] = to_json(value);
		}
	}
	template <typename T>
	void field(const char* name, std::vector<T>& list) {
		Json json_list = Json::array();
		for(auto& elem : list) {
			json_list.emplace_back(to_json(elem));
			if constexpr(std::is_compound_v<T>) {
				assert((json_list.back().find("original_index") != json_list.back().end()
					|| std::is_same_v<T, Vec3f>
					|| std::is_same_v<T, Vec4f>
					|| std::is_same_v<T, GC_8c_DL_70>
					|| std::is_same_v<T, Rgb96>
					|| std::is_same_v<T, DL_3c>
					|| std::is_same_v<T, GC_64_DL_48>
					|| std::is_same_v<T, PropertiesSecondPart>
					|| std::is_same_v<T, PropertiesThirdPart>
				));
			}
		}
		json[name] = json_list;
	}
	template <typename T>
	void field(const char* name, std::optional<T>& opt) {
		if(opt.has_value()) {
			field(name, *opt);
		}
	}
	void hexdump(const char* name, std::vector<u8>& buffer) {
		json[name] = buffer_to_json_hexdump(buffer);
	}
};

template <typename Object>
Json to_json(Object object) {
	if constexpr(std::is_compound_v<Object>) {
		ToJsonVisitor visitor;
		object.enumerate_fields(visitor);
		return visitor.json;
	} else {
		return object;
	}
}

template <typename Object>
void from_json(Object& dest, Json src);

struct FromJsonVisitor {
	Json& json;
	template <typename T>
	void field(const char* name, T& value) {
		if constexpr(std::is_same_v<T, std::string>) {
			verify(json.contains(name) && json[name].is_string(), "Expected string for field '%s'.", name);
			value = decode_json_string(json[name]);
		} else {
			verify(json.contains(name) && !json[name].is_null(), "Missing field '%s'.", name);
			from_json(value, json[name]);
		}
	}
	template <typename Field>
	void field(const char* name, std::vector<Field>& vec) {
		verify(json.contains(name) && !json[name].is_null(), "Missing field '%s'.", name);
		for(Json& element_json : json[name]) {
			Field element;
			from_json<Field>(element, element_json);
			vec.emplace_back(element);
		}
	}
	template <typename T>
	void field(const char* name, std::optional<T>& opt) {
		if(json.contains(name) && !json[name].is_null()) {
			T value;
			field(name, value);
			opt = value;
		}
	}
	void hexdump(const char* name, std::vector<u8>& buffer) {
		verify(json.contains(name), "Missing hexdump field '%s'.", name);
		if(!json[name].is_null()) {
			buffer = buffer_from_json_hexdump(json[name]);
		} else {
			buffer = {};
		}
	}
};

template <typename Object>
void from_json(Object& dest, Json src) {
	if constexpr(std::is_compound_v<Object>) {
		FromJsonVisitor visitor{src};
		dest.enumerate_fields(visitor);
	} else {
		dest = src;
	}
}

#endif
