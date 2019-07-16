#include "level.h"

const texture_provider* level::get_texture_provider() const {
	return const_cast<level*>(this)->get_texture_provider();
}

std::vector<const point_object*> level::point_objects() const {
	std::vector<const point_object*> result;
	for(auto moby : mobies()) {
		result.push_back(moby.second);
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

bool level::is_selected(const game_object* obj) const {
	return std::find(selection.begin(), selection.end(), obj) != selection.end();
}
