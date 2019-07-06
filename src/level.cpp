#include "level.h"

std::vector<const point_object*> level::point_objects() const {
	std::vector<const point_object*> result;
	for(auto [uid, moby] : mobies()) {
		result.push_back(moby);
	}
	return result;
}

std::map<uint32_t, const moby*> level::mobies() const {
	auto moby_map = const_cast<level*>(this)->mobies();
	std::map<uint32_t, const moby*> result;
	for(auto& [uid, moby] : moby_map) {
		result.emplace(uid, moby);
	}
	return result;
}
