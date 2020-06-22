/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2020 chaoticgd

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

#ifndef FORMATS_LEVEL_IMPL_H
#define FORMATS_LEVEL_IMPL_H

#include <memory>
#include <variant>
#include <stdint.h>

#include "../util.h"
#include "../stream.h"
#include "../worker_logger.h"
#include "toc.h"
#include "wad.h"
#include "tfrag.h"
#include "racpak.h"
#include "texture.h"
#include "game_model.h"
#include "level_types.h"

# /*
#	Read LEVEL*.WAD files.
# */

struct object_id {
	std::size_t value;
	
	bool operator<(const object_id& rhs) const {
		return value < rhs.value;
	}
	
	bool operator==(const object_id& rhs) const {
		return value == rhs.value;
	}
};

#define for_each_object_type(...) \
	(__VA_ARGS__).template operator()<tie>(); \
	(__VA_ARGS__).template operator()<shrub>(); \
	(__VA_ARGS__).template operator()<moby>(); \
	(__VA_ARGS__).template operator()<spline>()

// Access the member of the input struct corresponding to the object type T.
template <typename T, typename T_in>
auto& member_of_type(T_in& in) {
	if constexpr(std::is_same_v<T, tie>) return in.ties;
	if constexpr(std::is_same_v<T, shrub>) return in.shrubs;
	if constexpr(std::is_same_v<T, moby>) return in.mobies;
	if constexpr(std::is_same_v<T, spline>) return in.splines;
	
	// FIXME: This should be a compile-time error!
	throw std::runtime_error("member_of_type called with invalid object type!");
}

struct object_list {
	std::vector<object_id> ties;
	std::vector<object_id> shrubs;
	std::vector<object_id> mobies;
	std::vector<object_id> splines;
	
	template <typename T>
	void add(object_id id) {
		member_of_type<T>(*this).push_back(id);
	}
	
	std::size_t size() const {
		std::size_t result = 0;
		for_each_object_type([&]<typename T>() {
			result += member_of_type<T>(*this).size();
		});
		return result;
	}
	
	bool contains(object_id id) const {
		bool result = false;
		for_each_object_type([&]<typename T>() {
			const std::vector<object_id>& objects = member_of_type<T>(*this);
			if(std::find(objects.begin(), objects.end(), id) != objects.end()) {
				result = true;
			}
		});
		return result;
	}
	
	object_id first() const {
		std::optional<object_id> result;
		for_each_object_type([&]<typename T>() {
			const std::vector<object_id>& objects = member_of_type<T>(*this);
			if(!result && objects.size() > 0) {
				result = objects[0];
			}
		});
		if(!result) {
			throw std::runtime_error("object_list::first called on an empty object_list. Add an if(x.size() > 0) check.");
		}
		return *result;
	}
};

class game_world {
public:
	struct game_string {
		uint32_t id;
		std::string content;	
	};
	
	object_list selection;
	world_ship_data ship;
	
	game_world() {}
	
	bool is_selected(object_id id) const;
	
	void read(stream* src);
	void write();
	
	// Maps from logical object IDs to physical array indices and vice versa.
	struct object_mappings {
		std::map<object_id, std::size_t> id_to_index;
		std::map<std::size_t, object_id> index_to_id;
	};
	
	template <typename T>
	bool object_exists(object_id id) {
		auto& id_to_index = mappings_of_type<T>().id_to_index;
		return id_to_index.find(id) != id_to_index.end();
	}
	
	template <typename T>
	T& object_from_id(object_id id) {
		object_mappings& mappings = mappings_of_type<T>();
		return objects_of_type<T>()[mappings.id_to_index.at(id)];
	}
	
	template <typename T>
	T& object_from_index(std::size_t index) {
		return objects_of_type<T>()[index];
	}

	template <typename T>
	std::size_t count() {
		return objects_of_type<T>().size();
	}
		
	struct object_callbacks {
		std::function<void(object_id, tie&)> ties;
		std::function<void(object_id, shrub&)> shrubs;
		std::function<void(object_id, moby&)> mobies;
		std::function<void(object_id, spline&)> splines;
	};
	
	#define find_object_by_id(id, ...) \
		find_object_by_id_impl(id, {__VA_ARGS__, __VA_ARGS__, __VA_ARGS__, __VA_ARGS__})
		
	void find_object_by_id_impl(object_id id, object_callbacks cbs) {
		for_each_object_type([&]<typename T>() {
			if(object_exists<T>(id)) {
				member_of_type<T>(cbs)(id, object_from_id<T>(id));
			}
		});
	}
	
	template <typename T>
	void for_each_object_of_type(std::function<void(object_id, T&)> callback) {
		auto& objects = objects_of_type<T>();
		for(std::size_t i = 0; i < objects.size(); i++) {
			object_id id = mappings_of_type<T>().index_to_id.at(i);
			callback(id, objects[i]);
		}
	}
	
	#define for_each_object(...) \
		for_each_object_impl({__VA_ARGS__, __VA_ARGS__, __VA_ARGS__, __VA_ARGS__})
	
	void for_each_object_impl(object_callbacks cbs) {
		for_each_object_type([&]<typename T>() {
			for_each_object_of_type<T>(member_of_type<T>(cbs));
		});
	}
	
	template <typename T>
	void for_each_object_of_type_in(
			std::vector<object_id> objects, std::function<void(object_id, T&)> callback) {
		for(object_id id : objects) {
			if(object_exists<T>(id)) {
				callback(id, object_from_id<T>(id));
			}
		}
	}
	
	#define for_each_object_in(list, ...) \
		for_each_object_in_impl(list, {__VA_ARGS__, __VA_ARGS__, __VA_ARGS__, __VA_ARGS__})
	
	void for_each_object_in_impl(object_list& list, object_callbacks cbs) {
		for_each_object_type([&]<typename T>() {
			for_each_object_of_type_in<T>
				(member_of_type<T>(list), member_of_type<T>(cbs));
		});
	}
	
private:
	template <typename T>
	std::vector<T>& objects_of_type() {
		return member_of_type<T>(_object_store);
	}
	
	struct {
		std::vector<tie> ties;
		std::vector<shrub> shrubs;
		std::vector<moby> mobies;
		std::vector<spline> splines;
	} _object_store;
	
	template <typename T>
	object_mappings& mappings_of_type() {
		return member_of_type<T>(_object_mappings);
	}
	
	struct {
		object_mappings ties;
		object_mappings shrubs;
		object_mappings mobies;
		object_mappings splines;
	} _object_mappings;
	std::size_t _next_object_id = 1; // ID to assign to the next new object.
	
	std::map<std::string, std::vector<game_string>> _languages;
};

struct level_code_segment {
	level_code_segment_header header;
	std::vector<uint8_t> bytes;
};

class level {
public:
	level(iso_stream* iso, toc_level index);
	
	std::map<std::string, std::map<uint32_t, std::string>> game_strings() { return {}; }

	stream* moby_stream();

	game_world world;
	
	std::map<uint32_t, std::size_t> moby_class_to_model;
	std::vector<game_model> moby_models;
	std::vector<texture> mipmap_textures;
	std::vector<texture> terrain_textures;
	std::vector<texture> tie_textures;
	std::vector<texture> moby_textures;
	std::vector<texture> sprite_textures;
	std::vector<tfrag> tfrags;
	
	level_code_segment code_segment;
	
private:
	toc_level _index;
	level_file_header _file_header;
	proxy_stream _file;
	stream* _moby_stream;
	stream* _asset_segment;
	std::optional<trace_stream> _asset_segment_tracepoint;
};

#endif
