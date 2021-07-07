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

std::string encode_json_string(const std::string& input);
std::string decode_json_string(const std::string& input);
Json buffer_to_json_hexdump(const std::vector<u8>& buffer);
std::vector<u8> buffer_from_json_hexdump(const Json& json);

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
			verify(json.contains(name), "Missing field '%s'.", name);
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
	} else if(src.is_null()) {
		dest = Object();
	} else {
		dest = src;
	}
}

#endif
