/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019 chaoticgd

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

#include "level.h"

#include "formats/level_impl.h"

base_level::base_level()
	: _history_index(0) { }

const texture_provider* base_level::get_texture_provider() const {
	return const_cast<base_level*>(this)->get_texture_provider();
}

std::vector<game_object*> base_level::game_objects() {
	static std::vector<tie>    ties;
	static std::vector<shrub>  shrubs;
	static std::vector<spline> splines;
	static std::vector<moby>   mobies;
	
	ties.clear();
	shrubs.clear();
	splines.clear();
	mobies.clear();
	
	auto this_ = static_cast<level*>(this);
	for(std::size_t i = 0; i < this_->num_ties(); i++) {
		ties.push_back(this_->tie_at(i));
	}
	for(std::size_t i = 0; i < this_->num_shrubs(); i++) {
		shrubs.push_back(this_->shrub_at(i));
	}
	for(std::size_t i = 0; i < this_->num_splines(); i++) {
		splines.push_back(this_->spline_at(i));
	}
	for(std::size_t i = 0; i < this_->num_mobies(); i++) {
		mobies.push_back(this_->moby_at(i));
	}
	
	std::vector<game_object*> result;
	result.reserve(ties.size() + shrubs.size() + splines.size() + mobies.size());
	std::transform(ties.begin(), ties.end(), result.end(), [](auto& ref) { return &ref; });
	std::transform(shrubs.begin(), shrubs.end(), result.end(), [](auto& ref) { return &ref; });
	std::transform(splines.begin(), splines.end(), result.end(), [](auto& ref) { return &ref; });
	std::transform(mobies.begin(), mobies.end(), result.end(), [](auto& ref) { return &ref; });
	return result;
}

std::vector<const game_object*> base_level::game_objects() const {
	auto result = const_cast<base_level*>(this)->game_objects();
	return std::vector<const game_object*>(result.begin(), result.end());
}

std::vector<point_object*> base_level::point_objects() {
	static std::vector<tie>    ties;
	static std::vector<shrub>  shrubs;
	static std::vector<moby>   mobies;
	
	ties.clear();
	shrubs.clear();
	mobies.clear();
	
	auto this_ = static_cast<level*>(this);
	for(std::size_t i = 0; i < this_->num_ties(); i++) {
		ties.push_back(this_->tie_at(i));
	}
	for(std::size_t i = 0; i < this_->num_shrubs(); i++) {
		shrubs.push_back(this_->shrub_at(i));
	}
	for(std::size_t i = 0; i < this_->num_mobies(); i++) {
		mobies.push_back(this_->moby_at(i));
	}
	
	std::vector<point_object*> result
		(ties.size() + shrubs.size() + mobies.size());
	std::transform(ties.begin(), ties.end(), result.end(), [](auto& ref) { return &ref; });
	std::transform(shrubs.begin(), shrubs.end(), result.end(), [](auto& ref) { return &ref; });
	std::transform(mobies.begin(), mobies.end(), result.end(), [](auto& ref) { return &ref; });
	return result;
}

std::vector<const point_object*> base_level::point_objects() const {
	auto result = const_cast<base_level*>(this)->point_objects();
	return std::vector<const point_object*>(result.begin(), result.end());
}

bool base_level::is_selected(const game_object* obj) const {
	return std::find(selection.begin(), selection.end(), obj->base()) != selection.end();
}

void base_level::undo() {
	if(_history_index <= 0) {
		throw command_error("Nothing to undo.");
	}
	_history_stack[--_history_index]->undo();
}

void base_level::redo() {
	if(_history_index >= _history_stack.size()) {
		throw command_error("Nothing to redo.");
	}
	_history_stack[_history_index++]->apply();
}
