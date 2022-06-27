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
#include "assetmgr/asset_util.h"

#include <gui/gui.h>

static void recurse(Asset& asset, AssetSelector& state);

static std::vector<Asset*> assets;

Asset* asset_selector(const char* label, const char* preview_value, AssetSelector& state, AssetForest& forest) {
	Asset* selected = nullptr;
	static bool open_last_frame = false;
	if(ImGui::BeginCombo(label, preview_value)) {
		if(!open_last_frame) {
			assets.clear();
			if(Asset* root = forest.any_root()) {
				recurse(*root, state);
			}
			open_last_frame = true;
		}
		for(Asset* asset : assets) {
			if(ImGui::Selectable(asset->absolute_link().to_string().c_str())) {
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
		assets.push_back(&asset.highest_precedence());
		if(state.no_recurse) {
			return;
		}
	}
	asset.for_each_logical_child([&](Asset& child) {
		recurse(child, state);
	});
}
