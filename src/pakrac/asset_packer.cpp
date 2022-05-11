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

#include "asset_packer.h"

#include <iso/iso_packer.h>

s32 g_asset_packer_max_assets_processed = 0;
s32 g_asset_packer_num_assets_processed = 0;
bool g_asset_packer_dry_run = false;

on_load(Packer, []() {BuildAsset::funcs.pack_rac1 = wrap_iso_packer_func<BuildAsset>(pack_iso, pack_asset_impl);
	BuildAsset::funcs.pack_rac2 = wrap_iso_packer_func<BuildAsset>(pack_iso, pack_asset_impl);
	BuildAsset::funcs.pack_rac3 = wrap_iso_packer_func<BuildAsset>(pack_iso, pack_asset_impl);
	BuildAsset::funcs.pack_dl = wrap_iso_packer_func<BuildAsset>(pack_iso, pack_asset_impl);
})

void pack_asset_impl(OutputStream& dest, std::vector<u8>* header_dest, fs::file_time_type* time_dest, Asset& src, Game game, AssetFormatHint hint) {
	std::string reference = asset_reference_to_string(src.reference());
	
	if(!g_asset_packer_dry_run) {
		std::string type = asset_type_to_string(src.type());
		for(char& c : type) c = tolower(c);
		s32 completion_percentage = (s32) ((g_asset_packer_num_assets_processed * 100.f) / g_asset_packer_max_assets_processed);
		printf("[%3d%%] \033[32mPacking %s asset %s\033[0m\n", completion_percentage, type.c_str(), reference.c_str());
	}
	
	AssetPackerFunc* pack_func = nullptr;
	switch(game) {
		case Game::RAC1: pack_func = src.funcs.pack_rac1; break;
		case Game::RAC2: pack_func = src.funcs.pack_rac2; break;
		case Game::RAC3: pack_func = src.funcs.pack_rac3; break;
		case Game::DL: pack_func = src.funcs.pack_dl; break;
	}
	
	dest.seek(dest.size());
	SubOutputStream sub_dest(dest, dest.tell());
	
	verify(pack_func, "Tried to pack nonpackable asset '%s'.", reference.c_str());
	(*pack_func)(sub_dest, header_dest, time_dest, src, game, hint);
	
	dest.seek(dest.size());
	
	g_asset_packer_num_assets_processed++;
}
