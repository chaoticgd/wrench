/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2022 chaoticgd

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

#include "asset_selector.h"

#include <assetmgr/asset_types.h>
#include <gui/gui.h>

static void recurse(Asset& asset, AssetSelector& state);
static std::string get_display_name(Asset& asset);
static size_t compare_asset_links_numerically(const std::pair<std::string, Asset*>& lhs, const std::pair<std::string, Asset*>& rhs);

static std::vector<std::pair<std::string, Asset*>> assets;

Asset* asset_selector(const char* label, const char* preview_value, AssetSelector& state, AssetForest& forest) {
	Asset* selected = nullptr;
	static bool open_last_frame = false;
	if(ImGui::BeginCombo(label, preview_value)) {
		if(!open_last_frame) {
			assets.clear();
			if(Asset* root = forest.any_root()) {
				recurse(*root, state);
			}
			std::sort(BEGIN_END(assets), compare_asset_links_numerically);
			open_last_frame = true;
		}
		for(auto [link, asset] : assets) {
			if(ImGui::Selectable(link.c_str())) {
				selected = asset;
			}
		}
		ImGui::EndCombo();
	} else {
		open_last_frame = false;
	}
	return selected;
}

static void recurse(Asset& asset, AssetSelector& state) {
	if(asset.logical_type() == state.required_type) {
		assets.emplace_back(get_display_name(asset), &asset);
		if(state.no_recurse) {
			return;
		}
	}
	asset.for_each_logical_child([&](Asset& child) {
		recurse(child, state);
	});
}

static std::string get_display_name(Asset& asset) {
	std::string link = asset.absolute_link().to_string();
	if(asset.logical_type() == LevelAsset::ASSET_TYPE) {
		LevelAsset& level = asset.as<LevelAsset>();
		if(level.has_name()) {
			return link + " " + level.name();
		}
	}
	return link;
}

static size_t compare_asset_links_numerically(const std::pair<std::string, Asset*>& lhs, const std::pair<std::string, Asset*>& rhs) {
	const std::string& l = lhs.first;
	const std::string& r = rhs.first;
	size_t i = 0, j = 0;
	while(i < l.size() && j < r.size()) {
		if(isdigit(l[i]) && isdigit(r[j])) {
			size_t start_i = i;
			size_t start_j = j;
			do {
				i++;
			} while(l[i] >= '0' && l[i] <= '9');
			do {
				j++;
			} while(r[j] >= '0' && r[j] <= '9');
			if(i - start_i != j - start_j) {
				return i - start_i < j - start_j;
			}
			for(size_t k = 0; k < i - start_i; k++) {
				if(l[start_i + k] != r[start_j + k]) {
					return l[start_i + k] < r[start_j + k];
				}
			}
		} else if(l[i] != r[j]) {
			return l[i] < r[j];
		} else {
			i++; j++;
		}
	}
	return l.size() < r.size();
}
